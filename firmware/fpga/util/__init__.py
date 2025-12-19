#
# This file is part of HackRF.
#
# Copyright (c) 2025 Great Scott Gadgets <info@greatscottgadgets.com>
# SPDX-License-Identifier: BSD-3-Clause

from amaranth               import Module, signed, Shape
from amaranth.lib           import wiring, stream, data, fifo
from amaranth.lib.wiring    import In, Out

from ._stream               import StreamSkidBuffer, StreamMux, StreamDemux
from .lfsr                  import LinearFeedbackShiftRegister


class IQSample(data.StructLayout):
    def __init__(self, width=8):
        super().__init__({
            "i": signed(width),
            "q": signed(width),
        })


class ClockConverter(wiring.Component):

    def __init__(self, shape, depth, input_domain, output_domain, always_ready=True):
        super().__init__({
            "input":  In(stream.Signature(shape, always_ready=always_ready)),
            "output": Out(stream.Signature(shape, always_ready=always_ready)),
        })
        self.shape = shape
        self.depth = depth
        self._input_domain = input_domain
        self._output_domain = output_domain

    def elaborate(self, platform):
        m = Module()

        m.submodules.mem = mem = fifo.AsyncFIFOBuffered(
            width=Shape.cast(self.shape).width,
            depth=self.depth,
            r_domain=self._output_domain,
            w_domain=self._input_domain)

        m.d.comb += [
            # write.
            mem.w_data          .eq(self.input.p),
            mem.w_en            .eq(self.input.valid),
            # read.
            self.output.p       .eq(mem.r_data),
            self.output.valid   .eq(mem.r_rdy),
            mem.r_en            .eq(self.output.ready),
        ]
        if not self.input.signature.always_ready:
            m.d.comb += self.input.ready.eq(mem.w_rdy)

        return m

