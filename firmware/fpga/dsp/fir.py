#
# This file is part of HackRF.
#
# Copyright (c) 2025 Great Scott Gadgets <info@greatscottgadgets.com>
# SPDX-License-Identifier: BSD-3-Clause

from math                   import ceil, log2

from amaranth               import Module, Signal, Mux, DomainRenamer
from amaranth.lib           import wiring, stream, data, memory, fifo
from amaranth.lib.wiring    import In, Out
from amaranth.utils         import bits_for

from amaranth_future        import fixed

from dsp.mcm                import ShiftAddMCM


class HalfBandDecimator(wiring.Component):
    def __init__(self, taps, data_shape, shape_out=None, always_ready=False, domain="sync"):
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
        m.submodules.fir = fir = FIRFilter(fir_taps, shape=self.data_shape, always_ready=always_ready, 
            num_channels=1, add_tap=len(fir_taps)//2+1)
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
                    with m.If(self.input.valid):
                        m.d.sync += self.output.payload.eq(self.input.payload)
                    if not self.input.signature.always_ready:
                        m.d.comb += self.input.ready.eq(1)

                with m.If(self.enable & switch_stb):
                    m.next = "DECIMATE"

            with m.State("DECIMATE"):

                # I and Q channels are muxed in time, demuxed later in the output stage.
                even_buffer = Signal.like(self.input.p)
                odd_buffer = Signal.like(self.input.p)
                q_valid = Signal()

                if not self.input.signature.always_ready:
                    m.d.comb += self.input.ready.eq(fir.input.ready)

                with m.If(self.input.ready & self.input.valid):
                    with m.If(~odd):
                        m.d.sync += even_buffer.eq(self.input.p)
                    with m.Else():
                        m.d.sync += odd_buffer.eq(self.input.p)
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
                        fir.input.p     .eq(odd_buffer[1]),
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
                        m.d.sync += self.output.p[1].eq(fir.output.p[0] * fixed.Const(0.5))

                # Mode switch logic
                with m.If(~self.enable & switch_stb):
                    m.d.sync += even_buffer.eq(0)
                    m.d.sync += odd_buffer.eq(0)
                    m.next = "BYPASS"

        if self._domain != "sync":
            m = DomainRenamer(self._domain)(m)

        return m


class HalfBandInterpolator(wiring.Component):
    def __init__(self, taps, data_shape, shape_out=None, always_ready=False, num_channels=1, domain="sync"):
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
        super().__init__({
            "input":  In(stream.Signature(
                data.ArrayLayout(data_shape, num_channels),
                always_ready=always_ready
            )),
            "output": Out(stream.Signature(
                data.ArrayLayout(shape_out, num_channels),
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
        m.submodules.fir = fir = FIRFilter(arm0_taps, shape=self.data_shape, shape_out=self.shape_out, always_ready=always_ready, num_channels=self.num_channels)
        m.submodules.dly = dly = Delay(delay, shape=self.data_shape, always_ready=always_ready, num_channels=self.num_channels)
        m.submodules.dly_fifo = dly_fifo = fifo.SyncFIFOBuffered(width=self.num_channels*self.data_shape.as_shape().width, depth=1)
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
                    with m.If(self.input.valid):
                        m.d.sync += self.output.payload.eq(self.input.payload)
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


class FIRFilter(wiring.Component):

    def __init__(self, taps, shape, shape_out=None, always_ready=False, num_channels=1, add_tap=None):
        self.taps = list(taps)
        self.add_tap = add_tap
        self.shape = shape
        if shape_out is None:
            shape_out = self.compute_output_shape()
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

    def taps_shape(self):
        taps_as_ratios  = [tap.as_integer_ratio() for tap in self.taps]
        f_width         = bits_for(max(tap[1] for tap in taps_as_ratios)) - 1
        i_width         = max(0, bits_for(max(abs(tap[0]) for tap in taps_as_ratios)) - f_width)
        return fixed.Shape(i_width, f_width, signed=any(tap < 0 for tap in self.taps))

    def compute_output_shape(self):
        taps_shape = self.taps_shape()
        signed     = self.shape.signed | taps_shape.signed
        f_width    = self.shape.f_width + taps_shape.f_width
        filter_gain = ceil(log2(sum(self.taps) + (1 if self.add_tap is not None else 0)))
        i_width    = max(0, self.shape.as_shape().width + taps_shape.as_shape().width - signed - f_width + filter_gain)
        return fixed.Shape(i_width, f_width, signed=signed)

    def elaborate(self, platform):
        m = Module()

        # Implement transposed-form FIR because it needs a smaller number of registers.

        # Helper function to create smaller size registers for fixed point ops.
        def fixed_reg(value, *args, **kwargs):
            reg = Signal.like(value.raw(), *args, **kwargs)
            return fixed.Value(value.shape(), reg)

        # Implement constant multipliers.
        terms = []
        for i, tap in enumerate(self.taps):
            tap_fixed = fixed.Const(tap)
            terms.append(tap_fixed._value)
        
        m.submodules.mcm = mcm = ShiftAddMCM(self.shape.as_shape().width, terms, num_channels=self.num_channels, always_ready=self.always_ready)
        wiring.connect(m, wiring.flipped(self.input), mcm.input)

        # Cast outputs to fixed point values.
        muls = []
        for i, tap in enumerate(self.taps):
            tap_fixed = fixed.Const(tap)
            muls.append([ fixed.Value.cast(mcm.output.p[c][f"{i}"], tap_fixed.f_width + self.shape.f_width) for c in range(self.num_channels) ])

        # Implement adder line.
        with m.If(~self.output.valid | self.output.ready):
            if not self.always_ready:
                m.d.comb += mcm.output.ready.eq(1)
            m.d.sync += self.output.valid.eq(mcm.output.valid)

        # Carry sum
        if self.add_tap is not None:
            add_input_q = Signal.like(self.add_input)
            m.d.sync += add_input_q.eq(self.add_input)

        for c in range(self.num_channels):

            accum = None
            for i, tap in enumerate(self.taps[::-1]):

                match (accum, tap):
                    case (None, 0): continue
                    case (None, _): value = muls[::-1][i][c]
                    case (_, 0):    value = accum
                    case (_, _):    value = muls[::-1][i][c] + accum

                if self.add_tap is not None:
                    if i == self.add_tap:
                        value += add_input_q[c]

                accum = fixed_reg(value, name=f"add_{c}_{i}")

                with m.If(mcm.output.valid & mcm.output.ready):
                    m.d.sync += accum.eq(value)

            m.d.comb += self.output.payload[c].eq(accum)

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
            init=(tuple(0 for _ in range(self.num_channels)) for _ in range(self.delay))
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


class TestFIRFilter(_TestFilter):

    def test_filter(self):
        taps = [-1, 0, 9, 16, 9, 0, -1]
        taps = [ tap / 32 for tap in taps ]

        num_samples = 1024
        input_width = 8
        input_samples = self._generate_samples(num_samples, input_width)

        # Compute the expected result
        filtered_np = np.convolve(input_samples, taps).tolist()

        # Simulate DUT
        dut = FIRFilter(taps, shape=fixed.SQ(8, 0), always_ready=False)
        filtered = self._filter(dut, input_samples, len(input_samples), empty_ready_cycles=5)

        self.assertListEqual(filtered_np[:len(filtered)], filtered)


class TestHalfBandDecimator(_TestFilter):

    def test_filter(self):

        common_dut_options = dict(
            data_shape=fixed.SQ(7),
            shape_out=fixed.SQ(0,31),
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
                dut = HalfBandDecimator(**scenario["dut_options"])
                filtered = self._filter(dut, zip(samples_i_in, samples_q_in), len(samples_i_in) // 2, num_channels=2, **scenario["sim_opts"])
                filtered_i = [ x[0] for x in filtered ]
                filtered_q = [ x[1] for x in filtered ]

                self.assertListEqual(filtered_i_np[:len(filtered_i)], filtered_i)
                self.assertListEqual(filtered_q_np[:len(filtered_q)], filtered_q)


class TestHalfBandInterpolator(_TestFilter):

    def test_filter(self):

        common_dut_options = dict(
            data_shape=fixed.SQ(7),
            shape_out=fixed.SQ(1,16),
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
                dut = HalfBandInterpolator(**scenario["dut_options"])
                filtered = self._filter(dut, zip(samples_i_in, samples_q_in), len(samples_i_in) * 2, num_channels=2, **scenario["sim_opts"])
                filtered_i = [ x[0] for x in filtered ]
                filtered_q = [ x[1] for x in filtered ]

                self.assertListEqual(filtered_i_np[:len(filtered_i)], filtered_i)
                self.assertListEqual(filtered_q_np[:len(filtered_q)], filtered_q)

if __name__ == "__main__":
    unittest.main()
