#
# This file is part of HackRF.
#
# Copyright (c) 2025 Great Scott Gadgets <info@greatscottgadgets.com>
# SPDX-License-Identifier: BSD-3-Clause

from math                   import ceil, log2

from amaranth               import Module, Signal, Mux, Cat, DomainRenamer, ClockSignal, signed, unsigned
from amaranth.lib           import wiring, stream, data, memory, fifo
from amaranth.lib.wiring    import In, Out
from amaranth.utils         import ceil_log2

from amaranth_future        import fixed

from dsp.sb_mac16           import SB_MAC16
from dsp.fir                import FIRFilter, Delay


class HalfBandDecimatorMAC16(wiring.Component):
    def __init__(self, taps, data_shape, overclock_rate=4, shape_out=None, always_ready=False, domain="sync"):
        midtap = taps[len(taps)//2]
        assert taps[1::2] == [0]*(len(taps)//4) + [midtap] + [0]*(len(taps)//4)
        self.taps = taps
        self.data_shape = data_shape
        if shape_out is None:
            shape_out = FIRFilter.compute_output_shape(data_shape, taps)
        self.shape_out = shape_out
        self.always_ready = always_ready
        self.overclock_rate = overclock_rate
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
        })

    def elaborate(self, platform):
        m = Module()

        always_ready = self.always_ready
        taps     = [ 2 * tap for tap in self.taps ]  # scale by 0.5 at the output
        fir_taps = taps[0::2]
        dly_taps = taps[1::2]
        delay    = dly_taps.index(1) - 1

        # Arms
        fir_shape_out = (self.output.p[0] << 1).shape()
        m.submodules.fir = fir = FIRFilterMAC16(fir_taps, shape=self.data_shape, shape_out=fir_shape_out, 
            overclock_rate=2*self.overclock_rate, always_ready=always_ready, num_channels=2, carry=self.data_shape)
        m.submodules.dly = dly = Delay(delay, shape=self.data_shape, always_ready=always_ready, num_channels=2)

        # Input switching.
        odd = Signal()

        if not self.input.signature.always_ready:
            m.d.comb += self.input.ready.eq(~odd | fir.input.ready)
            m.d.comb += dly.output.ready.eq(fir.input.ready)

        m.d.comb += [
            dly.input.p.eq(self.input.p),
            dly.input.valid.eq(self.input.valid & ~odd),
        ]

        # Even samples are buffered and used as a secondary 
        # carry addition for the FIR filter.
        with m.If(self.input.valid & self.input.ready):
            m.d.sync += odd.eq(~odd)
        
        # 
        for c in range(2):
            m.d.comb += [
                fir.sum_carry[c]   .eq(dly.output.p[c]),
                fir.input.p[c]     .eq(self.input.p[c]),
            ]
        m.d.comb += fir.input.valid .eq(self.input.valid & odd)

        # Output.

        with m.If(~self.output.valid | self.output.ready):
            if not fir.output.signature.always_ready:
                m.d.comb += fir.output.ready.eq(1)
            m.d.sync += self.output.valid.eq(fir.output.valid)
            with m.If(fir.output.valid):
                m.d.sync += self.output.p[0].eq(fir.output.p[0] >> 1)
                m.d.sync += self.output.p[1].eq(fir.output.p[1] >> 1)

        if self._domain != "sync":
            m = DomainRenamer(self._domain)(m)

        return m


