#
# This file is part of HackRF.
#
# Copyright (c) 2025 Great Scott Gadgets <info@greatscottgadgets.com>
# SPDX-License-Identifier: BSD-3-Clause

from math                   import ceil, log2

from amaranth               import Module, Signal, Mux, DomainRenamer, ClockSignal, signed
from amaranth.lib           import wiring, stream, data, memory
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
            m.d.comb += dly.output.ready.eq(1)

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

        # Arms
        m.submodules.fir = fir = FIRFilterMAC16(arm0_taps, shape=self.data_shape, shape_out=self.shape_out, overclock_rate=self.overclock_rate, always_ready=always_ready, num_channels=self.num_channels, delayed_port=True)

        busy = Signal()
        with m.If(fir.input.valid & fir.input.ready):
            m.d.sync += busy.eq(1)

        # Input
        m.d.comb += fir.input.payload.eq(self.input.payload)
        m.d.comb += fir.input.valid.eq(self.input.valid & ~busy)

        if not self.input.signature.always_ready:
            m.d.comb += self.input.ready.eq(fir.input.ready & ~busy)

        # Output

        # Arm index selection: switch after every delivered sample
        arm_index = Signal()

        delayed = Signal.like(fir.input_delayed)
        with m.If(fir.output.valid & fir.output.ready):
            m.d.sync += delayed.eq(fir.input_delayed)


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
                        m.d.sync += self.output.payload[c].eq(delayed[c])
                    m.d.sync += self.output.valid.eq(1)
                    m.d.sync += arm_index.eq(0)
                    m.d.sync += busy.eq(0)
        
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

    def taps_shape(self):
        taps_as_ratios  = [tap.as_integer_ratio() for tap in self.taps]
        f_width         = bits_for(max(tap[1] for tap in taps_as_ratios)) - 1
        i_width         = max(0, bits_for(max(abs(tap[0]) for tap in taps_as_ratios)) - f_width)
        return fixed.Shape(i_width, f_width, signed=any(tap < 0 for tap in self.taps))

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

        # Build filter out of FIRFilterSerialMAC16 blocks.
        overclock_factor = self.overclock_rate

        # Symmetric coefficients special case.
        symmetric = (self.taps == self.taps[::-1])

        # Even-symmetric case. (N=2*K)
        # Odd-symmetric case. (N=2*K+1)
        if symmetric:
            taps = self.taps[:ceil(len(self.taps)/2)]
            odd_symmetric = ((len(self.taps) % 2) == 1)
        else:
            taps = self.taps

        dsp_block_count = ceil(len(taps) / overclock_factor)


        def pipe(signal, length):
            name = signal.name if hasattr(signal, "name") else "signal"
            pipe = [ signal ] + [ Signal.like(signal, name=f"{name}_q{i}") for i in range(length) ]
            for i in range(length):
                m.d.sync += pipe[i+1].eq(pipe[i])
            return pipe


        if self.carry is not None:
            sum_carry_q = Signal.like(self.sum_carry)
            with m.If(self.input.valid & self.input.ready):
                m.d.sync += sum_carry_q.eq(self.sum_carry)

        for c in range(self.num_channels):

            last = self.input
            dsp_blocks = []

            for i in range(dsp_block_count):
                taps_slice = taps[i*overclock_factor:(i+1)*overclock_factor]
                input_delayed = len(taps_slice)
                carry = last.output.p.shape() if i > 0 else self.carry
                
                if (i == dsp_block_count-1) and symmetric and odd_symmetric:
                    taps_slice[-1] /= 2
                    input_delayed -= 1

                dsp = FIRFilterSerialMAC16(taps=taps_slice, shape=self.shape, taps_shape=self.taps_shape(), carry=carry, symmetry=symmetric,
                    input_delayed_cycles=input_delayed, always_ready=self.always_ready)
                dsp_blocks.append(dsp)

                if i == 0:
                    m.d.comb += [
                        dsp.input.p         .eq(self.input.p[c]),
                        dsp.input.valid     .eq(self.input.valid & self.input.ready),
                    ]
                    if not self.input.signature.always_ready:
                        m.d.comb += self.input.ready.eq(dsp.input.ready)
                    if self.carry is not None:
                        m.d.comb += dsp.sum_carry.eq(sum_carry_q[c])
                else:
                    m.d.comb += [
                        dsp.input.p         .eq(pipe(last.input_delayed, last.delay())[-1]),
                        dsp.input.valid     .eq(last.output.valid),
                        dsp.sum_carry       .eq(last.output.p),
                    ]
                    if not last.output.signature.always_ready:
                        m.d.comb += last.output.ready.eq(dsp.input.ready)

                last = dsp

            if self.delayed_port:
                m.d.comb += self.input_delayed[c].eq(last.input_delayed)

            if symmetric:

                for i in reversed(range(dsp_block_count)):
                    end_block = (i == dsp_block_count-1)
                    m.d.comb += [
                        dsp_blocks[i].rev_input    .eq(dsp_blocks[i+1].rev_delayed if not end_block else dsp_blocks[i].input_delayed),
                    ]
            
            m.submodules += dsp_blocks

            m.d.comb += [
                self.output.payload[c]  .eq(last.output.p),
                self.output.valid       .eq(last.output.valid),
            ]
            if not last.output.signature.always_ready:
                m.d.comb += last.output.ready.eq(self.output.ready)

        return m


