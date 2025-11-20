#
# This file is part of HackRF.
#
# Copyright (c) 2025 Great Scott Gadgets <info@greatscottgadgets.com>
# SPDX-License-Identifier: BSD-3-Clause

from amaranth               import Module, Signal, Mux
from amaranth.lib           import wiring, stream
from amaranth.lib.wiring    import In, Out

from util                   import IQSample

class QuarterShift(wiring.Component):
    input:  In(stream.Signature(IQSample(8), always_ready=True))
    output: Out(stream.Signature(IQSample(8), always_ready=True))
    enable: In(1)
    up:     In(1)

    def elaborate(self, platform):
        m = Module()

        index = Signal(range(4))

        with m.If(~self.output.valid | self.output.ready):
            if not self.input.signature.always_ready:
                m.d.comb += self.input.ready.eq(1)
            m.d.sync += self.output.valid.eq(self.input.valid)
            with m.If(self.input.valid):
                # Select direction of shift with the `up` signal.
                with m.If(self.up):
                    m.d.sync += index.eq(index - 1)
                with m.Else():
                    m.d.sync += index.eq(index + 1)

                # Generate control signals derived from `index`.
                swap  = index[0]
                inv_q = index[0] ^ index[1]
                inv_i = index[1]

                # First stage: swap.
                i = Mux(swap, self.input.p.q, self.input.p.i)
                q = Mux(swap, self.input.p.i, self.input.p.q)

                # Second stage: sign inversion.
                i = Mux(inv_i, -i, i)
                q = Mux(inv_q, -q, q)

                with m.If(self.enable):
                    m.d.sync += self.output.p.i.eq(i)
                    m.d.sync += self.output.p.q.eq(q)
                with m.Else():
                    m.d.sync += self.output.p.i.eq(self.input.p.i)
                    m.d.sync += self.output.p.q.eq(self.input.p.q)

        return m