class HalfBandInterpolatorMAC16(wiring.Component):
    def __init__(self, taps, data_shape, shape_out=None, overclock_rate=4, always_ready=False, num_channels=1, domain="sync"):
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
        self.num_channels = num_channels
        self.overclock_rate = overclock_rate
        super().__init__({
            "input":  In(stream.Signature(
                data.ArrayLayout(data_shape, num_channels),
                always_ready=always_ready
            )),
            "output": Out(stream.Signature(
                data.ArrayLayout(shape_out, num_channels),
                always_ready=always_ready
            )),
        })

    def elaborate(self, platform):
        m = Module()        

        always_ready = self.always_ready

        taps      = [ 2 * tap for tap in self.taps ]
        arm0_taps = taps[0::2]
        arm1_taps = taps[1::2]
        delay     = arm1_taps.index(1)

        # FIR filter.
        m.submodules.fir = fir = FIRFilterMAC16(arm0_taps, shape=self.data_shape, shape_out=self.shape_out, overclock_rate=self.overclock_rate, always_ready=always_ready, num_channels=self.num_channels, delayed_port=delay)

        # Buffer delayed samples due to the delay introduced in FIRFilterMAC16.
        m.submodules.dly_fifo = dly_fifo = fifo.SyncFIFOBuffered(width=self.num_channels*self.data_shape.as_shape().width, depth=self.overclock_rate+1)
        delay_valid = Signal()
        m.d.sync += delay_valid.eq(self.input.valid & self.input.ready)
        m.d.comb += [
            dly_fifo.w_data.eq(fir.input_delayed),
            dly_fifo.w_en.eq(delay_valid),
        ]

        # Input
        wiring.connect(m, wiring.flipped(self.input), fir.input)

        # Output

        # Arm index selection: switch after every delivered sample
        arm_index = Signal()

        r_data_cast = data.ArrayLayout(self.data_shape, self.num_channels)(dly_fifo.r_data)

        with m.If(~self.output.valid | self.output.ready):
            with m.Switch(arm_index):
                with m.Case(0):
                    for c in range(self.num_channels):
                        m.d.sync += self.output.payload[c].eq(fir.output.payload[c])
                    m.d.sync += self.output.valid.eq(fir.output.valid)
                    if not fir.output.signature.always_ready:
                        m.d.comb += fir.output.ready.eq(1)
                    with m.If(fir.output.valid):
                        m.d.sync += arm_index.eq(1)
                with m.Case(1):
                    for c in range(self.num_channels):
                        m.d.sync += self.output.payload[c].eq(r_data_cast[c])
                    m.d.sync += self.output.valid.eq(dly_fifo.r_rdy)
                    m.d.comb += dly_fifo.r_en.eq(1)
                    with m.If(dly_fifo.r_rdy):
                        m.d.sync += arm_index.eq(0)
        
        if self._domain != "sync":
            m = DomainRenamer(self._domain)(m)

        return m


