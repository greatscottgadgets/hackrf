#
# This file is part of HackRF.
#
# Copyright (c) 2025 Great Scott Gadgets <info@greatscottgadgets.com>
# SPDX-License-Identifier: BSD-3-Clause

from amaranth               import Elaboratable, Module, Cat, DomainRenamer, Signal
from amaranth.lib           import cdc
from amaranth.lib.wiring    import connect

from amaranth_future        import fixed

from board                  import PralinePlatform, ClockDomainGenerator
from interface              import MAX586xInterface, SGPIOInterface, SPIRegisterInterface
from dsp.fir                import FIRFilter
from dsp.fir_mac16          import HalfBandDecimatorMAC16
from dsp.cic                import CICDecimator
from dsp.dc_block           import DCBlock
from dsp.nco                import NCO
from dsp.mixer              import ComplexMultiplier

from util                   import ClockConverter, IQSample


class Top(Elaboratable):

    def elaborate(self, platform):
        m = Module()

        m.submodules.clkgen = ClockDomainGenerator()
        adc_clk = "adclk"
        dac_clk = "daclk"

        # Submodules.
        m.submodules.adcdac_intf = adcdac_intf = MAX586xInterface(adc_domain=adc_clk, dac_domain=dac_clk)
        m.submodules.mcu_intf    = mcu_intf    = SGPIOInterface(
            sample_width=24,
            rx_assignments=[
                lambda w: w[0:8],
                lambda w: Cat(w[8:12], w[11].replicate(4)),
                lambda w: w[12:20],
                lambda w: Cat(w[20:24], w[23].replicate(4)),
            ],
            tx_assignments=[
                lambda w, v: w[0:8].eq(v),
                lambda w, v: w[8:12].eq(v),
                lambda w, v: w[12:20].eq(v),
                lambda w, v: w[20:24].eq(v),
            ],
            domain="sync"
        )

        m.d.comb += adcdac_intf.q_invert.eq(platform.request("q_invert").i)

        # Half-band filter taps.
        taps_hb1 = [-2, 0, 5, 0, -10, 0,18, 0, -30, 0,53, 0,-101, 0, 323, 512, 323, 0,-101, 0, 53, 0, -30, 0,18, 0, -10, 0, 5, 0,-2]
        taps_hb1 = [ tap/1024 for tap in taps_hb1 ]

        taps_hb2 = [ -6, 0, 19,  0, -44,  0, 89,  0, -163,  0, 278,  0, -452,  0, 711,  0, -1113,  0, 1800, 0, -3298,  0, 10370, 16384, 10370,  0, -3298,  0, 1800,  0, -1113,  0, 711,  0, -452,  0,  278,  0, -163,  0, 89,  0, -44,  0, 19,  0, -6]
        taps_hb2 = [ tap/16384/2 for tap in taps_hb2 ]

        # NCO and mixer.
        m.submodules.nco = nco = DomainRenamer(adc_clk)(NCO(phase_width=16, output_width=10))
        mixer = DomainRenamer(adc_clk)(ComplexMultiplier(IQSample(8), IQSample(10), IQSample(8), always_ready=True))
        mixer.input = mixer.a
        mixer.output = mixer.c
        m.d.comb += [
            mixer.b.p.eq(nco.output),
            mixer.b.valid.eq(1),
            nco.en.eq(1),
        ]

        rx_chain = {
            # DC block and mixer.
            "dc_block":      DCBlock(width=8, num_channels=2, domain=adc_clk),
            "mixer":         mixer,

            # CIC mandatory first stage with compensator.
            "cic":          CICDecimator(2, 4, (4,8,16,32), width_in=8, width_out=12, num_channels=2, always_ready=True, domain=adc_clk),
            "cic_comp":     DomainRenamer(adc_clk)(FIRFilter([-0.125, 0, 0.75, 0, -0.125], shape=fixed.SQ(11), shape_out=fixed.SQ(11), always_ready=True, num_channels=2)),

            # Final half-band decimator stages.
            "hbfir1":       HalfBandDecimatorMAC16(taps_hb1, data_shape=fixed.SQ(11), overclock_rate=4, always_ready=True, domain=adc_clk),
            "hbfir2":       HalfBandDecimatorMAC16(taps_hb2, data_shape=fixed.SQ(11), overclock_rate=8, always_ready=True, domain=adc_clk),

            # Clock domain conversion.
            "clkconv":      ClockConverter(IQSample(12), 8, adc_clk, "sync", always_ready=True),
        }
        for k,v in rx_chain.items():
            m.submodules[f"rx_{k}"] = v

        # Connect receiver chain.
        last = adcdac_intf.adc_stream
        for block in rx_chain.values():
            connect(m, last, block.input)
            last = block.output
        connect(m, last, mcu_intf.adc_stream)

        # SPI register interface.
        spi_port = platform.request("spi")
        m.submodules.spi_regs = spi_regs = SPIRegisterInterface(spi_port)

        # Add control registers.
        ctrl         = spi_regs.add_register(0x01, init=0)
        rx_decim     = Signal(3, init=2)
        rx_decim_new = Signal(3)
        rx_decim_stb = Signal()
        spi_regs.add_sfr(0x02, read=rx_decim, write_signal=rx_decim_new, write_strobe=rx_decim_stb)
        rx_nco       = spi_regs.add_register(0x03, init=0)

        m.d.comb += [
            # Trigger enable.
            mcu_intf.trigger_en                 .eq(ctrl[7]),
        ]
        
        # RX DC block.
        m.submodules.dc_block_cdc = cdc.FFSynchronizer(ctrl[0], rx_chain["dc_block"].enable, o_domain=adc_clk)

        # RX decimation rate.
        m.submodules.rx_decim_cdc = cdc.FFSynchronizer(rx_decim, rx_chain["cic"].factor, o_domain=adc_clk)

        with m.If(rx_decim_stb):
            with m.If(rx_decim_new < 2):
                m.d.sync += rx_decim.eq(2)
            with m.Else():
                m.d.sync += rx_decim.eq(rx_decim_new)        

        # NCO control.
        rx_nco_ad = Signal(8)
        m.submodules.nco_phase_cdc = cdc.FFSynchronizer(rx_nco, rx_nco_ad, o_domain=adc_clk)
        m.d[adc_clk] += nco.phase.eq(nco.phase + (rx_nco_ad << 8))

        return m


if __name__ == "__main__":
    plat = PralinePlatform()
    plat.build(Top())