class FIRFilterSerialMAC16(wiring.Component):

    def __init__(self, taps, shape, shape_out=None, taps_shape=None, carry=None, symmetry=False, input_delayed_cycles=None, always_ready=False):
        assert shape.as_shape().width <= 16, "DSP slice inputs have a maximum width of 16 bit."

        self.carry = carry
        self.taps = list(taps)
        self.shape = shape
        self.taps_shape = taps_shape or self.taps_shape()
        if shape_out is None:
            shape_out = self.compute_output_shape()
        self.shape_out = shape_out
        self.always_ready = always_ready
        self.symmetry = symmetry
        if input_delayed_cycles is None:
            self.input_delayed_cycles = len(self.taps)
        else:
            self.input_delayed_cycles = input_delayed_cycles

        signature = {
            "input":            In(stream.Signature(shape, always_ready=always_ready)),
            "input_delayed":    Out(shape),
            "output":           Out(stream.Signature(shape_out, always_ready=always_ready)),
        }
        if carry is not None:
            signature.update({
                "sum_carry": In(carry)
            })
        else:
            self.sum_carry = 0
        if symmetry:
            signature.update({
                "rev_input": In(shape),
                "rev_delayed": Out(shape),
            })
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

    def delay(self):
        return 1 + 1 + 3 + len(self.taps) - 1

    def elaborate(self, platform):
        m = Module()

        depth = len(self.taps)
        counter_in   = Signal(range(depth))
        counter_mult = Signal(range(depth))
        counter_out  = Signal(range(depth))
        dsp_ready = ~self.output.valid | self.output.ready

        window_valid = Signal()
        window_ready = dsp_ready
        multin_valid = Signal()


        input_ready = Signal()
        # Ready to process a sample either when the DSP slice is ready and the samples window is:
        # - Not valid yet.
        # - Only valid for 1 more cycle.
        m.d.comb += input_ready.eq(~window_valid | ((counter_in == depth-1) & window_ready))
        if not self.input.signature.always_ready:
            m.d.comb += self.input.ready.eq(input_ready)

        window = [ Signal.like(self.input.p, name=f"window_{i}") for i in range(max(depth, self.input_delayed_cycles)) ]

        # Sample window.
        with m.If(input_ready):
            m.d.sync += window_valid.eq(self.input.valid)
            with m.If(self.input.valid):
                m.d.sync += window[0].eq(self.input.p)
                for i in range(1, len(window)):
                    m.d.sync += window[i].eq(window[i-1])

        m.d.sync += multin_valid.eq(window_valid)

        dsp_a = Signal.like(self.input.p)
        with m.Switch(counter_in):
            for i in range(depth):
                with m.Case(i):
                    m.d.sync += dsp_a.eq(window[i])

        m.d.comb += self.input_delayed.eq(window[self.input_delayed_cycles-1])

        # Sample counter.
        with m.If(window_ready & window_valid):
            m.d.sync += counter_in.eq(_incr(counter_in, depth))

        # Symmetry handling.
        if self.symmetry:

            window_rev = [ Signal.like(self.input.p, name=f"window_rev_{i}") for i in range(depth) ]

            with m.If(input_ready & self.input.valid):
                m.d.sync += window_rev[0].eq(self.rev_input)
                m.d.sync += [ window_rev[i].eq(window_rev[i-1]) for i in range(1, len(window_rev)) ]
            
            m.d.comb += self.rev_delayed.eq(window_rev[-1])
            
            dsp_a_rev = Signal.like(self.input.p)
            with m.Switch(counter_in):
                for i in range(depth):
                    with m.Case(i):
                        m.d.sync += dsp_a_rev.eq(window_rev[depth-1-i])


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

        shape_out = self.compute_output_shape()

        if self.carry:
            sum_carry_q = Signal.like(self.sum_carry)
            with m.If(self.input.ready & self.input.valid):
                m.d.sync += sum_carry_q.eq(self.sum_carry)

        m.submodules.dsp = dsp = iCE40Multiplier()
        if self.symmetry:
            m.d.comb += dsp.a.eq(dsp_a + dsp_a_rev)
        else:
            m.d.comb += dsp.a.eq(dsp_a)
        m.d.comb += [
            dsp.b               .eq(coeff_rd.data),
            shape_out(dsp.p)    .eq(sum_carry_q if self.carry is not None else 0),
            dsp.valid_in        .eq(multin_valid & window_ready),
            dsp.p_load          .eq(counter_mult == 0),
            self.output.p       .eq(shape_out(dsp.o)),
            self.output.valid   .eq(dsp.valid_out & (counter_out == depth-1)),
        ]
        
        # Multiplier input and output counters.
        with m.If(dsp.valid_in):
            m.d.sync += counter_mult.eq(_incr(counter_mult, depth))
        with m.If(dsp.valid_out):
            m.d.sync += counter_out.eq(_incr(counter_out, depth))

        return m



