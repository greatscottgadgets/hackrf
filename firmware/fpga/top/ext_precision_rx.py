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
from dsp.fir_mac16          import HalfBandDecimatorMAC16
from dsp.cic                import CICDecimator
from dsp.dc_block           import DCBlock
from dsp.quarter_shift      import QuarterShift
from util                   import ClockConverter, IQSample


class Top(Elaboratable):

    def elaborate(self, platform):
        m = Module()

        m.submodules.clkgen = ClockDomainGenerator()

        # Submodules.
        m.submodules.adcdac_intf = adcdac_intf = MAX586xInterface(bb_domain="gck1")
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

        rx_chain = {
            # DC block and quarter shift.
            "dc_block":      DCBlock(width=8, num_channels=2, domain="gck1"),
            "quarter_shift": DomainRenamer("gck1")(QuarterShift()),

            # CIC mandatory first stage with compensator.
            "cic":          CICDecimator(2, 4, (4,8,16,32), width_in=8, width_out=12, num_channels=2, always_ready=True, domain="gck1"),
            "cic_comp":     DomainRenamer("gck1")(FIRFilter([-0.125, 0, 0.75, 0, -0.125], shape=fixed.SQ(11), shape_out=fixed.SQ(11), always_ready=True, num_channels=2)),

            # Final half-band decimator stages.
            "hbfir1":       HalfBandDecimatorMAC16(taps_hb1, data_shape=fixed.SQ(11), overclock_rate=4, always_ready=True, domain="gck1"),
            "hbfir2":       HalfBandDecimatorMAC16(taps_hb2, data_shape=fixed.SQ(11), overclock_rate=8, always_ready=True, domain="gck1"),

            # Clock domain conversion.
            "clkconv":      ClockConverter(IQSample(12), 8, "gck1", "sync", always_ready=True),
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
        ctrl     = spi_regs.add_register(0x01, init=0)
        rx_decim = spi_regs.add_register(0x02, init=0, size=3)
        #tx_intrp = spi_regs.add_register(0x04, init=0, size=3)

        m.d.comb += [
            # Trigger enable.
            mcu_intf.trigger_en                 .eq(ctrl[7]),

            # RX settings.
            rx_chain["dc_block"].enable         .eq(ctrl[0]),
            rx_chain["quarter_shift"].enable    .eq(ctrl[1]),
            rx_chain["quarter_shift"].up        .eq(ctrl[2]),

            # RX decimation rate.
            rx_chain["cic"].factor              .eq(rx_decim+2),
        ]

        return m


if __name__ == "__main__":
    plat = PralinePlatform()
    plat.build(Top())
