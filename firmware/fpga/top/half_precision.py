#!/usr/bin/env python3
#
# This file is part of HackRF.
#
# Copyright (c) 2024 Great Scott Gadgets <info@greatscottgadgets.com>
# SPDX-License-Identifier: BSD-3-Clause

from amaranth               import Elaboratable, Module, DomainRenamer
from amaranth.lib           import stream, wiring
from amaranth.lib.wiring    import Out, In, connect

from board                  import PralinePlatform, ClockDomainGenerator
from interface              import MAX586xInterface, SGPIOInterface, SPIRegisterInterface
from dsp.dc_block           import DCBlock
from dsp.round              import convergent_round
from util                   import IQSample, ClockConverter


class IQHalfPrecisionConverter(wiring.Component):
    input:  In(stream.Signature(IQSample(8), always_ready=True))
    output: Out(stream.Signature(IQSample(4), always_ready=True))

    def elaborate(self, platform):
        m = Module()
        
        m.d.comb += [
            self.output.p.i     .eq(convergent_round(self.input.p.i, 4)),
            self.output.p.q     .eq(convergent_round(self.input.p.q, 4)),
            self.output.valid   .eq(self.input.valid),
        ]

        return m

class IQHalfPrecisionConverterInv(wiring.Component):
    input:  In(stream.Signature(IQSample(4)))
    output: Out(stream.Signature(IQSample(8)))

    def elaborate(self, platform):
        m = Module()
        
        m.d.comb += [
            self.output.p.i     .eq(self.input.p.i << 4),
            self.output.p.q     .eq(self.input.p.q << 4),
            self.output.valid   .eq(self.input.valid),
            self.input.ready    .eq(self.output.ready),
        ]

        return m


class Top(Elaboratable):

    def elaborate(self, platform):
        m = Module()

        m.submodules.clkgen = ClockDomainGenerator()

        # Submodules.
        m.submodules.adcdac_intf = adcdac_intf = MAX586xInterface(bb_domain="gck1")
        m.submodules.mcu_intf    = mcu_intf    = SGPIOInterface(sample_width=8, domain="sync")

        m.d.comb += adcdac_intf.q_invert.eq(platform.request("q_invert").i)

        rx_chain = {
            "dc_block":      DCBlock(width=8, num_channels=2, domain="gck1"),
            "half_prec":     DomainRenamer("gck1")(IQHalfPrecisionConverter()),
            "clkconv":       ClockConverter(IQSample(4), 16, "gck1", "sync"),
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
            "clkconv":       ClockConverter(IQSample(4), 16, "sync", "gck1", always_ready=False),
            "half_prec":     DomainRenamer("gck1")(IQHalfPrecisionConverterInv()),
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
        ctrl  = spi_regs.add_register(0x01, init=0)
        m.d.comb += [
            # Trigger enable.
            mcu_intf.trigger_en                 .eq(ctrl[7]),

            # RX settings.
            rx_chain["dc_block"].enable         .eq(ctrl[0]),
        ]

        return m


if __name__ == "__main__":
    plat = PralinePlatform()
    plat.build(Top())