class FIRFilterMAC16(wiring.Component):

    def __init__(self, taps, shape, shape_out=None, always_ready=False, overclock_rate=8, num_channels=1, carry=None, delayed_port=None):
        self.carry = carry
        self.taps = list(taps)
        self.shape = shape
        self.full_shape = FIRFilter.compute_output_shape(self.shape, self.taps, carry)
        if shape_out is None:
            shape_out = self.full_shape
        self.saturate = self.full_shape.i_bits > shape_out.i_bits
        self.shape_out = shape_out
        self.num_channels = num_channels
        self.always_ready = always_ready
        self.overclock_rate = overclock_rate
        self.delayed_port = delayed_port

        signature = {
            "input":  In(stream.Signature(
                data.ArrayLayout(shape, num_channels),
                always_ready=always_ready
            )),
            "output": Out(stream.Signature(
                data.ArrayLayout(shape_out, num_channels),
                always_ready=always_ready
            )),
        }
        if carry is not None:
            signature.update({
                "sum_carry": In(data.ArrayLayout(carry, num_channels))
            })
        if delayed_port:
            signature.update({
                "input_delayed": Out(data.ArrayLayout(shape, num_channels))
            })
        super().__init__(signature)

    def _build_window(self, m):
        taps          = self.taps
        window_depth  = len(taps)
        window        = Signal(data.ArrayLayout(self.input.p.shape(), window_depth))

        window_valid  = Signal()
        input_ready   = Signal()
        filters_ready = Signal()

        # filters_ready is driven in _build_mac_chain
        m.d.comb += input_ready.eq(~window_valid | filters_ready)
        if not self.input.signature.always_ready:
            m.d.comb += self.input.ready.eq(input_ready)

        sum_carry_q = None
        if self.carry is not None:
            sum_carry_q = Signal.like(self.sum_carry)

        with m.If(input_ready):
            m.d.sync += window_valid.eq(self.input.valid)
            with m.If(self.input.valid):
                m.d.sync += window[0].eq(self.input.p)
                for i in range(1, window_depth):
                    m.d.sync += window[i].eq(window[i - 1])
                if sum_carry_q is not None:
                    m.d.sync += sum_carry_q.eq(self.sum_carry)

        return window, window_valid, filters_ready, sum_carry_q

    def _fold_symmetric(self, m, window, taps):
        symmetric = (taps == taps[::-1])
        if not symmetric:
            return window, taps, self.shape

        # New sample shape: sum of two inputs.
        sum_shape     = (self.input.p[0] + self.input.p[0]).shape()
        odd_symmetric = (len(taps) % 2) == 1
        new_len       = len(taps) // 2 + (1 if odd_symmetric else 0)

        new_window = Signal(data.ArrayLayout(
            data.ArrayLayout(sum_shape, self.num_channels), new_len))

        # pre-sum symmetric pairs
        for i in range(new_len - (1 if odd_symmetric else 0)):
            for c in range(self.num_channels):
                m.d.comb += new_window[i][c].eq(window[i][c] + window[-i - 1][c])

        # center tap for odd length
        if odd_symmetric:
            center = len(taps) // 2
            for c in range(self.num_channels):
                m.d.comb += new_window[-1][c].eq(window[center][c])

        # truncate tap list to first half (+ center if odd)
        new_taps = taps[:ceil(len(taps) / 2)]

        return new_window, new_taps

    def _build_mac_chain(self, m, window, window_valid, taps, filters_ready, sum_carry_q):
        overclock_factor = self.overclock_rate

        # Number of MAC16 blocks
        dsp_block_count = ceil(len(taps) / overclock_factor)

        # Pad taps to make all blocks the same size, if needed.
        if dsp_block_count > 1 and len(taps) % overclock_factor != 0:
            pad = overclock_factor - (len(taps) % overclock_factor)
            taps = taps + [0] * pad

        dsp_blocks = []

        for i in range(dsp_block_count):
            taps_slice   = taps[i * overclock_factor:(i + 1) * overclock_factor]
            window_slice = window[i * overclock_factor:(i + 1) * overclock_factor]
            carry        = None if i > 0 else self.carry

            dsp = SerialMAC16(
                taps=taps_slice,
                shape=window_slice[0][0].shape(),  # element shape
                carry=carry,
                always_ready=self.always_ready,
                num_channels=self.num_channels,
            )

            # Connect window into this MAC16.
            for j, sample in enumerate(window_slice):
                m.d.comb += dsp.input.p[j].eq(sample)
            m.d.comb += dsp.input.valid.eq(window_valid)

            # first block drives filters_ready and captures carry if any
            if i == 0:
                m.d.comb += filters_ready.eq(dsp.input.ready)
                if sum_carry_q is not None:
                    m.d.comb += dsp.sum_carry.eq(sum_carry_q)

            m.submodules[f"dsp_{i}"] = dsp
            dsp_blocks.append(dsp)

        return dsp_blocks

    def elaborate(self, platform):
        m = Module()

        # Samples window + sum_carry capture.
        window, window_valid, filters_ready, sum_carry_q = self._build_window(m)

        # Connect delayed copy of the input if requested.
        if self.delayed_port is not None:
            m.d.comb += self.input_delayed.eq(window[self.delayed_port])

        # Symmetric folding (if applicable). This is done combinatorially.
        window, taps = self._fold_symmetric(m, window, self.taps)

        # MAC chain.
        dsp_blocks = self._build_mac_chain(
            m,
            window,
            window_valid,
            taps,
            filters_ready,
            sum_carry_q,
        )

        # FIR output stream (full precision).
        fir_output = stream.Signature(
            data.ArrayLayout(self.full_shape, self.num_channels),
            always_ready=self.always_ready,
        ).create()

        # Final sum.
        if len(dsp_blocks) == 1:
            # There's only 1 DSP block per channel: wire directly.
            block = dsp_blocks[0]
            m.d.comb += fir_output.valid.eq(block.output.valid)
            for c in range(self.num_channels):
                m.d.comb += fir_output.p[c].eq(block.output.p[c])
            if not block.output.signature.always_ready:
                m.d.comb += block.output.ready.eq(fir_output.ready)
        else:
            # Add results of the individual DSP blocks.
            advance = Signal()
            
            first = dsp_blocks[0]
            with m.If(~fir_output.valid | fir_output.ready):
                m.d.sync += fir_output.valid.eq(first.output.valid)
                m.d.comb += advance.eq(1)
                for block in dsp_blocks:
                    if not block.output.signature.always_ready:
                        m.d.comb += block.output.ready.eq(advance)
                for c in range(self.num_channels):
                    m.d.sync += fir_output.p[c].eq(sum(block.output.p[c] for block in dsp_blocks))

        # Perform rounding and saturation when needed.
        last_output = fir_output
        if self.full_shape != self.shape_out:
            last_output = FIRFilter.xform_stage(m, last_output, lambda t: t.round(self.shape_out.f_bits, clip=not self.saturate))
        if self.saturate:
            last_output = FIRFilter.xform_stage(m, last_output, lambda t: t.saturate(self.shape_out), domain="comb")

        wiring.connect(m, last_output, wiring.flipped(self.output))

        return m