class iCE40Multiplier(wiring.Component):

    a:          In(signed(16))
    b:          In(signed(16))
    valid_in:   In(1)

    p:          In(signed(32))
    p_load:     In(1)

    o:          Out(signed(32))
    valid_out:  Out(1)
   
    def elaborate(self, platform):
        m = Module()

        def pipe(signal, length):
            pipe = [ signal ] + [ Signal.like(signal, name=f"{signal.name}_q{i}") for i in range(length) ]
            for i in range(length):
                m.d.sync += pipe[i+1].eq(pipe[i])
            return pipe

        p_load_v    = Signal()

        dsp_delay   = 3
        valid_pipe  = pipe(self.valid_in, dsp_delay)
        m.d.comb   += p_load_v.eq(self.p_load & self.valid_in)
        p_pipe      = pipe(self.p, dsp_delay-1)
        p_load_pipe = pipe(p_load_v, dsp_delay - 1)
        m.d.comb   += self.valid_out.eq(valid_pipe[dsp_delay])

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
            mac.C           .eq(Mux(p_load_pipe[2], p_pipe[2][16:], self.o[16:])),
            mac.A           .eq(self.a),
            mac.B           .eq(self.b),
            mac.D           .eq(Mux(p_load_pipe[2], p_pipe[2][:16], self.o[:16])),
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
            
            # Outputs.
            self.o          .eq(mac.O),
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

    def _filter(self, dut, samples, count, num_channels=1, outfile=None, empty_cycles=0):

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

    def test_filter_serial(self):
        taps = [-1, 0, 9, 16, 9, 0, -1]
        taps = [ tap / 32 for tap in taps ]

        num_samples = 1024
        input_width = 8
        input_samples = self._generate_samples(num_samples, input_width)

        # Compute the expected result
        filtered_np = np.convolve(input_samples, taps).tolist()

        # Simulate DUT
        dut = FIRFilterSerialMAC16(taps, fixed.SQ(15, 0), always_ready=False)
        filtered = self._filter(dut, input_samples, len(input_samples))

        self.assertListEqual(filtered_np[:len(filtered)], filtered)

    def test_filter(self):
        taps = [-1, 0, 9, 16, 9, 0, -1]
        taps = [ tap / 32 for tap in taps ]

        num_samples = 1024
        input_width = 8
        input_samples = self._generate_samples(num_samples, input_width)

        # Compute the expected result
        filtered_np = np.convolve(input_samples, taps).tolist()

        # Simulate DUT
        dut = FIRFilterMAC16(taps, fixed.SQ(15, 0), always_ready=False)
        filtered = self._filter(dut, input_samples, len(input_samples))

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
                "sim_opts": dict(empty_cycles=3),
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
                "dut_options": dict(**common_dut_options, always_ready=False, num_channels=2, taps=taps0),
                "sim_opts": dict(empty_cycles=0),
            },

            "test_filter_with_backpressure_and_empty_cycles": {
                "num_samples": 1024,
                "dut_options": dict(**common_dut_options, num_channels=2, always_ready=False, taps=taps0),
                "sim_opts": dict(empty_cycles=3),
            },

            "test_filter_with_backpressure_taps1": {
                "num_samples": 1024,
                "dut_options": dict(**common_dut_options, num_channels=2, always_ready=False, taps=taps1),
                "sim_opts": dict(empty_cycles=0),
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
