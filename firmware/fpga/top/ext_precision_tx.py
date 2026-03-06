#
# This file is part of HackRF.
#
# Copyright (c) 2025 Great Scott Gadgets <info@greatscottgadgets.com>
# SPDX-License-Identifier: BSD-3-Clause

from amaranth               import Elaboratable, Module, Cat, DomainRenamer
from amaranth.lib.wiring    import connect

from amaranth_future        import fixed

from board                  import PralinePlatform, ClockDomainGenerator
from interface              import MAX586xInterface, SGPIOInterface, SPIRegisterInterface
from dsp.fir                import FIRFilter
from dsp.fir_mac16          import HalfBandInterpolatorMAC16
from dsp.cic                import CICInterpolator
from util                   import ClockConverter, IQSample, StreamSkidBuffer


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

        taps_hb2 = [3, 0, -16, 0, 77, 128, 77, 0, -16, 0, 3]
        taps_hb2 = [ tap/256 for tap in taps_hb2 ]

        tx_chain = {
            # Clock domain conversion.
            "clkconv":          ClockConverter(IQSample(12), 8, "sync", dac_clk, always_ready=False),

            # Half-band interpolation stages (+ skid buffers for timing closure).
            "hbfir1":           HalfBandInterpolatorMAC16(taps_hb1, data_shape=fixed.SQ(11),
                overclock_rate=8, num_channels=2, always_ready=False, domain=dac_clk),
            "skid1":            DomainRenamer(dac_clk)(StreamSkidBuffer(IQSample(12), always_ready=False)),
            "hbfir2":           HalfBandInterpolatorMAC16(taps_hb2, data_shape=fixed.SQ(11),
                overclock_rate=4, num_channels=2, always_ready=False, domain=dac_clk),
            "skid2":            DomainRenamer(dac_clk)(StreamSkidBuffer(IQSample(12), always_ready=False)),

            # CIC interpolation stage.
            "cic_comp":         DomainRenamer(dac_clk)(FIRFilter([-0.125, 0, 0.75, 0, -0.125], shape=fixed.SQ(11), shape_out=fixed.SQ(11), always_ready=False, num_channels=2)),
            "cic_interpolator": CICInterpolator(2, 4, (4, 8, 16, 32), 12, 8, num_channels=2, 
                always_ready=False, domain=dac_clk),
        }
        for k,v in tx_chain.items():
            m.submodules[f"tx_{k}"] = v

        # Connect transmitter chain.
        last = mcu_intf.dac_stream
        for block in tx_chain.values():
            connect(m, last, block.input)
            last = block.output
        connect(m, last, adcdac_intf.dac_stream)


        # SPI register interface.
        spi_port = platform.request("spi")
        m.submodules.spi_regs = spi_regs = SPIRegisterInterface(spi_port)

        # Add control registers.
        ctrl     = spi_regs.add_register(0x01, init=0)
        tx_intrp = spi_regs.add_register(0x02, init=0, size=3)

        m.d.comb += [
            # Trigger enable.
            mcu_intf.trigger_en                 .eq(ctrl[7]),

            # TX interpolation rate.
            tx_chain["cic_interpolator"].factor .eq(tx_intrp + 2),
        ]

        return m


if __name__ == "__main__":
    plat = PralinePlatform()
    plat.build(Top())
