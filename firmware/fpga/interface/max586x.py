# This file is part of HackRF.
#
# Copyright (c) 2025 Great Scott Gadgets <info@greatscottgadgets.com>
# SPDX-License-Identifier: BSD-3-Clause

from amaranth               import Module, Signal, C, Cat
from amaranth.lib           import io, stream, wiring
from amaranth.lib.wiring    import Out, In

from util                   import IQSample

class MAX586xInterface(wiring.Component):
    adc_stream: Out(stream.Signature(IQSample(8), always_ready=True))
    dac_stream: In(stream.Signature(IQSample(8), always_ready=True))

    adc_capture: In(1)
    dac_capture: In(1)
    q_invert:    In(1)

    def __init__(self, bb_domain):
        super().__init__()
        self._bb_domain = bb_domain

    def elaborate(self, platform):
        m = Module()
        adc_stream = self.adc_stream
        dac_stream = self.dac_stream

        # Generate masks for inverting the Q component based on the q_invert signal.
        q_invert = Signal()
        rx_q_mask = Signal(8)
        tx_q_mask = Signal(10)
        m.d[self._bb_domain] += q_invert.eq(self.q_invert)
        with m.If(q_invert):
            m.d.comb += [
                rx_q_mask.eq(0x80),
                tx_q_mask.eq(0x1FF),
            ]
        with m.Else():
            m.d.comb += [
                rx_q_mask.eq(0x7F),
                tx_q_mask.eq(0x200),
            ]

        # Capture the ADC signals using a DDR input buffer.
        m.submodules.adc_in = adc_in = io.DDRBuffer("i", platform.request("da", dir="-"), i_domain=self._bb_domain)
        m.d.comb += [
            adc_stream.p.i      .eq(adc_in.i[0] ^ 0x80),       # I: non-inverted between MAX2837 and MAX5864.
            adc_stream.p.q      .eq(adc_in.i[1] ^ rx_q_mask),  # Q: inverted between MAX2837 and MAX5864.
            adc_stream.valid    .eq(self.adc_capture),
        ]

        # Output the transformed data to the DAC using a DDR output buffer.
        m.submodules.dac_out = dac_out = io.DDRBuffer("o", platform.request("dd", dir="-"), o_domain=self._bb_domain)
        with m.If(dac_stream.valid):
            m.d.comb += [
                dac_out.o[0] .eq(Cat(C(0, 2), dac_stream.p.i) ^ 0x200),
                dac_out.o[1] .eq(Cat(C(0, 2), dac_stream.p.q) ^ tx_q_mask),
            ]
        with m.Else():
            m.d.comb += [
                dac_out.o[0] .eq(0x200),
                dac_out.o[1] .eq(0x200),
            ]

        return m