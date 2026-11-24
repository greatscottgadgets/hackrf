# This file is part of HackRF.
#
# Copyright (c) 2025 Great Scott Gadgets <info@greatscottgadgets.com>
# SPDX-License-Identifier: BSD-3-Clause

from amaranth               import Module, Signal, C, Cat, Mux
from amaranth.lib           import io, stream, wiring
from amaranth.lib.wiring    import Out, In

from util                   import IQSample


class MAX586xInterface(wiring.Component):
    adc_stream: Out(stream.Signature(IQSample(8), always_ready=True, always_valid=True))
    dac_stream: In(stream.Signature(IQSample(8), always_ready=True))
    q_invert:   In(1)

    def __init__(self, adc_domain, dac_domain, adc_xform=(lambda m,x: x)):
        super().__init__()
        self._adc_domain = adc_domain
        self._dac_domain = dac_domain
        self.adc_xform = adc_xform

    def elaborate(self, platform):
        m = Module()
        adc_stream = self.adc_stream
        dac_stream = self.dac_stream

        # Generate masks for inverting the Q component based on the q_invert signal.
        q_invert_rx = Signal()
        q_invert_tx = Signal()
        rx_q_mask = Signal(8)
        tx_q_mask = Signal(10)
        m.d[self._adc_domain] += q_invert_rx.eq(self.q_invert)
        m.d[self._dac_domain] += q_invert_tx.eq(self.q_invert)

        m.d.comb += [
            rx_q_mask.eq(Mux(q_invert_rx, 0x80, 0x7F)),
            tx_q_mask.eq(Mux(q_invert_tx, 0x1FF, 0x200)),
        ]

        # Capture the ADC signals using a DDR input buffer.
        m.submodules.adc_in = _adc_in = io.DDRBuffer("i", platform.request("da", dir="-"), i_domain=self._adc_domain)
        adc_in = self.adc_xform(m, _adc_in.i)
        m.d.comb += [
            adc_stream.p.i      .eq(adc_in[0] ^ 0x80),       # I: non-inverted between MAX2837 and MAX5864.
            adc_stream.p.q      .eq(adc_in[1] ^ rx_q_mask),  # Q: inverted between MAX2837 and MAX5864.
        ]

        # Output to the DAC using a DDR output buffer.
        m.submodules.dac_out = dac_out = io.DDRBuffer("o", platform.request("dd", dir="-"), o_domain=self._dac_domain)
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