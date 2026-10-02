#
# This file is part of HackRF.
#
# Copyright (c) 2025 Great Scott Gadgets <info@greatscottgadgets.com>
# SPDX-License-Identifier: BSD-3-Clause

from amaranth               import Module, Signal, Mux, DomainRenamer, signed, unsigned
from amaranth.lib           import wiring, stream, data, memory, fifo
from amaranth.lib.wiring    import In, Out
from amaranth.utils         import ceil_log2, bits_for

from amaranth_future        import fixed

from dsp.mcm                import ShiftAddMCM


class HalfBandDecimator(wiring.Component):
    def __init__(self, taps, data_shape, shape_out=None, always_ready=False, domain="sync"):
        midtap = taps[len(taps)//2]
        assert taps[1::2] == [0]*(len(taps)//4) + [midtap] + [0]*(len(taps)//4)
        assert midtap == 0.5
        self.taps = taps
        self.data_shape = data_shape
        self.full_shape = FIRFilter.compute_output_shape(data_shape, taps)
        if shape_out is None:
            shape_out = self.full_shape
        self.shape_out = shape_out
        self.always_ready = always_ready
        self._domain = domain
        super().__init__({
            "input":  In(stream.Signature(
                data.ArrayLayout(data_shape, 2),
                always_ready=always_ready
            )),
            "output": Out(stream.Signature(
                data.ArrayLayout(shape_out, 2),
                always_ready=always_ready
            )),
            "enable": In(1),
        })

    @staticmethod
    def interleave_with_zeros(seq, factor):
        out = []
        for n in seq:
            out.append(n)
            out.extend([0]*factor)
        return out[:-factor]

    def elaborate(self, platform):
        m = Module()        

        always_ready = self.always_ready
        taps     = [ 2 * tap for tap in self.taps ]  # scale by 0.5 at the output
        fir_taps = self.interleave_with_zeros(taps[0::2], 1)

        # Arms
        fir_shape_out = (self.output.p[0] << 1).shape()
        m.submodules.fir = fir = FIRFilter(fir_taps, shape=self.data_shape, shape_out=fir_shape_out,
            always_ready=always_ready, num_channels=1, add_tap=len(fir_taps)//2+1)
        fir_out_odd = Signal()
        with m.If(fir.output.valid & fir.output.ready):
            m.d.sync += fir_out_odd.eq(~fir_out_odd)

        odd = Signal()
        with m.If(self.input.valid & self.input.ready):
            m.d.sync += odd.eq(~odd)

        # Only switch modes at even samples.
        switch_stb = Signal()
        m.d.comb += switch_stb.eq((~odd) ^ (self.input.valid & self.input.ready))

        with m.FSM():

            with m.State("BYPASS"):

                with m.If(~self.output.valid | self.output.ready):
                    m.d.sync += self.output.valid.eq(self.input.valid)
                    m.d.sync += self.output.payload.eq(self.input.payload)
                    if not self.input.signature.always_ready:
                        m.d.comb += self.input.ready.eq(1)

                with m.If(self.enable & switch_stb):
                    m.next = "DECIMATE"

            with m.State("DECIMATE"):

                # I and Q channels are muxed in time, demuxed later in the output stage.
                even_buffer = Signal.like(self.input.p, reset_less=True)
                q_value     = Signal.like(self.input.p[1], reset_less=True)
                q_valid     = Signal()

                if not self.input.signature.always_ready:
                    m.d.comb += self.input.ready.eq(fir.input.ready)

                with m.If(self.input.ready & self.input.valid):
                    with m.If(~odd):
                        m.d.sync += even_buffer.eq(self.input.p)
                    with m.Else():
                        m.d.sync += q_value.eq(self.input.p[1])
                        m.d.sync += q_valid.eq(1)

                with m.If(odd):
                    m.d.comb += [
                        fir.add_input   .eq(even_buffer[0]),
                        fir.input.p     .eq(self.input.p[0]),
                        fir.input.valid .eq(self.input.valid),
                    ]
                with m.Else():
                    m.d.comb += [
                        fir.add_input   .eq(even_buffer[1]),
                        fir.input.p     .eq(q_value),
                        fir.input.valid .eq(q_valid),
                    ]
                    with m.If(fir.input.ready):
                        m.d.sync += q_valid.eq(0)

                # Output sum and demux.
                with m.If(~self.output.valid | self.output.ready):
                    if not fir.output.signature.always_ready:
                        m.d.comb += fir.output.ready.eq(1)
                    m.d.sync += self.output.valid.eq(fir.output.valid & fir_out_odd)
                    with m.If(fir.output.valid):
                        m.d.sync += self.output.p[0].eq(self.output.p[1])
                        m.d.sync += self.output.p[1].eq(fir.output.p[0] >> 1)

                # Mode switch logic
                with m.If(~self.enable & switch_stb):
                    m.d.sync += even_buffer.eq(0)
                    m.d.sync += q_value.eq(0)
                    m.next = "BYPASS"

        if self._domain != "sync":
            m = DomainRenamer(self._domain)(m)

        return m


class HalfBandInterpolator(wiring.Component):
    def __init__(self, taps, data_shape, shape_out=None, always_ready=False, domain="sync"):
        midtap = taps[len(taps)//2]
        assert taps[1::2] == [0]*(len(taps)//4) + [midtap] + [0]*(len(taps)//4)
        assert midtap == 0.5
        self.taps = taps
        self.data_shape = data_shape
        self.full_shape = FIRFilter.compute_output_shape(data_shape, [ 2 * tap for tap in taps[0::2]])
        if shape_out is None:
            shape_out = self.full_shape
        self.shape_out = shape_out
        self.always_ready = always_ready
        self._domain = domain
        super().__init__({
            "input":  In(stream.Signature(
                data.ArrayLayout(data_shape, 2),
                always_ready=always_ready
            )),
            "output": Out(stream.Signature(
                data.ArrayLayout(shape_out, 2),
                always_ready=always_ready
            )),
            "enable": In(1),
        })

    def elaborate(self, platform):
        m = Module()        

        always_ready = self.always_ready

        taps      = [ 2 * tap for tap in self.taps ]
        arm0_taps = taps[0::2]
        arm1_taps = taps[1::2]
        delay     = arm1_taps.index(1)

        # Arms
        m.submodules.fir = fir = FIRFilter(arm0_taps, shape=self.data_shape, shape_out=self.shape_out, always_ready=always_ready, num_channels=2)
        m.submodules.dly = dly = Delay(delay, shape=self.data_shape, always_ready=always_ready, num_channels=2)
        m.submodules.dly_fifo = dly_fifo = fifo.SyncFIFOBuffered(width=2*self.data_shape.as_shape().width, depth=fir.delay-1)
        arms = [fir, dly]

        m.d.comb += [
            dly_fifo.w_data.eq(dly.output.p),
            dly_fifo.w_en.eq(dly.output.valid),
        ]
        if not dly.output.signature.always_ready:
            m.d.comb += dly.output.ready.eq(dly_fifo.w_rdy)

        with m.FSM():

            with m.State("BYPASS"):

                with m.If(~self.output.valid | self.output.ready):
                    m.d.sync += self.output.valid.eq(self.input.valid)
                    m.d.sync += self.output.p[0].eq(self.input.p[0])
                    m.d.sync += self.output.p[1].eq(self.input.p[1])
                    if not self.input.signature.always_ready:
                        m.d.comb += self.input.ready.eq(1)

                with m.If(self.enable):
                    m.next = "INTERPOLATE"

            with m.State("INTERPOLATE"):

                # Mode switch logic.
                with m.If(~self.enable):
                    m.next = "BYPASS"

                # Input
                for i, arm in enumerate(arms):
                    m.d.comb += arm.input.payload.eq(self.input.payload)
                    m.d.comb += arm.input.valid.eq(self.input.valid & arms[i^1].input.ready)
                if not self.input.signature.always_ready:
                    m.d.comb += self.input.ready.eq(arms[0].input.ready & arms[1].input.ready)

                # Output

                # Arm index selection: switch after every delivered sample
                arm_index = Signal()

                # Output buffers for each arm.
                r_data_cast = data.ArrayLayout(self.data_shape, 2)(dly_fifo.r_data)

                with m.If(~self.output.valid | self.output.ready):
                    with m.Switch(arm_index):
                        with m.Case(0):
                            m.d.sync += self.output.p[0].eq(fir.output.p[0])
                            m.d.sync += self.output.p[1].eq(fir.output.p[1])
                            m.d.sync += self.output.valid.eq(fir.output.valid)
                            if not fir.output.signature.always_ready:
                                m.d.comb += fir.output.ready.eq(1)
                            with m.If(fir.output.valid):
                                m.d.sync += arm_index.eq(1)
                        with m.Case(1):
                            m.d.sync += self.output.p[0].eq(r_data_cast[0])
                            m.d.sync += self.output.p[1].eq(r_data_cast[1])
                            m.d.sync += self.output.valid.eq(dly_fifo.r_rdy)
                            m.d.comb += dly_fifo.r_en.eq(1)
                            with m.If(dly_fifo.r_rdy):
                                m.d.sync += arm_index.eq(0)

        if self._domain != "sync":
            m = DomainRenamer(self._domain)(m)

        return m


class FIRFilter(wiring.Component):

    def __init__(self, taps, shape, shape_out=None, always_ready=False, num_channels=1, add_tap=None):
        self.taps = list(taps)
        self.add_tap = add_tap
        self.shape = shape
        self.full_shape = self.compute_output_shape(self.shape, self.taps, self.shape if self.add_tap else None)
        if shape_out is None:
            shape_out = self.full_shape
        self.saturate = self.full_shape.i_bits > shape_out.i_bits
        self.shape_out = shape_out
        self.num_channels = num_channels
        self.always_ready = always_ready

        sig = {
            "input":  In(stream.Signature(
                data.ArrayLayout(shape, num_channels),
                always_ready=always_ready
            )),
            "output": Out(stream.Signature(
                data.ArrayLayout(shape_out, num_channels),
                always_ready=always_ready
            ))
        }
        if add_tap is not None:
            sig |= {"add_input": In(data.ArrayLayout(shape, num_channels))}
        
        super().__init__(sig)

    @property
    def delay(self):
        d = 2  # base delay value
        if self.full_shape != self.shape_out:
            d += 1
        # saturation is done combinatorially, no additional delay
        return d

    @staticmethod
    def compute_output_shape(shape, taps, add_shape=None):
        # Compute output shape taking into account the actual coefficients.
        def max_raw_magnitude(shape):
            return max(
                abs(shape.min().as_integer_ratio()[0]),
                abs(shape.max().as_integer_ratio()[0])
            )

        _signed = shape.signed or any(t < 0 for t in taps)
        if add_shape is not None:
            _signed |= add_shape.signed
        base_shape = signed if _signed else unsigned

        taps_as_ratios = [
            fixed.Const(t).as_integer_ratio()
            for t in taps
            if t != 0
        ]
        max_denom = max(abs(d) for _, d in taps_as_ratios)
        f_bits = shape.f_bits + ceil_log2(max_denom)

        max_input = max_raw_magnitude(shape)
        taps_sum = sum(
            abs(n) * max_denom // d
            for n, d in taps_as_ratios
        )
        max_raw = max_input * taps_sum

        if add_shape is not None:
            max_add = max_raw_magnitude(add_shape)
            shift = f_bits - add_shape.f_bits
            if shift >= 0:
                max_raw += max_add << shift
            else:
                max_raw += max_add >> -shift
        
        t_bits = max(f_bits, bits_for(max_raw))
        return fixed.Shape(base_shape(t_bits + _signed), f_bits)

    @staticmethod
    def xform_stage(m, in_stream, xform, domain="sync"):

        # Create output stream.
        out_shape = xform(in_stream.p[0]).shape()
        num_channels = len(in_stream.p)
        out_stream = stream.Signature(
            data.ArrayLayout(out_shape, num_channels),
            always_ready=in_stream.signature.always_ready
        ).create()

        # Assign transformed stream.
        if domain == "comb":
            m.d.comb += out_stream.valid.eq(in_stream.valid)
            for c in range(num_channels):
                m.d.comb += out_stream.p[c].eq(xform(in_stream.p[c]))
            if not in_stream.signature.always_ready:
                m.d.comb += in_stream.ready.eq(out_stream.ready)
        else:
            with m.If(~out_stream.valid | out_stream.ready):
                if not in_stream.signature.always_ready:
                    m.d.comb += in_stream.ready.eq(1)
                m.d.sync += out_stream.valid.eq(in_stream.valid)
                for c in range(num_channels):
                    m.d.sync += out_stream.p[c].eq(xform(in_stream.p[c]))
        
        return out_stream

    def elaborate(self, platform):
        m = Module()

        # Implement transposed-form FIR because it needs a smaller number of registers.

        # Implement constant multipliers.
        nz_taps = [ t for t in self.taps if t != 0]
        m.submodules.mcm = mcm = ShiftAddMCM(
            self.shape.as_shape().width,
            [ fixed.Const(tap).as_integer_ratio()[0] for tap in nz_taps ],
            num_channels=self.num_channels,
            always_ready=self.always_ready)
        wiring.connect(m, wiring.flipped(self.input), mcm.input)

        # Cast outputs to fixed point values.
        muls = dict()
        for i, tap in enumerate(nz_taps):
            mul_shape = (fixed.Const(tap) * self.input.p[0]).shape()
            muls[tap] = [ mul_shape(mcm.output.p[c][f"{i}"]) for c in range(self.num_channels) ]

        # FIR output stream (full precision).
        fir_output = stream.Signature(
            data.ArrayLayout(self.full_shape, self.num_channels),
            always_ready=self.always_ready,
        ).create()
        with m.If(~fir_output.valid | fir_output.ready):
            if not self.always_ready:
                m.d.comb += mcm.output.ready.eq(1)
            m.d.sync += fir_output.valid.eq(mcm.output.valid)

        # Carry sum
        if self.add_tap is not None:
            add_input_q = Signal.like(self.add_input)
            # TODO: prepare to deal with MCM delay different to 1.
            with m.If(self.input.valid & self.input.ready):
                m.d.sync += add_input_q.eq(self.add_input)

        # Implement adder line.
        advance = Signal()
        m.d.comb += advance.eq(mcm.output.valid & mcm.output.ready)

        def _add(acc, val):
            return acc + val if acc is not None else val

        for c in range(self.num_channels):

            accum = None
            for i, tap in enumerate(self.taps[::-1]):
                
                value = accum
                if tap != 0:
                    value = _add(value, muls[tap][c])
                if i == self.add_tap:
                    value = _add(value, add_input_q[c])
                if value is None:
                    continue

                # Create a register with the smallest shape that can hold `value``.
                accum_shape = self.compute_output_shape(
                    self.shape,
                    self.taps[::-1][:i+1],
                    add_shape=(self.shape if self.add_tap is not None and i>=self.add_tap else None)
                )
                accum = Signal(accum_shape, name=f"add_{c}_{i}")

                with m.If(advance):
                    m.d.sync += accum.eq(value)

            m.d.comb += fir_output.payload[c].eq(accum)

        # Perform rounding and saturation when needed.
        last_output = fir_output
        if self.full_shape != self.shape_out:  # round
            last_output = self.xform_stage(m, last_output, lambda t: t.round(self.shape_out.f_bits, clip=not self.saturate))
        if self.saturate:
            last_output = self.xform_stage(m, last_output, lambda t: t.saturate(self.shape_out), "comb")

        wiring.connect(m, last_output, wiring.flipped(self.output))

        return m


class Delay(wiring.Component):
    def __init__(self, delay, shape, always_ready=False, num_channels=1):
        self.delay = delay
        self.shape = shape
        self.num_channels = num_channels

        super().__init__({
            "input":  In(stream.Signature(
                data.ArrayLayout(shape, num_channels),
                always_ready=always_ready
            )),
            "output": Out(stream.Signature(
                data.ArrayLayout(shape, num_channels),
                always_ready=always_ready
            )),
        })

    def elaborate(self, platform):
        if self.delay < 3:
            return self.elaborate_regs()
        return self.elaborate_memory()
    
    def elaborate_regs(self):
        m = Module()

        last = self.input.payload
        for i in range(self.delay + 1):
            reg = Signal.like(last, name=f"reg_{i}")
            with m.If(self.input.valid & self.input.ready):
                m.d.sync += reg.eq(last)
            last = reg
        m.d.comb += self.output.payload.eq(last)

        with m.If(self.output.ready | ~self.output.valid):
            if not self.input.signature.always_ready:
                m.d.comb += self.input.ready.eq(1)
            m.d.sync += self.output.valid.eq(self.input.valid)

        return m

    def elaborate_memory(self):
        m = Module()

        m.submodules.mem = mem = memory.Memory(
            shape=self.input.payload.shape(),
            depth=self.delay,
            init=()
        )
        mem_wr = mem.write_port(domain="sync")
        mem_rd = mem.read_port(domain="sync")

        addr = Signal.like(mem_wr.addr)
        with m.If(self.input.valid & self.input.ready):
            m.d.sync += addr.eq(Mux(addr == self.delay-1, 0, addr + 1))

        m.d.comb += [
            mem_wr.addr         .eq(addr),
            mem_rd.addr         .eq(addr),
            mem_wr.data         .eq(self.input.payload),
            mem_wr.en           .eq(self.input.valid & self.input.ready),
            mem_rd.en           .eq(self.input.valid & self.input.ready),
            self.output.payload .eq(mem_rd.data),
        ]

        with m.If(self.output.ready | ~self.output.valid):
            if not self.input.signature.always_ready:
                m.d.comb += self.input.ready.eq(1)
            m.d.sync += self.output.valid.eq(self.input.valid)

        return m

