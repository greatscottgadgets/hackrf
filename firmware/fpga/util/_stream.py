#
# This file is part of HackRF.
#
# Copyright (c) 2025 Great Scott Gadgets <info@greatscottgadgets.com>
# SPDX-License-Identifier: BSD-3-Clause

import unittest

from amaranth               import Module, Mux, Signal, Cat
from amaranth.lib           import wiring, stream, data
from amaranth.lib.wiring    import In, Out
from amaranth.sim           import Simulator


class StreamSkidBuffer(wiring.Component):

    def __init__(self, shape, always_ready=False):
        super().__init__({
            "input":  In(stream.Signature(shape, always_ready=always_ready)),
            "output": Out(stream.Signature(shape, always_ready=always_ready)),
        })
    
    def elaborate(self, platform):
        m = Module()

        # To provide for the "elasticity" needed due to a registered "ready" signal, we need 
        # two registers for the payload. When the consumer is not ready, there's a cycle 
        # where the data from the producer is stored in r_payload.
        # Read https://www.itdev.co.uk/blog/pipelining-axi-buses-registered-ready-signals

        r_payload   = Signal.like(self.input.payload, reset_less=True)
        r_valid     = Signal()

        with m.If(self.input.ready):
            m.d.sync += r_valid.eq(self.input.valid)
            m.d.sync += r_payload.eq(self.input.payload)

        # r_valid can only be asserted when there is incoming data but the consumer is not ready.
        with m.If(self.output.ready):
            m.d.sync += r_valid.eq(0)

        if not self.input.signature.always_ready:
            m.d.comb += self.input.ready.eq(~r_valid)
        m.d.comb += self.output.valid.eq(self.input.valid | r_valid)
        m.d.comb += self.output.p.eq(Mux(r_valid, r_payload, self.input.p))

        return m


class StreamMux(wiring.Component):

    def __init__(self, data_shape, num_channels, always_ready=False):
        self.num_channels = num_channels
        super().__init__({
            "input":  In(stream.Signature(
                data.ArrayLayout(data_shape, num_channels),
                always_ready=always_ready
            )),
            "output":  Out(stream.Signature(
                data.ArrayLayout(data_shape, 1),
                always_ready=always_ready
            )),
        })

    def elaborate(self, platform):
        m = Module()

        ratio = self.num_channels
        counter = Signal(range(ratio))

        sreg = Signal.like(self.input.p)
        m.d.comb += self.output.payload.eq(sreg[0])

        with m.If(self.output.ready & self.output.valid):
            m.d.sync += counter.eq(counter + 1)
            for i in range(ratio-1):
                m.d.sync += sreg[i].eq(sreg[i+1])

        with m.If(~self.output.valid | (self.output.ready & (counter == ratio-1))):
            m.d.sync += self.output.valid.eq(self.input.valid)
            m.d.sync += sreg.eq(self.input.payload)
            if not self.input.signature.always_ready:
                m.d.comb += self.input.ready.eq(1)

        return m


class StreamDemux(wiring.Component):

    def __init__(self, data_shape, num_channels, always_ready=False):
        self.num_channels = num_channels
        super().__init__({
            "input":  In(stream.Signature(
                data.ArrayLayout(data_shape, 1),
                always_ready=always_ready
            )),
            "output": Out(stream.Signature(
                data.ArrayLayout(data_shape, num_channels),
                always_ready=always_ready
            )),
        })

    def elaborate(self, platform):
        m = Module()

        ratio = self.num_channels
        counter = Signal(range(ratio))

        with m.If(~self.output.valid | self.output.ready):
            m.d.sync += self.output.valid.eq(self.input.valid & (counter == ratio-1))
            if not self.input.signature.always_ready:
                m.d.comb += self.input.ready.eq(1)
            with m.If(self.input.valid):

                m.d.sync += self.output.p[ratio-1].eq(self.input.p[0])
                for i in range(ratio-1):
                    m.d.sync += self.output.p[i].eq(self.output.p[i+1])

                # TODO: if I remove the following line timing is much worse. Study why.
                m.d.sync += self.output.p.eq(Cat(self.output.p[len(self.input.p):], self.input.p))
                m.d.sync += counter.eq(counter + 1)

        return m




class TestStreamMux(unittest.TestCase):

    def test_mux(self):
        dut = StreamMux(data_shape=8, num_channels=2)
        input_stream = [[0xAA, 0xBB], [0xCC, 0xDD]]
        output_stream = []
        output_len = 4

        async def stream_input(ctx):
            for sample in input_stream:
                ctx.set(dut.input.payload, sample)
                ctx.set(dut.input.valid, 1)
                await ctx.tick().until(dut.input.ready)
            ctx.set(dut.input.valid, 0)

        async def stream_output(ctx):
            ctx.set(dut.output.ready, 1)
            while len(output_stream) < output_len:
                await ctx.tick()
                if ctx.get(dut.output.valid):
                    output_stream.append(ctx.get(dut.output.payload))

        sim = Simulator(dut)
        sim.add_clock(1e-6)
        sim.add_testbench(stream_input)
        sim.add_testbench(stream_output)
        sim.run()

        self.assertListEqual(output_stream, [[0xAA], [0xBB], [0xCC], [0xDD]])


    def test_demux(self):
        dut = StreamDemux(data_shape=8, num_channels=2)
        input_stream = [[0xAA], [0xBB], [0xCC], [0xDD]]
        output_stream = []
        output_len = 2

        async def stream_input(ctx):
            for sample in input_stream:
                ctx.set(dut.input.payload, sample)
                ctx.set(dut.input.valid, 1)
                await ctx.tick().until(dut.input.ready)
            ctx.set(dut.input.valid, 0)

        async def stream_output(ctx):
            ctx.set(dut.output.ready, 1)
            while len(output_stream) < output_len:
                await ctx.tick()
                if ctx.get(dut.output.valid):
                    output_stream.append(ctx.get(dut.output.payload))

        sim = Simulator(dut)
        sim.add_clock(1e-6)
        sim.add_testbench(stream_input)
        sim.add_testbench(stream_output)
        sim.run()

        self.assertListEqual(output_stream, [[0xAA, 0xBB], [0xCC, 0xDD]])


if __name__ == '__main__':
    unittest.main()