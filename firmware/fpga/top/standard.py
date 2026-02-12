#
# This file is part of HackRF.
#
# Copyright (c) 2025 Great Scott Gadgets <info@greatscottgadgets.com>
# SPDX-License-Identifier: BSD-3-Clause

from amaranth               import Elaboratable, Module, Signal, Mux, DomainRenamer
from amaranth.lib           import cdc
from amaranth.lib.wiring    import connect

from amaranth_future        import fixed

from board                  import PralinePlatform, ClockDomainGenerator
from interface              import MAX586xInterface, SGPIOInterface, SPIRegisterInterface
from dsp.fir                import HalfBandDecimator, HalfBandInterpolator
from dsp.cic                import CICInterpolator
from dsp.dc_block           import DCBlock
from dsp.quarter_shift      import QuarterShift
from dsp.nco                import NCO
from util                   import ClockConverter, IQSample, StreamSkidBuffer


class Top(Elaboratable):

    def elaborate(self, platform):
        m = Module()

        m.submodules.clkgen = ClockDomainGenerator()
        adc_clk = "adclk"
        dac_clk = "daclk"

        # Submodules.
        m.submodules.adcdac_intf = adcdac_intf = MAX586xInterface(adc_domain=adc_clk, dac_domain=dac_clk)
        m.submodules.mcu_intf    = mcu_intf    = SGPIOInterface(sample_width=16, domain="sync")

        m.d.comb += adcdac_intf.q_invert.eq(platform.request("q_invert").i)

        # Half-band filter taps.
        taps = [-2, 0, 7, 0, -18, 0, 41, 0, -92, 0, 320, 512, 320, 0, -92, 0, 41, 0, -18, 0, 7, 0, -2]
        taps = [ tap/1024 for tap in taps ]

        taps2 = [3, 0, -16, 0, 77, 128, 77, 0, -16, 0, 3]
        taps2 = [ tap/256 for tap in taps2 ]

        taps3 = [-9, 0, 73, 128, 73, 0, -9]
        taps3 = [ tap/256 for tap in taps3 ]

        taps4 = [-8, 0, 72, 128, 72, 0, -8]
        taps4 = [ tap/256 for tap in taps4 ]

        taps5 = [-1, 0, 9, 16, 9, 0, -1]
        taps5 = [ tap/32 for tap in taps5 ]

        common_rx_filter_opts = dict(
            data_shape=fixed.SQ(7),
            always_ready=True,
            domain=adc_clk,
        )

        rx_chain = {
            # DC block and quarter shift.
            "dc_block":      DCBlock(width=8, num_channels=2, domain=adc_clk),
            "quarter_shift": DomainRenamer(adc_clk)(QuarterShift()),

            # Half-band decimation stages.
            "hbfir5":        HalfBandDecimator(taps5, **common_rx_filter_opts),
            "hbfir4":        HalfBandDecimator(taps4, **common_rx_filter_opts),
            "hbfir3":        HalfBandDecimator(taps3, **common_rx_filter_opts),
            "hbfir2":        HalfBandDecimator(taps2, **common_rx_filter_opts),
            "hbfir1":        HalfBandDecimator(taps, **common_rx_filter_opts),

            # Clock domain conversion.
            "clkconv":       ClockConverter(IQSample(8), 8, adc_clk, "sync"),
        }
        for k,v in rx_chain.items():
            m.submodules[f"rx_{k}"] = v

        # Connect receiver chain.
        last = adcdac_intf.adc_stream
        for block in rx_chain.values():
            connect(m, last, block.input)
            last = block.output
        connect(m, last, mcu_intf.adc_stream)

        tx_chain = {
            # Clock domain conversion.
            "clkconv":          ClockConverter(IQSample(8), 8, "sync", dac_clk, always_ready=False), 

            # Half-band interpolation stages (+ skid buffers for timing closure).
            "hbfir1":           HalfBandInterpolator(taps, data_shape=fixed.SQ(7), 
                num_channels=2, always_ready=False, domain=dac_clk),
            "skid2":            DomainRenamer(dac_clk)(StreamSkidBuffer(IQSample(8), always_ready=False)),
            "hbfir2":           HalfBandInterpolator(taps2, data_shape=fixed.SQ(7), 
                num_channels=2, always_ready=False, domain=dac_clk),
            "skid3":            DomainRenamer(dac_clk)(StreamSkidBuffer(IQSample(8), always_ready=False)),

            # CIC interpolation stage.
            "cic_interpolator": CICInterpolator(1, 3, (1, 2, 4, 8), 8, 8, num_channels=2, 
                always_ready=False, domain=dac_clk),
            "skid4":            DomainRenamer(dac_clk)(StreamSkidBuffer(IQSample(8), always_ready=False)),
        }
        for k,v in tx_chain.items():
            m.submodules[f"tx_{k}"] = v

        # Connect transmitter chain.
        last = mcu_intf.dac_stream
        for block in tx_chain.values():
            connect(m, last, block.input)
            last = block.output
        # DAC can also be driven with an internal NCO.
        m.submodules.nco = nco = DomainRenamer(dac_clk)(NCO(phase_width=16, output_width=8))
        with m.If(nco.en):
            m.d.comb += [
                adcdac_intf.dac_stream.p.eq(nco.output),
                adcdac_intf.dac_stream.valid.eq(1),
                last.ready.eq(1),
            ]
        with m.Else():
            connect(m, last, adcdac_intf.dac_stream)

        # SPI register interface.
        spi_port = platform.request("spi")
        m.submodules.spi_regs = spi_regs = SPIRegisterInterface(spi_port)

        # Add control registers.
        ctrl     = spi_regs.add_register(0x01, init=0)
        rx_decim = spi_regs.add_register(0x02, init=0, size=3)
        tx_ctrl  = spi_regs.add_register(0x03, init=0, size=1)
        tx_intrp = spi_regs.add_register(0x04, init=0, size=3)
        tx_pstep = spi_regs.add_register(0x05, init=0)

        m.d.sync += [
            # Trigger enable.
            mcu_intf.trigger_en                 .eq(ctrl[7]),

            # PRBS enable.
            mcu_intf.prbs                       .eq(ctrl[6]),

            # RX settings.
            rx_chain["dc_block"].enable         .eq(ctrl[0]),
            rx_chain["quarter_shift"].enable    .eq(ctrl[1]),
            rx_chain["quarter_shift"].up        .eq(ctrl[2]),

            # RX decimation rate.
            rx_chain["hbfir5"].enable           .eq(rx_decim > 4),
            rx_chain["hbfir4"].enable           .eq(rx_decim > 3),
            rx_chain["hbfir3"].enable           .eq(rx_decim > 2),
            rx_chain["hbfir2"].enable           .eq(rx_decim > 1),
            rx_chain["hbfir1"].enable           .eq(rx_decim > 0),

            # TX interpolation rate.
            tx_chain["cic_interpolator"].factor .eq(Mux(tx_intrp > 2, tx_intrp - 2, 0)),
            tx_chain["hbfir1"].enable           .eq(tx_intrp > 0),
            tx_chain["hbfir2"].enable           .eq(tx_intrp > 1),
        ]

        # TX NCO control.
        tx_pstep_dacclk = Signal(8)
        m.submodules.nco_phase_cdc = cdc.FFSynchronizer(tx_pstep, tx_pstep_dacclk, o_domain=dac_clk)
        m.d[dac_clk] += [
            nco.en                              .eq(tx_ctrl[0]),
            nco.phase                           .eq(nco.phase + (tx_pstep_dacclk << 6)),
        ]

        return m


if __name__ == "__main__":
    plat = PralinePlatform()
    plat.build(Top())
