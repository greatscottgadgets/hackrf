#
# This file is part of HackRF.
#
# Copyright (c) 2025 Great Scott Gadgets <info@greatscottgadgets.com>
# SPDX-License-Identifier: BSD-3-Clause

from math                   import pi, sin, cos

from amaranth               import Module, Signal, Mux, Cat
from amaranth.lib           import wiring, memory
from amaranth.lib.wiring    import In, Out

from util                   import IQSample


class NCO(wiring.Component):
    """
    Retrieve cos(x), sin(x) using a look-up table.
    Latency is 2 cycles.
    
    We only precompute 1/8 of the (cos,sin) cycle, and the top 3 bits of the 
    phase are used to reconstruct the final values with symmetric properties.
    
    Parameters
    ----------
    phase_width : int
        Bit width of the phase accumulator.
    output_width : int
        Bit width of the output cos/sin words.

    Signals
    -------
    phase : Signal(phase_width), in
        Input phase.
    en : Signal(1), in
        Enable strobe.
    output : IQSample(output_width), out
        Returned result for cos(phase), sin(phase).
    """

    def __init__(self, phase_width=24, output_width=10):
        self.phase_width  = phase_width
        self.output_width = output_width
        super().__init__({
            "phase":  In(phase_width),
            "en":     In(1),
            "output": Out(IQSample(output_width)),
        })

    def elaborate(self, platform):
        m = Module()

        # Create internal table with precomputed entries.
        addr_width = (self.output_width + 1) - 3
        lut_depth  = 1 << addr_width
        lut_scale  = (1 << (self.output_width-1)) - 1
        lut_phases = [ i * pi / 4 / lut_depth + .5 for i in range(lut_depth) ]
        lut_data = memory.MemoryData(
            shape=IQSample(self.output_width),
            depth=lut_depth,
            init=({"i": round(lut_scale * cos(x)), "q": round(lut_scale * sin(x))} for x in lut_phases)
        )
        m.submodules.table = table = memory.Memory(data=lut_data)
        table_rd = table.read_port(domain="sync")

        # 3 MSBs of the phase word: sign, quadrant, octant.
        o, q, s = self.phase[-3:]
        rev_addr = o
        swap    = Signal()
        neg_cos = Signal()
        neg_sin = Signal()
        with m.If(self.en):
            m.d.sync += [
                swap    .eq(q ^ o),
                neg_cos .eq(s ^ q),
                neg_sin .eq(s),
            ]

        # Map phase to the [0,pi/4) range.
        octant_phase = Signal(addr_width)
        octant_mask  = rev_addr.replicate(len(octant_phase))  # reverse mask
        m.d.comb += octant_phase.eq(octant_mask ^ self.phase[-addr_width-3:-3])

        # Retrieve precomputed (cos, sin) values from the reduced range.
        e_s0 = Signal(IQSample(self.output_width))
        m.d.comb += [
            table_rd.addr.eq(octant_phase),
            table_rd.en  .eq(self.en),
            e_s0         .eq(table_rd.data),
        ]
        
        # Unmap the phase to its original octant.
        e_s1 = Signal.like(e_s0)
        e_s2 = Signal.like(e_s1)
        
        m.d.comb += [
            Cat(e_s1.i, e_s1.q) .eq(Mux(swap, Cat(e_s0.q, e_s0.i), e_s0)),
            e_s2.i              .eq(Mux(neg_cos, -e_s1.i, e_s1.i)),
            e_s2.q              .eq(Mux(neg_sin, -e_s1.q, e_s1.q)),
        ]
        m.d.sync += self.output.eq(e_s2)
        
        return m
