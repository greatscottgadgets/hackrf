#
# This file is part of HackRF.
#
# Copyright (c) 2025 Great Scott Gadgets <info@greatscottgadgets.com>
# SPDX-License-Identifier: BSD-3-Clause

from collections            import defaultdict

from amaranth               import Module, Signal, signed
from amaranth.lib           import wiring, stream, data
from amaranth.lib.wiring    import In, Out
from amaranth.utils         import bits_for


class ShiftAddMCM(wiring.Component):
    def __init__(self, width, terms, num_channels=1, always_ready=False):
        self.terms = terms
        self.width = width
        self.num_channels = num_channels
        super().__init__({
            "input":  In(stream.Signature(
                data.ArrayLayout(signed(width), num_channels), 
                always_ready=always_ready)),
            "output": Out(stream.Signature(
                data.ArrayLayout(
                    data.StructLayout({
                        f"{i}": signed(width + bits_for(term)) for i, term in enumerate(terms)
                    }), num_channels), always_ready=always_ready)),
        })

    def elaborate(self, platform):
        m = Module()

        # Get unique, odd terms.
        terms = self.terms
        unique_terms = defaultdict(list)
        for i, term in enumerate(terms):
            if term == 0:
                continue
            term_odd, shift = make_odd(term)
            unique_terms[term_odd] += [(i, shift)]

        # Negated inputs for CSD.
        input_neg = Signal.like(self.input.p)
        for c in range(self.num_channels):
            m.d.comb += input_neg[c].eq(-self.input.p[c])

        with m.If(~self.output.valid | self.output.ready):
            if not self.input.signature.always_ready:
                m.d.comb += self.input.ready.eq(1)
            m.d.sync += self.output.valid.eq(self.input.valid)

        for term, outputs in unique_terms.items():

            term_csd = to_csd(term)

            for c in range(self.num_channels):

                n = self.input.p[c]
                n_neg = input_neg[c]

                result = None
                for s, t in enumerate(term_csd):
                    if t == 0:
                        continue
                    n_base = n if t == 1 else n_neg
                    shifted_n = n_base if s == 0 else (n_base << s)
                    if result is None:
                        result = shifted_n
                    else:
                        result += shifted_n

                # A single register can feed multiple outputs.
                result_q = Signal(signed(self.width+bits_for(term-1)), name=f"mul_{term}_{c}")
                with m.If(self.input.ready & self.input.valid):
                    m.d.sync += result_q.eq(result)

                for out_index, shift in outputs:
                    m.d.comb += self.output.p[c][f"{out_index}"].eq(result_q if shift == 0 else (result_q << shift))

        return m


def make_odd(n):
    """Convert number to odd fundamental by right-shifting. Returns (odd_part, shift_amount)"""
    if n == 0:
        return 0, 0
    
    shift = 0
    while n % 2 == 0:
        n = n >> 1
        shift += 1
    
    return n, shift


def multiply(n, k):
    if k == 0:
        return 0

    csd_k = to_csd(k)

    result = None
    for i, c in enumerate(csd_k):
        if c == 0:
            continue
        shifted_n = n if i == 0 else (n << i)
        if result is None:
            if c == 1:
                result = shifted_n
            elif c == -1:
                result = -shifted_n
        else:
            if c == 1:
                result += shifted_n
            elif c == -1:
                result -= shifted_n
    
    return result[:bits_for(k-1)+len(n)].as_signed()


def to_csd(n):
    """ Convert integer to Canonical Signed Digit representation (LSB first). """
    if n == 0:
        return [0]
    
    sign = n < 0
    n = abs(n)
    binary = [ int(b) for b in f"{n:b}" ][::-1]

    # Apply CSD conversion algorithm.
    binary_padded = binary + [0]
    carry = 0
    csd = []
    for i, bit in enumerate(binary_padded):
        nextbit = binary_padded[i+1] if i+1 < len(binary_padded) else 0
        d = bit ^ carry
        ys = nextbit & d  # sign bit
        yd = ~nextbit & d  # data bit
        csd.append(yd - ys)
        carry = (bit & nextbit) | ((bit|nextbit)&carry)
    if sign:
        csd = [-1*c for c in csd]

    # Remove trailing zeros.
    while len(csd) > 1 and csd[-1] == 0:
        csd.pop()

    # Regular binary representation is preferred if the number
    # of additions was not improved.
    if sum(binary) <= sum(abs(d) for d in csd) - sign:
        if sign:
            return [ -d for d in binary ]
        return binary

    return csd
