#!/usr/bin/env python3
#
# This file is part of HackRF.
#
# Copyright (c) 2024 Great Scott Gadgets <info@greatscottgadgets.com>
# SPDX-License-Identifier: BSD-3-Clause

from amaranth               import Elaboratable, Module, Signal, C, Mux, Instance, Cat, ClockSignal, DomainRenamer, signed
from amaranth.lib           import io, stream, wiring, cdc, data, fifo
from amaranth.lib.wiring    import Out, In, connect

from board                  import PralinePlatform, ClockDomainGenerator
from interface              import MAX586xInterface
from interface.spi          import SPIRegisterInterface
from dsp.dc_block           import DCBlock
from dsp.round              import convergent_round
from util                   import IQSample, ClockConverter


class MCUInterface(wiring.Component):
    adc_stream: In(stream.Signature(IQSample(4), always_ready=True))
    dac_stream: Out(stream.Signature(IQSample(4)))
    direction:  In(1)
    enable:     In(1)
    
    def __init__(self, domain="sync"):
        self._domain = domain
        super().__init__()

    def elaborate(self, platform):
        m = Module()

        adc_stream = self.adc_stream
        dac_stream = self.dac_stream

        # Determine data transfer direction.
        direction = Signal()
        enable    = Signal()
        m.d.sync += enable.eq(self.enable)
        m.d.sync += direction.eq(self.direction)
        transfer_from_adc = (direction == 0)
        transfer_to_dac   = (direction == 1)

        # SGPIO clock and data lines.
        m.submodules.clk_out = clk_out = io.DDRBuffer("o", platform.request("host_clk", dir="-"), o_domain=self._domain)
        m.submodules.host_io = host_io = io.DDRBuffer('io', platform.request("host_data", dir="-"), i_domain=self._domain, o_domain=self._domain)

        # State machine to control SGPIO clock and data lines.
        m.d.sync += clk_out.o[0].eq(0)
        m.d.sync += clk_out.o[1].eq(0)
        m.d.sync += host_io.oe.eq(transfer_from_adc)

        data_to_host = Signal.like(Cat(adc_stream.p.i, adc_stream.p.q))
        assert len(data_to_host) == 8
        m.d.comb += host_io.o[0].eq(data_to_host)
        m.d.comb += host_io.o[1].eq(data_to_host)

        tx_dly_write = Signal(2)
        m.d.sync += tx_dly_write.eq(tx_dly_write << 1)
        m.d.comb += dac_stream.payload.eq(host_io.i[1])
        m.d.comb += dac_stream.valid.eq(tx_dly_write[-1])

        with m.FSM():
            with m.State("IDLE"):
                with m.If(enable):
                    with m.If(transfer_from_adc & adc_stream.valid):
                        m.d.sync += data_to_host.eq(Cat(adc_stream.p.i, adc_stream.p.q))
                        m.d.sync += clk_out.o[1].eq(1)

                    with m.Elif(transfer_to_dac & dac_stream.ready):
                        m.d.sync += clk_out.o[0].eq(1)
                        m.d.sync += tx_dly_write[0].eq(1)  # delayed write

        if self._domain != "sync":
            m = DomainRenamer(self._domain)(m)

        return m


class FlowAndTriggerControl(wiring.Component):
    trigger_en:  In(1)
    direction:   Out(1)  # async
    enable:      Out(1)  # async
    adc_capture: Out(1)
    dac_capture: Out(1)

    def __init__(self, domain):
        super().__init__()
        self._domain = domain

    def elaborate(self, platform):
        m = Module()

        #
        # Signal synchronization and trigger logic.
        #
        trigger_enable   =  self.trigger_en
        trigger_in       =  platform.request("trigger_in").i
        trigger_out      =  platform.request("trigger_out").o
        host_data_enable = ~platform.request("disable").i
        m.d.comb += trigger_out.eq(host_data_enable)

        # Create a latch for the trigger input signal using a FPGA primitive.
        trigger_in_latched = Signal()
        trigger_in_reg = Instance("SB_DFFES",
            i_D = 0,
            i_S = trigger_in,  # async set
            i_E = ~host_data_enable,
            i_C = ClockSignal(self._domain),
            o_Q = trigger_in_latched
        )
        m.submodules.trigger_in_reg = trigger_in_reg

        # Export signals for direction control and gating captures.
        m.d.comb += self.direction.eq(platform.request("direction").i)
        m.d.comb += self.enable.eq(host_data_enable)
        
        with m.If(host_data_enable):
            m.d[self._domain] += self.adc_capture.eq((trigger_in_latched | ~trigger_enable) & (self.direction == 0))
            m.d[self._domain] += self.dac_capture.eq((trigger_in_latched | ~trigger_enable) & (self.direction == 1))
        with m.Else():
            m.d[self._domain] += self.adc_capture.eq(0)
            m.d[self._domain] += self.dac_capture.eq(0)

        return m




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
        m.submodules.flow_ctl    = flow_ctl    = FlowAndTriggerControl(domain="gck1")
        m.submodules.adcdac_intf = adcdac_intf = MAX586xInterface(bb_domain="gck1")
        m.submodules.mcu_intf    = mcu_intf    = MCUInterface(domain="sync")

        m.d.comb += adcdac_intf.adc_capture.eq(flow_ctl.adc_capture)
        m.d.comb += adcdac_intf.dac_capture.eq(flow_ctl.dac_capture)
        m.d.comb += adcdac_intf.q_invert.eq(platform.request("q_invert").i)
        m.d.comb += mcu_intf.direction.eq(flow_ctl.direction)
        m.d.comb += mcu_intf.enable.eq(flow_ctl.enable)

        rx_chain = {
            "dc_block":      DCBlock(width=8, num_channels=2, domain="gck1"),
            "half_prec":     DomainRenamer("gck1")(IQHalfPrecisionConverter()),
            "clkconv":       ClockConverter(IQSample(4), 4, "gck1", "sync"),
        }
        m.submodules += rx_chain.values()

        # Connect receiver chain.
        last = adcdac_intf.adc_stream
        for block in rx_chain.values():
            connect(m, last, block.input)
            last = block.output
        connect(m, last, mcu_intf.adc_stream)

        
        tx_chain = {
            "clkconv":       ClockConverter(IQSample(4), 4, "sync", "gck1", always_ready=False),
            "half_prec":     DomainRenamer("gck1")(IQHalfPrecisionConverterInv()),
        }
        m.submodules += tx_chain.values()

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
            flow_ctl.trigger_en                 .eq(ctrl[7]),

            # RX settings.
            rx_chain["dc_block"].enable         .eq(ctrl[0]),
        ]

        return m


if __name__ == "__main__":
    plat = PralinePlatform()
    plat.build(Top_HP())