class SerialMAC16(wiring.Component):

    def __init__(self, taps, shape, shape_out=None, taps_shape=None, carry=None, num_channels=1, always_ready=False):
        assert shape.as_shape().width <= 16, f"DSP slice inputs have a maximum width of 16 bit. {shape} {shape.as_shape().width}"

        self.carry = carry
        self.taps = list(taps)
        self.shape = shape
        self.taps_shape = taps_shape or self.taps_shape()
        if shape_out is None:
            shape_out = FIRFilter.compute_output_shape(shape, taps, add_shape=carry)
        self.shape_out = shape_out
        self.num_channels = num_channels
        self.always_ready = always_ready
        signature = {
            "input":            In(stream.Signature(data.ArrayLayout(
                data.ArrayLayout(shape, num_channels),
                len(taps)
            ), always_ready=always_ready)),
            "output":           Out(stream.Signature(
                data.ArrayLayout(shape_out, num_channels),
            always_ready=always_ready)),
        }
        if carry is not None:
            signature.update({
                "sum_carry": In(data.ArrayLayout(carry, num_channels))
            })
        else:
            self.sum_carry = [0]*num_channels
        super().__init__(signature)

    def taps_shape(self):
        _signed        = any(t<0 for t in self.taps)
        taps_as_ratios = [fixed.Const(tap).as_integer_ratio() for tap in self.taps]
        max_denom      = max(abs(denom) for _, denom in taps_as_ratios)
        f_bits         = ceil_log2(max_denom)
        t_bits         = max(f_bits, ceil_log2(max(abs(n) * max_denom // d for n,d in taps_as_ratios)))
        base_shape     = signed if _signed else unsigned
        return fixed.Shape(base_shape(t_bits + _signed), f_bits)

    def elaborate(self, platform):
        m = Module()

        depth       = len(self.taps)
        index       = Signal(range(depth))      # tap index
        active      = Signal()                  # window is being consumed
        dsp_ready   = Signal()                  # MAC16 ready
        dsp_valid   = Signal()                  # valid_in to MAC16

        m.d.comb += active.eq(index != 0)

        # Ready to accept a new window when MAC can start and the last sample
        # of the previous window is being consumed.
        input_ready = Signal()
        m.d.comb += input_ready.eq(dsp_ready & (index == depth-1))
        if not self.input.signature.always_ready:
            m.d.comb += self.input.ready.eq(input_ready)

        # Register inputs (valid, sample from window).
        dsp_a = Signal(data.ArrayLayout(self.shape, self.num_channels))
        with m.If(dsp_ready):
            m.d.sync += dsp_valid.eq(self.input.valid | active)
            with m.If(self.input.valid | active):
                m.d.sync += index.eq(_incr(index, depth))
            
            with m.Switch(index):
                for i in range(depth):
                    with m.Case(i):
                        m.d.sync += dsp_a.eq(self.input.p[i])
                with m.Default():
                    m.d.sync += dsp_a.eq(0)

        # Coefficient ROM.
        taps_shape = self.taps_shape
        assert taps_shape.as_shape().width <= 16, "DSP slice inputs have a maximum width of 16 bit."
        coeff_data = memory.MemoryData(
            shape=taps_shape,
            depth=depth,
            init=(fixed.Const(tap, shape=taps_shape) for tap in self.taps),
        )
        m.submodules.coeff_rom = coeff_rom = memory.Memory(data=coeff_data)
        coeff_rd = coeff_rom.read_port(domain="sync")
        m.d.comb += coeff_rd.addr.eq(index)
        m.d.comb += coeff_rd.en.eq(dsp_ready)

        shape_out = FIRFilter.compute_output_shape(self.shape, self.taps, self.carry)

        if self.carry:
            sum_carry_q = Signal.like(self.sum_carry)
            with m.If(input_ready):
                m.d.sync += sum_carry_q.eq(self.sum_carry)
        else:
            sum_carry_q = self.sum_carry

        valid_cnt = Signal(depth, init=1)
        mult_cnt  = Signal(depth, init=1)

        for c in range(self.num_channels):
            m.submodules[f"dsp_c{c}"] = dsp = iCE40Multiplier(
                o_width=shape_out.as_shape().width,
                p_width=shape_out.as_shape().width)

            m.d.comb += [
                dsp.a               .eq(dsp_a[c]),
                dsp.b               .eq(coeff_rd.data),
                shape_out(dsp.p)    .eq(sum_carry_q[c]),
                dsp.valid_in        .eq(dsp_valid),
                dsp.p_load          .eq(mult_cnt[0]),
                self.output.p[c]    .eq(shape_out(dsp.o)),
                dsp.ready_out       .eq(self.output.ready | ~valid_cnt[-1]),
            ]
            if c == 0:
                m.d.comb += [
                    dsp_ready.eq(dsp.ready_in),
                    self.output.valid.eq(dsp.valid_out & valid_cnt[-1]),
                ]
        
                # Multiplier input and output counters.
                with m.If(dsp.valid_in & dsp.ready_in):
                    m.d.sync += mult_cnt.eq(mult_cnt.rotate_left(1))
                with m.If(dsp.valid_out & dsp.ready_out):
                    m.d.sync += valid_cnt.eq(valid_cnt.rotate_left(1))

        return m



class iCE40Multiplier(wiring.Component):

    def __init__(self, a_width=16, b_width=16, p_width=32, o_width=32):
        signature = {
            "a": In(signed(a_width)),
            "b": In(signed(b_width)),
            "valid_in": In(1),
            "ready_in": Out(1),
            "o": Out(signed(o_width)),
            "valid_out": Out(1),
            "ready_out": In(1),
        }
        if p_width > 0:
            signature.update({
                "p": In(signed(p_width)),
                "p_load": In(1),
            })
        super().__init__(signature)
        self.p_width = p_width
   
    def elaborate(self, platform):
        m = Module()

        def pipe(signal, length):
            pipe = [ signal ] + [ Signal.like(signal, name=f"{signal.name}_q{i}") for i in range(length) ]
            with m.If(self.ready_in):  # clock enable
                m.d.sync += [ pipe[i+1].eq(pipe[i]) for i in range(length) ]
            return pipe

        dsp_delay   = 3
        valid_pipe  = pipe(self.valid_in, dsp_delay)

        if self.p_width > 0:
            p_load_v    = Signal()
            m.d.comb   += p_load_v.eq(self.p_load & self.valid_in)
            p_pipe      = pipe(self.p, 2)
            p_load_pipe = pipe(p_load_v, 2)
        
        m.d.comb += self.ready_in.eq(~self.valid_out | self.ready_out)

        m.submodules.sb_mac16 = mac = SB_MAC16(
            C_REG=0,
            A_REG=1,
            B_REG=1,
            D_REG=0,
            TOP_8x8_MULT_REG=0,
            BOT_8x8_MULT_REG=0,
            PIPELINE_16x16_MULT_REG1=0,
            PIPELINE_16x16_MULT_REG2=1,
            TOPOUTPUT_SELECT=1,
            TOPADDSUB_LOWERINPUT=2,
            TOPADDSUB_UPPERINPUT=1,
            TOPADDSUB_CARRYSELECT=2,
            BOTOUTPUT_SELECT=1,
            BOTADDSUB_LOWERINPUT=2,
            BOTADDSUB_UPPERINPUT=1,
            BOTADDSUB_CARRYSELECT=0,
            MODE_8x8=0,
            A_SIGNED=1,
            B_SIGNED=1,
        )

        m.d.comb += [
            # Inputs.
            mac.CLK                         .eq(ClockSignal("sync")),
            mac.CE                          .eq(self.ready_in),
            mac.A.as_signed()               .eq(self.a),
            mac.B.as_signed()               .eq(self.b),
            Cat(mac.D, mac.C).as_signed()   .eq(Mux(p_load_pipe[2], p_pipe[2], mac.O) if self.p_width > 0 else 0),
            mac.AHOLD                       .eq(~valid_pipe[0]),  # 0: load
            mac.BHOLD                       .eq(~valid_pipe[0]),
            mac.CHOLD                       .eq(0),
            mac.DHOLD                       .eq(0),
            mac.OHOLDTOP                    .eq(~valid_pipe[2]),
            mac.OHOLDBOT                    .eq(~valid_pipe[2]),
            mac.ADDSUBTOP                   .eq(0),
            mac.ADDSUBBOT                   .eq(0),
            mac.OLOADTOP                    .eq(0),
            mac.OLOADBOT                    .eq(0),

            # Outputs.
            self.o                          .eq(mac.O),
            self.valid_out                  .eq(valid_pipe[dsp_delay]),
        ]

        return m


def _incr(signal, modulo):
    if modulo == 2 ** len(signal):
        return signal + 1
    else:
        return Mux(signal == modulo - 1, 0, signal + 1)
