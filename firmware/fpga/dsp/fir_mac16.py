#
# This file is part of HackRF.
#
# Copyright (c) 2025 Great Scott Gadgets <info@greatscottgadgets.com>
# SPDX-License-Identifier: BSD-3-Clause

from math                   import ceil, log2

from amaranth               import Module, Signal, Mux, DomainRenamer, ClockSignal, signed
from amaranth.lib           import wiring, stream, data, memory, fifo
from amaranth.lib.wiring    import In, Out
from amaranth.utils         import bits_for

from amaranth_future        import fixed

from dsp.sb_mac16           import SB_MAC16
from dsp.fir                import Delay


class HalfBandDecimatorMAC16(wiring.Component):
    def __init__(self, taps, data_shape, overclock_rate=4, shape_out=None, always_ready=False, domain="sync"):
        midtap = taps[len(taps)//2]
        assert taps[1::2] == [0]*(len(taps)//4) + [midtap] + [0]*(len(taps)//4)
        self.taps = taps
        self.data_shape = data_shape
        if shape_out is None:
            shape_out = data_shape
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
        m.submodules.fir = fir = FIRFilterMAC16(fir_taps, shape=self.data_shape, overclock_rate=2*self.overclock_rate, always_ready=always_ready, num_channels=2, carry=self.data_shape)
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
                fir.sum_carry[c]   .eq(dly.output.p[c]),  # TODO: optimize shape?
                fir.input.p[c]     .eq(self.input.p[c]),
            ]
        m.d.comb += fir.input.valid .eq(self.input.valid & odd)

        # Output.

        with m.If(~self.output.valid | self.output.ready):
            if not fir.output.signature.always_ready:
                m.d.comb += fir.output.ready.eq(1)
            m.d.sync += self.output.valid.eq(fir.output.valid)
            with m.If(fir.output.valid):
                m.d.sync += self.output.p[0].eq(fir.output.p[0] * fixed.Const(0.5))
                m.d.sync += self.output.p[1].eq(fir.output.p[1] * fixed.Const(0.5))

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
        if shape_out is None:
            shape_out = data_shape
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

        # Arms
        m.submodules.fir = fir = FIRFilterMAC16(arm0_taps, shape=self.data_shape, shape_out=self.shape_out, overclock_rate=self.overclock_rate, always_ready=always_ready, num_channels=self.num_channels)
        m.submodules.dly = dly = Delay(delay, shape=self.data_shape, always_ready=always_ready, num_channels=self.num_channels)
        m.submodules.dly_fifo = dly_fifo = fifo.SyncFIFOBuffered(width=self.num_channels*self.data_shape.as_shape().width, depth=self.overclock_rate+1)
        
        m.d.comb += [
            dly_fifo.w_data.eq(dly.output.p),
            dly_fifo.w_en.eq(dly.output.valid),
        ]
        if not dly.output.signature.always_ready:
            m.d.comb += dly.output.ready.eq(dly_fifo.w_rdy)

        #busy = Signal()
        #with m.If(fir.input.valid & fir.input.ready):
        #    m.d.sync += busy.eq(1)

        # Input
        m.d.comb += fir.input.payload.eq(self.input.payload)
        m.d.comb += fir.input.valid.eq(self.input.valid & dly.input.ready)
        m.d.comb += dly.input.payload.eq(self.input.payload)
        m.d.comb += dly.input.valid.eq(self.input.valid & fir.input.ready)

        if not self.input.signature.always_ready:
            m.d.comb += self.input.ready.eq(fir.input.ready & dly.input.ready)

        # Output

        # Arm index selection: switch after every delivered sample
        arm_index = Signal()

        #delayed = Signal.like(fir.input_delayed)
        #with m.If(fir.output.valid & fir.output.ready):
        #    m.d.sync += delayed.eq(fir.input_delayed)
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

    def __init__(self, taps, shape, shape_out=None, always_ready=False, overclock_rate=8, num_channels=1, carry=None, delayed_port=False):
        self.carry = carry
        self.taps = list(taps)
        self.shape = shape
        if shape_out is None:
            shape_out = self.compute_output_shape()
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

    def taps_shape(self, taps=None):
        taps            = taps or self.taps
        taps_as_ratios  = [tap.as_integer_ratio() for tap in taps]
        f_width         = bits_for(max(tap[1] for tap in taps_as_ratios)) - 1
        i_width         = max(0, bits_for(max(abs(tap[0]) for tap in taps_as_ratios)) - f_width)
        return fixed.Shape(i_width, f_width, signed=any(tap < 0 for tap in taps))

    def compute_output_shape(self):
        taps_shape = self.taps_shape()
        signed     = self.shape.signed | taps_shape.signed
        f_width    = self.shape.f_width + taps_shape.f_width
        filter_gain = ceil(log2(sum(self.taps)))
        i_width    = max(0, self.shape.as_shape().width + taps_shape.as_shape().width - signed - f_width + filter_gain)
        if self.carry is not None:
            f_width = max(f_width, self.carry.f_width)
            i_width = max(i_width, self.carry.i_width) + 1
        shape_out = fixed.Shape(i_width, f_width, signed=signed)
        return shape_out

    def elaborate(self, platform):
        m = Module()

        # Build filter out of SerialMAC16 blocks.
        overclock_factor = self.overclock_rate

        taps = self.taps

        if self.carry is not None:
            sum_carry_q = Signal.like(self.sum_carry)

        filters_ready = Signal()
        window_valid = Signal()
        input_ready = Signal()
        m.d.comb += input_ready.eq(~window_valid | filters_ready)
        if not self.input.signature.always_ready:
            m.d.comb += self.input.ready.eq(input_ready)

        # Samples window.
        window = [ Signal.like(self.input.p, name=f"window_{i}") for i in range(len(self.taps)) ]

        with m.If(input_ready):
            m.d.sync += window_valid.eq(self.input.valid)
            with m.If(self.input.valid):
                m.d.sync += window[0].eq(self.input.p)
                for i in range(1, len(window)):
                    m.d.sync += window[i].eq(window[i-1])
                if self.carry is not None:
                    m.d.sync += sum_carry_q.eq(self.sum_carry)

        # When filter is symmetric, presum samples to obtain a smaller window.
        symmetric = (self.taps == self.taps[::-1])
        if symmetric:
            sum_shape = (self.input.p[0] + self.input.p[0]).shape()
            odd_symmetric = ((len(self.taps) % 2) == 1)
            new_len = len(self.taps) // 2 + odd_symmetric
            new_window = [ Signal(data.ArrayLayout(sum_shape, self.num_channels), name=f"window_sym_{i}") for i in range(new_len) ]
            for i in range(len(new_window) - odd_symmetric):
                for c in range(self.num_channels):
                    m.d.comb += new_window[i][c].eq(window[i][c] + window[-i-1][c])
            if odd_symmetric:
                for c in range(self.num_channels):
                    m.d.comb += new_window[-1][c].eq(window[len(self.taps)//2][c])
            window = new_window
            taps = self.taps[:ceil(len(self.taps)/2)]
            samples_shape = sum_shape
        else:
            samples_shape = self.shape

        # Build filter out of SerialMAC16 blocks: each one multiplies and 
        # accumulates `overclock_factor` taps serially.
        dsp_block_count = ceil(len(taps) / overclock_factor)

        # If we have multiple subfilters, make them all the same size.
        if dsp_block_count > 1 and len(taps) % overclock_factor != 0:
            taps = taps + [0]*(overclock_factor - (len(taps)%overclock_factor))

        for c in range(self.num_channels):

            dsp_blocks = []

            for i in range(dsp_block_count):
                taps_slice = taps[i*overclock_factor:(i+1)*overclock_factor]
                window_slice = window[i*overclock_factor:(i+1)*overclock_factor]
                carry = None if i > 0 else self.carry
                
                dsp = SerialMAC16(taps=taps_slice, shape=samples_shape, taps_shape=self.taps_shape(taps), carry=carry, always_ready=self.always_ready)
                dsp_blocks.append(dsp)

                for j in range(len(window_slice)):
                    m.d.comb += dsp.input.p[j].eq(window_slice[j][c])
                    m.d.comb += dsp.input.valid.eq(window_valid)

                if i == 0:
                    m.d.comb += filters_ready.eq(dsp.input.ready)
                    if self.carry is not None:
                        m.d.comb += dsp.sum_carry.eq(sum_carry_q[c])
            
            m.submodules += dsp_blocks

            # Adder tree for channel c
            if dsp_block_count > 1:
                with m.If(~self.output.valid | self.output.ready):
                    for i in range(dsp_block_count):
                        if not dsp_blocks[i].output.signature.always_ready:
                            m.d.comb += dsp_blocks[i].output.ready.eq(1)
                    m.d.sync += self.output.valid.eq(dsp_blocks[0].output.valid)
                    with m.If(dsp_blocks[0].output.valid):
                        m.d.sync += self.output.payload[c]  .eq(sum(dsp_blocks[i].output.p for i in range(dsp_block_count)))
            else:
                m.d.comb += self.output.payload[c].eq(dsp_blocks[0].output.p)
                m.d.comb += self.output.valid.eq(dsp_blocks[0].output.valid)
                if not dsp_blocks[0].output.signature.always_ready:
                    m.d.comb += dsp_blocks[0].output.ready.eq(self.output.ready)

        return m


class SerialMAC16(wiring.Component):

    def __init__(self, taps, shape, shape_out=None, taps_shape=None, carry=None, always_ready=False):
        assert shape.as_shape().width <= 16, f"DSP slice inputs have a maximum width of 16 bit. {shape} {shape.as_shape().width}"

        self.carry = carry
        self.taps = list(taps)
        self.shape = shape
        self.taps_shape = taps_shape or self.taps_shape()
        if shape_out is None:
            shape_out = self.compute_output_shape()
        self.shape_out = shape_out
        self.always_ready = always_ready
        signature = {
            "input":            In(stream.Signature(data.ArrayLayout(shape, len(taps)), always_ready=always_ready)),
            "output":           Out(stream.Signature(shape_out, always_ready=always_ready)),
        }
        if carry is not None:
            signature.update({
                "sum_carry": In(carry)
            })
        else:
            self.sum_carry = 0
        super().__init__(signature)

    def taps_shape(self):
        taps_as_ratios  = [tap.as_integer_ratio() for tap in self.taps]
        f_width         = bits_for(max(tap[1] for tap in taps_as_ratios)) - 1
        i_width         = max(0, bits_for(max(abs(tap[0]) for tap in taps_as_ratios)) - f_width)
        return fixed.Shape(i_width, f_width, signed=any(tap < 0 for tap in self.taps))

    def compute_output_shape(self):
        taps_shape = self.taps_shape
        signed     = self.shape.signed | taps_shape.signed
        f_width    = self.shape.f_width + taps_shape.f_width
        filter_gain = ceil(log2(max(1, sum(self.taps))))
        i_width    = max(0, self.shape.as_shape().width + taps_shape.as_shape().width - signed - f_width + filter_gain)
        if self.carry is not None:
            f_width = max(f_width, self.carry.f_width)
            i_width = max(i_width, self.carry.i_width) + 1
        shape_out = fixed.Shape(i_width, f_width, signed=signed)
        return shape_out

    def elaborate(self, platform):
        m = Module()

        depth = len(self.taps)
        counter_in   = Signal(range(depth))
        dsp_ready = Signal()
        multin_valid = Signal()

        input_ready = Signal()
        # Ready to process a sample either when the DSP slice is ready and the samples window is:
        # - Not valid yet.
        # - Only valid for 1 more cycle.
        m.d.comb += input_ready.eq((counter_in == depth-1) & dsp_ready)
        if not self.input.signature.always_ready:
            m.d.comb += self.input.ready.eq(input_ready)

        # Sample counter.
        with m.If((self.input.valid | (counter_in != 0)) & dsp_ready):
            m.d.sync += counter_in.eq(_incr(counter_in, depth))

        with m.If(dsp_ready):
            m.d.sync += multin_valid.eq(self.input.valid | (counter_in != 0))

        # Select sample from window.
        dsp_a = Signal(self.shape)
        with m.If(dsp_ready):
            with m.Switch(counter_in):
                for i in range(depth):
                    with m.Case(i):
                        m.d.sync += dsp_a.eq(self.input.p[i])

        # Coefficient ROM.
        taps_shape = self.taps_shape
        assert taps_shape.as_shape().width <= 16, "DSP slice inputs have a maximum width of 16 bit."
        coeff_data = memory.MemoryData(
            shape=taps_shape,
            depth=depth,  # +200 to force BRAM
            init=(fixed.Const(tap, shape=taps_shape) for tap in self.taps),
        )
        m.submodules.coeff_rom = coeff_rom = memory.Memory(data=coeff_data)
        coeff_rd = coeff_rom.read_port(domain="sync")
        m.d.comb += coeff_rd.addr.eq(counter_in)
        m.d.comb += coeff_rd.en.eq(dsp_ready)

        shape_out = self.compute_output_shape()

        if self.carry:
            sum_carry_q = Signal.like(self.sum_carry)
            with m.If(input_ready):
                m.d.sync += sum_carry_q.eq(self.sum_carry)

        m.submodules.dsp = dsp = iCE40Multiplier(
            o_width=shape_out.as_shape().width,
            always_ready=self.always_ready)

        valid_cnt = Signal(depth, init=1)
        mult_cnt  = Signal(depth, init=1)
        m.d.comb += [
            dsp.a               .eq(dsp_a),
            dsp.b               .eq(coeff_rd.data),
            shape_out(dsp.p)    .eq(sum_carry_q if self.carry is not None else 0),
            dsp.valid_in        .eq(multin_valid),
            dsp_ready           .eq(dsp.ready_in),
            dsp.p_load          .eq(mult_cnt[0]),
            self.output.p       .eq(shape_out(dsp.o)),
            self.output.valid   .eq(dsp.valid_out & valid_cnt[-1]),
            dsp.ready_out       .eq(self.output.ready | ~valid_cnt[-1]),
        ]
        
        # Multiplier input and output counters.
        with m.If(dsp.valid_in & dsp.ready_in):
            m.d.sync += mult_cnt.eq(mult_cnt.rotate_left(1))
        with m.If(dsp.valid_out & dsp.ready_out):
            m.d.sync += valid_cnt.eq(valid_cnt.rotate_left(1))

        return m



class iCE40Multiplier(wiring.Component):

    def __init__(self, a_width=16, b_width=16, p_width=32, o_width=32, always_ready=False):
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
        self.always_ready = always_ready
        self.p_width = p_width
        self.o_width = o_width
   
    def elaborate(self, platform):
        m = Module()

        def pipe(signal, length):
            pipe = [ signal ] + [ Signal.like(signal, name=f"{signal.name}_q{i}") for i in range(length) ]
            for i in range(length):
                m.d.sync += pipe[i+1].eq(pipe[i])
            return pipe

        valid_v     = Signal()
        m.d.comb += valid_v.eq(self.valid_in & self.ready_in)

        dsp_delay   = 3
        valid_pipe  = pipe(valid_v, dsp_delay)

        if self.p_width > 0:
            p_load_v    = Signal()
            m.d.comb   += p_load_v.eq(self.p_load & valid_v)
            p_pipe      = pipe(self.p, dsp_delay-1)
            p_load_pipe = pipe(p_load_v, dsp_delay - 1)

        # skid buffer
        if not self.always_ready:
            m.submodules.out_fifo = out_fifo = fifo.SyncFIFOBuffered(width=self.o_width, depth=dsp_delay+2)
        
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
            TOPADDSUB_CARRYSELECT=3,
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
            mac.CLK         .eq(ClockSignal("sync")),
            mac.CE          .eq(1),
            mac.C.as_signed().eq(Mux(p_load_pipe[2], p_pipe[2][16:], mac.O[16:]) if self.p_width > 0 else 0),
            mac.A.as_signed().eq(self.a),
            mac.B.as_signed().eq(self.b),
            mac.D.as_signed().eq(Mux(p_load_pipe[2], p_pipe[2][:16], mac.O[:16]) if self.p_width > 0 else 0),
            mac.AHOLD       .eq(~valid_pipe[0]),  # 0: load
            mac.BHOLD       .eq(~valid_pipe[0]),
            mac.CHOLD       .eq(0),
            mac.DHOLD       .eq(0),
            mac.OHOLDTOP    .eq(~valid_pipe[2]),
            mac.OHOLDBOT    .eq(~valid_pipe[2]),
            mac.ADDSUBTOP   .eq(0),
            mac.ADDSUBBOT   .eq(0),
            mac.OLOADTOP    .eq(0),
            mac.OLOADBOT    .eq(0),
        ]

        if not self.always_ready:
            m.d.comb += [
                out_fifo.w_data.eq(mac.O),
                out_fifo.w_en.eq(valid_pipe[dsp_delay]),
                
                self.o.eq(out_fifo.r_data),
                self.valid_out.eq(out_fifo.r_rdy),
                out_fifo.r_en.eq(self.ready_out),
            ]
        else:
            m.d.comb += [                
                self.o.eq(mac.O),
                self.valid_out.eq(valid_pipe[dsp_delay]),
            ]

        return m


def _incr(signal, modulo):
    if modulo == 2 ** len(signal):
        return signal + 1
    else:
        return Mux(signal == modulo - 1, 0, signal + 1)

#
# Tests
#

import unittest
import numpy as np
from amaranth.sim import Simulator

class _TestFilter(unittest.TestCase):

    rng = np.random.default_rng(0)

    def _generate_samples(self, count, width, f_width=0):
        # Generate `count` random samples.
        samples = self.rng.normal(0, 1, count)

        # Convert to integer.
        samples = np.round(samples / max(abs(samples)) * (2**(width-1) - 1)).astype(int)
        assert max(samples) < 2**(width-1) and min(samples) >= -2**(width-1)  # sanity check

        if f_width > 0:
            return samples / (1 << f_width)
        return samples

    def _filter(self, dut, samples, count, num_channels=1, outfile=None, empty_cycles=0, empty_ready_cycles=0):

        async def input_process(ctx):
            if hasattr(dut, "enable"):
                ctx.set(dut.enable, 1)
            await ctx.tick()

            for i, sample in enumerate(samples):
                if num_channels > 1:
                    ctx.set(dut.input.payload, [s.item() for s in sample])
                else:
                    if isinstance(dut.input.payload.shape(), data.ArrayLayout):
                        ctx.set(dut.input.payload, [sample.item()])
                    else:
                        ctx.set(dut.input.payload, sample.item())
                ctx.set(dut.input.valid, 1)
                await ctx.tick().until(dut.input.ready)
                ctx.set(dut.input.valid, 0)
                if empty_cycles > 0:
                    await ctx.tick().repeat(empty_cycles)

        filtered = []
        async def output_process(ctx):
            if not dut.output.signature.always_ready:
                ctx.set(dut.output.ready, 1)
            while len(filtered) < count:
                payload, = await ctx.tick().sample(dut.output.payload).until(dut.output.valid)
                if num_channels > 1:
                    filtered.append([v.as_float() for v in payload])
                else:
                    if isinstance(payload.shape(), data.ArrayLayout):
                        filtered.append(payload[0].as_float())
                    else:
                        filtered.append(payload.as_float())
                if empty_ready_cycles > 0:
                    ctx.set(dut.output.ready, 0)
                    await ctx.tick().repeat(empty_ready_cycles)
                    ctx.set(dut.output.ready, 1)
            if not dut.output.signature.always_ready:
                ctx.set(dut.output.ready, 0)

        sim = Simulator(dut)
        sim.add_clock(1/100e6)
        sim.add_testbench(input_process)
        sim.add_testbench(output_process)
        if outfile:
            with sim.write_vcd(outfile):
                sim.run()
        else:
            sim.run()
        
        return filtered


class TestFIRFilterMAC16(_TestFilter):

    def test_filter(self):
        taps = [-1, 0, 9, 16, 9, 0, -1]
        taps = [ tap / 32 for tap in taps ]

        num_samples = 1024
        input_width = 8
        input_samples = self._generate_samples(num_samples, input_width)

        # Compute the expected result
        filtered_np = np.convolve(input_samples, taps).tolist()

        # Simulate DUT
        dut = FIRFilterMAC16(taps, shape=fixed.SQ(8, 0), always_ready=False)
        filtered = self._filter(dut, input_samples, len(input_samples), empty_ready_cycles=5)

        self.assertListEqual(filtered_np[:len(filtered)], filtered)


class TestHalfBandDecimatorMAC16(_TestFilter):

    def test_filter(self):

        common_dut_options = dict(
            data_shape=fixed.SQ(7),
            shape_out=fixed.SQ(0,31),
            overclock_rate=4,
        )

        taps0 = (np.array([-1, 0, 9, 16, 9, 0, -1]) / 32).tolist()
        taps1 = (np.array([-2, 0, 7, 0, -18, 0, 41, 0, -92, 0, 320, 512, 320, 0, -92, 0, 41, 0, -18, 0, 7, 0, -2]) / 1024).tolist()


        inputs = {

            "test_filter_with_backpressure": {
                "num_samples": 1024,
                "dut_options": dict(**common_dut_options, always_ready=False, taps=taps0),
                "sim_opts": dict(empty_cycles=0),
            },

            "test_filter_with_backpressure_and_empty_cycles": {
                "num_samples": 1024,
                "dut_options": dict(**common_dut_options, always_ready=False, taps=taps0),
                "sim_opts": dict(empty_cycles=3),
            },

            "test_filter_with_backpressure_taps1": {
                "num_samples": 1024,
                "dut_options": dict(**common_dut_options, always_ready=False, taps=taps1),
                "sim_opts": dict(empty_cycles=0),
            },

            "test_filter_no_backpressure_and_empty_cycles_taps1": {
                "num_samples": 1024,
                "dut_options": dict(**common_dut_options, always_ready=True, taps=taps0),
                "sim_opts": dict(empty_cycles=6),
            },

            "test_filter_no_backpressure": {
                "num_samples": 1024,
                "dut_options": dict(**common_dut_options, always_ready=True, taps=taps1),
                "sim_opts": dict(empty_cycles=3),
            },
        }
        
        for name, scenario in inputs.items():

            with self.subTest(name):
                taps        = scenario["dut_options"]["taps"]
                num_samples = scenario["num_samples"]

                input_width = 8
                samples_i_in = self._generate_samples(num_samples, input_width, f_width=7)
                samples_q_in = self._generate_samples(num_samples, input_width, f_width=7)

                # Compute the expected result
                filtered_i_np = np.convolve(samples_i_in, taps)[1::2].tolist()
                filtered_q_np = np.convolve(samples_q_in, taps)[1::2].tolist()

                # Simulate DUT
                dut = HalfBandDecimatorMAC16(**scenario["dut_options"])
                filtered = self._filter(dut, zip(samples_i_in, samples_q_in), len(samples_i_in) // 2, num_channels=2, **scenario["sim_opts"])
                filtered_i = [ x[0] for x in filtered ]
                filtered_q = [ x[1] for x in filtered ]

                self.assertListEqual(filtered_i_np[:len(filtered_i)], filtered_i)
                self.assertListEqual(filtered_q_np[:len(filtered_q)], filtered_q)


class TestHalfBandInterpolatorMAC16(_TestFilter):

    def test_filter(self):

        common_dut_options = dict(
            data_shape=fixed.SQ(7),
            shape_out=fixed.SQ(1,16),
            overclock_rate=4,
        )

        taps0 = (np.array([-1, 0, 9, 16, 9, 0, -1]) / 32).tolist()
        taps1 = (np.array([-2, 0, 7, 0, -18, 0, 41, 0, -92, 0, 320, 512, 320, 0, -92, 0, 41, 0, -18, 0, 7, 0, -2]) / 1024).tolist()

        inputs = {

            "test_filter_with_backpressure": {
                "num_samples": 1024,
                "dut_options": dict(**common_dut_options, always_ready=False, num_channels=2, taps=taps1),
                "sim_opts": dict(empty_cycles=0, empty_ready_cycles=0),
            },

            "test_filter_with_backpressure_and_empty_cycles": {
                "num_samples": 1024,
                "dut_options": dict(**common_dut_options, num_channels=2, always_ready=False, taps=taps0),
                "sim_opts": dict(empty_ready_cycles=7, empty_cycles=3),
            },

            "test_filter_with_backpressure_taps1": {
                "num_samples": 1024,
                "dut_options": dict(**common_dut_options, num_channels=2, always_ready=False, taps=taps1),
                "sim_opts": dict(empty_ready_cycles=7, empty_cycles=0),
            },

            "test_filter_no_backpressure_and_empty_cycles_taps1": {
                "num_samples": 1024,
                "dut_options": dict(**common_dut_options, num_channels=2, always_ready=True, taps=taps0),
                "sim_opts": dict(empty_cycles=8),
            },

            "test_filter_no_backpressure": {
                "num_samples": 1024,
                "dut_options": dict(**common_dut_options, num_channels=2, always_ready=True, taps=taps1),
                "sim_opts": dict(empty_cycles=16),
            },

        }

    
        for name, scenario in inputs.items():
            with self.subTest(name):
                taps        = scenario["dut_options"]["taps"]
                num_samples = scenario["num_samples"]

                input_width = 8
                samples_i_in = self._generate_samples(num_samples, input_width, f_width=7)
                samples_q_in = self._generate_samples(num_samples, input_width, f_width=7)

                # Compute the expected result
                input_samples_pad = np.zeros(2*len(samples_i_in))
                input_samples_pad[0::2] = 2*samples_i_in  # pad with zeros, adjust gain
                filtered_i_np = np.convolve(input_samples_pad, taps).tolist()
                input_samples_pad = np.zeros(2*len(samples_q_in))
                input_samples_pad[0::2] = 2*samples_q_in  # pad with zeros, adjust gain
                filtered_q_np = np.convolve(input_samples_pad, taps).tolist()

                # Simulate DUT
                dut = HalfBandInterpolatorMAC16(**scenario["dut_options"])
                filtered = self._filter(dut, zip(samples_i_in, samples_q_in), len(samples_i_in) * 2, num_channels=2, **scenario["sim_opts"])
                filtered_i = [ x[0] for x in filtered ]
                filtered_q = [ x[1] for x in filtered ]

                self.assertListEqual(filtered_i_np[:len(filtered_i)], filtered_i)
                self.assertListEqual(filtered_q_np[:len(filtered_q)], filtered_q)

if __name__ == "__main__":
    unittest.main()
