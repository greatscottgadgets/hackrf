#
# This file is part of HackRF.
#
# Copyright (c) 2025 Great Scott Gadgets <info@greatscottgadgets.com>
# SPDX-License-Identifier: BSD-3-Clause

from math                   import floor, log2, ceil, comb

from amaranth               import Module, Signal, Const, signed, ResetInserter, DomainRenamer, C
from amaranth.utils         import bits_for

from amaranth.lib           import wiring, stream, data
from amaranth.lib.wiring    import In, Out, connect

from dsp.round              import convergent_round


class CICInterpolator(wiring.Component):
    def __init__(self, M, stages, rates, width_in, width_out=None, num_channels=1, always_ready=False, domain="sync"):
        self.M         = M
        self.stages    = stages
        self.rates     = rates
        self.width_in  = width_in
        self.num_channels = num_channels
        if width_out is None:
            width_out  = width_in + self.bit_growths()[-1]
        self.width_out = width_out
        self._domain = domain
        super().__init__({
            "input":  In(stream.Signature(
                data.ArrayLayout(signed(width_in), num_channels),
                always_ready=always_ready
            )),
            "output": Out(stream.Signature(
                data.ArrayLayout(signed(width_out), num_channels),
                always_ready=always_ready
            )),
            "factor": In(range(bits_for(max(rates)))),
        })

    def bit_growths(self):
        bit_growths = cic_growth(N=self.stages, M=self.M, R=max(self.rates))
        return bit_growths

    def elaborate(self, platform):
        m = Module()

        always_ready = self.output.signature.always_ready

        # Detect interpolation factor changes to provide an internal reset signal.
        factor_last = Signal.like(self.factor)
        factor_change = Signal()
        m.d.sync += factor_last.eq(self.factor)
        m.d.sync += factor_change.eq(factor_last != self.factor)
        factor_reset = ResetInserter(factor_change)

        # Calculated bit growths only used below for integrator stages.
        bit_growths = iter(self.bit_growths())

        stages = []

        # When M=1, we can replace the inner CIC stage with an equivalent zero-order hold integrator.
        inner_zoh = self.M == 1

        # Comb stages.
        width = self.width_in
        for i in range(self.stages - int(inner_zoh)):
            next_width = self.width_in + next(bit_growths)
            stage = factor_reset(CombStage(self.M, width, width_out=next_width, num_channels=self.num_channels, always_ready=always_ready))
            m.submodules[f"comb{i}"] = stage
            stages += [ stage ]
            width = next_width
        
        # Upsampling.
        if list(self.rates) != [1]:
            if inner_zoh:
                _ = next(bit_growths), next(bit_growths)  # drop comb and integrator growths
            stage = factor_reset(Upsampler(self.num_channels * width, max(self.rate), zero_order_hold=inner_zoh, variable=True, always_ready=always_ready))
            m.submodules["upsampler"] = stage
            m.d.sync += stage.factor.eq(1 << self.factor)
            stages += [ stage ]

        # Integrator stages.
        for i in range(self.stages - int(inner_zoh)):
            width_out = self.width_in + next(bit_growths)
            stage = SignExtend(width, width_out, num_channels=self.num_channels, always_ready=always_ready)
            m.submodules[f"signextend{i}"] = stage
            stages += [ stage ]
            stage = factor_reset(IntegratorStage(width_out, width_out, num_channels=self.num_channels, always_ready=always_ready))
            m.submodules[f"integrator{i}"] = stage
            stages += [ stage ]
            width = width_out
    
        # Variable gain stage.
        min_shift = self.width_in + cic_growth(N=self.stages, M=self.M, R=min(self.rates))[-1] - self.width_out  # at min rate
        shift_per_rate = { int(log2(rate)): min_shift + (self.stages-1)*i for i, rate in enumerate(self.rates) }
        stage = factor_reset(ProgrammableShift(width, width_out=self.width_out, num_channels=self.num_channels, shift_map=shift_per_rate, always_ready=always_ready))
        m.submodules["gain"] = stage
        if len(self.rates) > 1:
            m.d.sync += stage.shift.eq(self.factor)
        stages += [ stage ]
        width = self.width_out

        # Connect all stages to build the final filter.
        # For the upsampling CIC, we can only drop bits at the last stage.
        last = wiring.flipped(self.input)
        for stage in stages:
            connect(m, last, stage.input)
            last = stage.output
        connect(m, last, wiring.flipped(self.output))

        if self._domain != "sync":
            m = DomainRenamer(self._domain)(m)

        return m


class CICDecimator(wiring.Component):
    def __init__(self, M, stages, rates, width_in, width_out=None, num_channels=1, always_ready=False, domain="sync"):
        self.M            = M
        self.stages       = stages
        self.rates        = rates
        self.width_in     = width_in
        self.num_channels = num_channels
        self._domain      = domain
        if width_out is None:
            width_out    = width_in + ceil(stages * log2(max(rates) * M))
        self.width_out    = width_out
        super().__init__({
            "input":  In(stream.Signature(
                data.ArrayLayout(signed(width_in), num_channels),
                always_ready=always_ready
            )),
            "output": Out(stream.Signature(
                data.ArrayLayout(signed(width_out), num_channels),
                always_ready=always_ready
            )),
            "factor": In(range(bits_for(max(rates)))),
        })

    def truncation_summary(self):
        rates = min(self.rates)
        return cic_truncation(N=self.stages, R=rates, M=self.M, 
                              Bin=self.width_in, Bout=self.width_out)

    def elaborate(self, platform):
        m = Module()

        stages = []

        always_ready = self.output.signature.always_ready

        full_width = self.width_in + ceil(self.stages * log2(max(self.rates) * self.M))
        stage_widths = ( full_width - n for n in self.truncation_summary() )

        # Sign extend stage
        last_width = self.width_in
        stage_width = next(stage_widths)
        stage = SignExtend(last_width, stage_width, num_channels=self.num_channels, always_ready=always_ready)
        m.submodules["signextend"] = stage
        stages += [ stage ]
        last_width = stage_width

        # Integrator stages
        for i in range(self.stages):
            stage_width = next(stage_widths)
            stage = IntegratorStage(last_width, stage_width, num_channels=self.num_channels, always_ready=always_ready)
            m.submodules[f"integrator{i}"] = stage
            stages += [ stage ]
            last_width = stage_width
        
        # Downsampling
        if list(self.rates) != [1]:
            stage = Downsampler(self.num_channels * last_width, max(self.rates), variable=True, always_ready=always_ready)
            m.submodules["downsampler"] = stage
            m.d.sync += stage.factor.eq(1 << self.factor)
            stages += [ stage ]

        # Comb stages
        for i in range(self.stages):
            stage_width = next(stage_widths)
            stage = CombStage(self.M, last_width, stage_width, num_channels=self.num_channels, always_ready=always_ready)
            m.submodules[f"comb{i}"] = stage
            stages += [ stage ]
            last_width = stage_width

        # Gain stage

        # Ensure filter gain is at least the gain from width conversion.
        min_growth = ceil(self.stages * log2(min(self.rates) * self.M))
        if min_growth < self.width_out - self.width_in:
            growth = self.width_out - self.width_in - min_growth
            stage = WidthConverter(last_width, last_width+growth, num_channels=self.num_channels, always_ready=always_ready)
            m.submodules["gain0"] = stage
            stages += [ stage ]
            last_width = last_width + growth

        if len(self.rates) > 1:
            shift_per_rate = { int(log2(rate)): self.stages*i for i, rate in enumerate(self.rates) }
            stage = ProgrammableShift(last_width, width_out=self.width_out, num_channels=self.num_channels, shift_map=shift_per_rate, always_ready=always_ready)
            m.submodules["gain"] = stage
            m.d.sync += stage.shift.eq(self.factor)
            stages += [stage]
            last_width = self.width_out

        # Connect stages, rounding/truncating where needed
        last = wiring.flipped(self.input)
        for stage in stages:
            connect(m, last, stage.input)
            last = stage.output
        connect(m, last, wiring.flipped(self.output))

        if self._domain != "sync":
            m = DomainRenamer(self._domain)(m)

        return m


class ProgrammableShift(wiring.Component):
    def __init__(self, width_in, shift_map, width_out=None, num_channels=1, always_ready=False):
        self.num_channels = num_channels
        self.width_in = width_in
        self.width_out = width_out or width_in
        self.shift_map = shift_map
        if len(self.shift_map) == 1:
            self.shift = C(list(self.shift_map.keys())[0])
        super().__init__({
            "input":  In(stream.Signature(
                data.ArrayLayout(signed(self.width_in), num_channels),
                always_ready=always_ready
            )),
            "output": Out(stream.Signature(
                data.ArrayLayout(signed(self.width_out), num_channels),
                always_ready=always_ready
            )),
        } | ({"shift":  In(range(max(shift_map.keys())+1))} if len(shift_map)>1 else {}))

    def elaborate(self, platform):
        m = Module()

        # Implement the map itself (should it be done outside?)
        max_shift = max(self.shift_map.values())

        value_scaled = [ Signal(signed(self.width_in + max_shift)) for _ in range(self.num_channels) ]
        scaled_valid = Signal()
        scaled_ready = Signal()

        with m.If(~scaled_valid | scaled_ready):
            if not self.input.signature.always_ready:
                m.d.comb += self.input.ready.eq(1)
            m.d.sync += scaled_valid.eq(self.input.valid)
            with m.If(self.input.valid):
                for c in range(self.num_channels):
                    with m.Switch(self.shift):
                        for k, v in self.shift_map.items():
                            with m.Case(k):
                                m.d.sync += value_scaled[c].eq(self.input.payload[c] << (max_shift - v))
        
        with m.If(~self.output.valid | self.output.ready):
            m.d.comb += scaled_ready.eq(1)
            m.d.sync += self.output.valid.eq(scaled_valid)
            with m.If(scaled_valid):
                for c in range(self.num_channels):
                    if max_shift > 0:
                        # Convergent rounding / round to even.
                        m.d.sync += self.output.payload[c].eq(convergent_round(value_scaled[c], max_shift))
                        # Truncation.
                        #m.d.sync += self.output.payload[c].eq(value_scaled[c][max_shift:])
                    else:
                        m.d.sync += self.output.payload[c].eq(value_scaled[c])
        
        return m


class SignExtend(wiring.Component):
    def __init__(self, width_in, width_out, num_channels=1, always_ready=False):
        self.num_channels = num_channels
        self.always_ready = always_ready
        super().__init__({
            "input":  In(stream.Signature(
                data.ArrayLayout(signed(width_in), num_channels),
                always_ready=always_ready
            )),
            "output": Out(stream.Signature(
                data.ArrayLayout(signed(width_out), num_channels),
                always_ready=always_ready
            )),
        })

    def elaborate(self, platform):
        m = Module()
        for c in range(self.num_channels):
            m.d.comb += self.output.p[c].eq(self.input.p[c])
        m.d.comb += self.output.valid.eq(self.input.valid)
        if not self.always_ready:
            m.d.comb += self.input.ready.eq(self.output.ready)
        return m


class WidthConverter(wiring.Component):
    def __init__(self, width_in, width_out, num_channels=1, always_ready=False):
        self.width_in = width_in
        self.width_out = width_out
        self.num_channels = num_channels
        self.always_ready = always_ready
        super().__init__({
            "input":  In(stream.Signature(
                data.ArrayLayout(signed(width_in), num_channels),
                always_ready=always_ready
            )),
            "output": Out(stream.Signature(
                data.ArrayLayout(signed(width_out), num_channels),
                always_ready=always_ready
            )),
        })

    def elaborate(self, platform):
        m = Module()

        shift = self.width_out - self.width_in

        for c in range(self.num_channels):
            m.d.comb += self.output.p[c].eq(self.input.p[c] << shift)
        m.d.comb += self.output.valid.eq(self.input.valid)
        if not self.always_ready:
            m.d.comb += self.input.ready.eq(self.output.ready)
        return m


class CombStage(wiring.Component):
    def __init__(self, M, width_in, width_out=None, num_channels=1, always_ready=False):
        assert M in (1,2)
        self.M         = M
        self.width_in  = width_in
        self.width_out = width_out or width_in + 1
        self.num_channels = num_channels
        super().__init__({
            "input":  In(stream.Signature(
                data.ArrayLayout(signed(self.width_in), num_channels),
                always_ready=always_ready
            )),
            "output": Out(stream.Signature(
                data.ArrayLayout(signed(self.width_out), num_channels),
                always_ready=always_ready
            )),
        })
    
    def elaborate(self, platform):
        m = Module()

        shift = max(self.width_in - self.width_out, 0)
        delay = [ Signal.like(self.input.p) for _ in range(self.M) ]

        with m.If(~self.output.valid | self.output.ready):
            if not self.input.signature.always_ready:
                m.d.comb += self.input.ready.eq(1)
            m.d.sync += self.output.valid.eq(self.input.valid)
            with m.If(self.input.valid):
                m.d.sync += delay[0].eq(self.input.p)
                m.d.sync += [ delay[i].eq(delay[i-1]) for i in range(1, self.M) ]
                for c in range(self.num_channels):
                    diff = self.input.p[c] - delay[-1][c]
                    m.d.sync += self.output.p[c].eq(diff if shift == 0 else (diff >> shift))

        return m


class IntegratorStage(wiring.Component):
    def __init__(self, width_in, width_out, num_channels=1, always_ready=False):
        self.width_in = width_in
        self.width_out = width_out
        self.num_channels = num_channels
        super().__init__({
            "input":  In(stream.Signature(
                data.ArrayLayout(signed(width_in), num_channels),
                always_ready=always_ready
            )),
            "output": Out(stream.Signature(
                data.ArrayLayout(signed(width_out), num_channels),
                always_ready=always_ready
            )),
        })

    def elaborate(self, platform):
        m = Module()

        shift = max(self.width_in - self.width_out, 0)

        accumulator = Signal.like(self.input.p)
        for c in range(self.num_channels):
            m.d.comb += self.output.payload[c].eq(accumulator[c] if shift == 0 else (accumulator[c] >> shift))

        with m.If(~self.output.valid | self.output.ready):
            if not self.input.signature.always_ready:
                m.d.comb += self.input.ready.eq(1)
            m.d.sync += self.output.valid.eq(self.input.valid)
            with m.If(self.input.valid):
                for c in range(self.num_channels):
                    m.d.sync += accumulator[c].eq(accumulator[c] + self.input.payload[c])

        return m


class Upsampler(wiring.Component):
    def __init__(self, width, factor, zero_order_hold=False, variable=False, always_ready=False):
        self.width = width
        self.zoh   = zero_order_hold
        signature = {
            "input":  In(stream.Signature(width, always_ready=always_ready)),
            "output": Out(stream.Signature(width, always_ready=always_ready)),
        }
        if variable:
            signature.update({"factor": In(range(factor + 1))})
        else:
            self.factor = Const(factor)
        super().__init__(signature)

    def elaborate(self, platform):
        m = Module()

        counter = Signal.like(self.factor)
        ready_stb = Signal(init=1)
        if not self.input.signature.always_ready:
            m.d.comb += self.input.ready.eq(ready_stb)

        with m.If(~self.output.valid | self.output.ready):
            with m.If(counter == 0):
                m.d.sync += self.output.payload.eq(self.input.payload)
                m.d.sync += self.output.valid.eq(self.input.valid)
                with m.If(self.input.valid):
                    m.d.sync += counter.eq(self.factor - 1)
                    m.d.sync += ready_stb.eq(self.factor < 2)
            with m.Else():
                if not self.zoh:
                    m.d.sync += self.output.payload.eq(0)
                m.d.sync += self.output.valid.eq(1)
                m.d.sync += counter.eq(counter - 1)
                with m.If(counter == 1):
                    m.d.sync += ready_stb.eq(1)

        return m


class Downsampler(wiring.Component):
    def __init__(self, width, factor, variable=False, always_ready=False):
        signature = {
            "input":  In(stream.Signature(width, always_ready=always_ready)),
            "output": Out(stream.Signature(width, always_ready=always_ready)),
        }
        if variable:
            # TODO: optimize bit
            signature.update({"factor": In(range(factor + 1))})
        else:
            self.factor = Const(factor)
        super().__init__(signature)

    def elaborate(self, platform):
        m = Module()

        counter = Signal.like(self.factor)

        with m.If(self.input.ready & self.input.valid):
            with m.If(counter == 0):
                m.d.sync += counter.eq(self.factor - 1)
            with m.Else():
                m.d.sync += counter.eq(counter - 1)

        with m.If(self.output.ready | ~self.output.valid):
            if not self.input.signature.always_ready:
                m.d.comb += self.input.ready.eq(1)
            m.d.sync += self.output.valid.eq(self.input.valid & (counter == 0))
            with m.If(self.input.valid & (counter == 0)):
                m.d.sync += self.output.payload.eq(self.input.payload)

        return m


# Refs:
# [1]: Eugene Hogenauer, "An Economical Class of Digital Filters For Decimation and Interpolation,"
#      IEEE Trans. Acoust. Speech and Signal Proc., Vol. ASSP-29, April 1981, pp. 155-162.
# [2]: Rick Lyons, "Computing CIC filter register pruning using MATLAB"
#      https://www.dsprelated.com/showcode/269.php
# [3]: Peter Thorwartl, "Implementation of Filters", Part 3, lecture notes.
#      https://www.so-logic.net/documents/trainings/03_so_implementation_of_filters.pdf


# CIC downsamplers / decimators
# How much can we prune / truncate every stage output given a desired output width ?
# Calculate how many bits we can discard after each intermediate stage such that the quantization 
# error introduced is not greater than the one introduced by truncating/rounding at the filter 
# output.

def F_sq(N, R, M, i):
    assert i <= 2*N + 1
    if i == 2*N + 1: return 1  # eq. (16b) from [1]
    # h(k) and L (range of k), eq. (9b) from [1]
    if i <= N:
        # integrator stage
        L = N * (R * M - 1) + i - 1
        def h(k):
            return sum((-1)**l * comb(N, l) * comb(N-i+k-R*M*l, k-R*M*l)
                        for l in range(k//(R*M)+1))
    else:
        # comb stage
        L = 2*N + 1 - i
        def h(k):
            return (-1)**k * comb(2*N+1-i, k)
    # Compute standard deviation error gain from stage i to output
    F_i_sq = sum(h(k)**2 for k in range(L+1))
    return F_i_sq

def cic_truncation(N, R, M, Bin, Bout=None):
    full_width = Bin + ceil(N * log2(R * M))  # maximum width at output
    Bout = Bout or full_width                 # allow to specify full width
    B_last = full_width - Bout                # number of bits discarded at last stage
    t = log2(2**(2*B_last)/12) + log2(6 / N)  # Last two terms of (21) from [1]
    truncation = []
    for stage in range(2*N):
        ou = F_sq(N, R, M, stage+1)
        B_i = floor(0.5 * (-log2(ou) + t))    # Eq. (21) from [1]
        truncation.append(max(0, B_i))
    truncation.append(max(0, B_last))
    truncation[0] = 0  # [2]: fix case where input is truncated prior to any filtering
    return truncation

# CIC upsamplers / interpolators
# How much bit growth there is per intermediate stage?
# In the interpolator case, we cannot discard bits in intermediate stages: small errors in the 
# interpolator stages causes the variance of the error to grow without bound resulting in an 
# unstable filter.

def cic_growth(N, R, M):
    growths = []
    for i in range(1, 2*N+1):
        if i <= N:
            G_i = 2**i                             # comb stage
            # special case from [1] when differential delay is 1
            if M == 1 and i == N:
                G_i = 2**(N - 1)
        else:
            G_i = (2**(2*N-i) * (R*M)**(i-N)) / R  # integration stage
        growths.append(ceil(log2(G_i)))
    return growths





#
# Tests
#

import unittest
import numpy as np
from amaranth.sim import Simulator
from collections import namedtuple

class _TestFilter(unittest.TestCase):

    def _generate_samples(self, count, width, f_width=0):
        # Generate `count` random samples.
        rng = np.random.default_rng(0)
        samples = rng.normal(0, 1, count)

        # Convert to integer.
        samples = np.round(samples / max(abs(samples)) * (2**(width-1) - 1)).astype(int)
        assert max(samples) < 2**(width-1) and min(samples) >= -2**(width-1)  # sanity check

        if f_width > 0:
            return samples / (1 << f_width)
        return samples

    def _filter(self, dut, samples, count, oob=[], outfile=None):

        async def input_process(ctx):
            if hasattr(dut, "enable"):
                ctx.set(dut.enable, 1)
            for name, value in oob.items():
                ctx.set(getattr(dut, name), value)
            await ctx.tick()
            await ctx.tick()

            for sample in samples:
                ctx.set(dut.input.payload, [sample.item()])
                ctx.set(dut.input.valid, 1)
                await ctx.tick().until(dut.input.ready)
            ctx.set(dut.input.valid, 0)

        filtered = []
        async def output_process(ctx):
            if not dut.output.signature.always_ready:
                ctx.set(dut.output.ready, 1)
            async for clk, rst, valid, payload in ctx.tick().sample(dut.output.valid, dut.output.payload):
                if valid:
                    filtered.append(payload[0])
                if len(filtered) == count:
                    break

        sim = Simulator(dut)
        sim.add_clock(1/100e6)
        sim.add_testbench(input_process)
        sim.add_testbench(output_process)
        if outfile is not None:
            with sim.write_vcd(outfile):
                sim.run()
        else:
            sim.run()
        
        return filtered


class TestCICDecimator(_TestFilter):

    def test_filter(self):
        num_samples = 1024
        input_width = 8
        input_samples = self._generate_samples(num_samples, input_width)

        test = namedtuple('CICDecimatorTest', ['M', 'order', 'rates', 'factor_log', 'width_in', 'width_out', 'outfile'], defaults=(None,)*7)
        cic_tests = []

        # for different CIC orders...
        for o in [1,2,3,4]:
            # test signal with no rate change
            cic_tests.append(test(M=1, order=o, rates=(1,), factor_log=0, width_in=8, width_out=8))
            cic_tests.append(test(M=2, order=o, rates=(1,), factor_log=0, width_in=8, width_out=8))
            cic_tests.append(test(M=2, order=o, rates=(1,), factor_log=0, width_in=8, width_out=12))

            # test decimation by 4 with different M values and minimum decimation factors
            cic_tests.append(test(M=1, order=o, rates=(1, 2, 4, 8, 16, 32), factor_log=2, width_in=8, width_out=8))
            cic_tests.append(test(M=2, order=o, rates=(1, 2, 4, 8, 16, 32), factor_log=2, width_in=8, width_out=8))
            cic_tests.append(test(M=1, order=o, rates=(2, 4, 8, 16, 32), factor_log=2, width_in=8, width_out=8))
            cic_tests.append(test(M=2, order=o, rates=(2, 4, 8, 16, 32), factor_log=2, width_in=8, width_out=8))
            cic_tests.append(test(M=1, order=o, rates=(4, 8, 16, 32), factor_log=2, width_in=8, width_out=8))

            # different bit widths
            cic_tests.append(test(M=1, order=o, rates=(1, 2, 4, 8, 16, 32), factor_log=2, width_in=8, width_out=9))
            cic_tests.append(test(M=1, order=o, rates=(1, 2, 4, 8, 16, 32), factor_log=2, width_in=8, width_out=10))
            cic_tests.append(test(M=1, order=o, rates=(1, 2, 4, 8, 16, 32), factor_log=0, width_in=8, width_out=12))
            cic_tests.append(test(M=1, order=o, rates=(1, 2, 4, 8, 16, 32), factor_log=1, width_in=8, width_out=12))
            cic_tests.append(test(M=1, order=o, rates=(1, 2, 4, 8, 16, 32), factor_log=2, width_in=8, width_out=12))
            
            # test fixed decimation by 32
            cic_tests.append(test(M=1, order=o, rates=(32,), factor_log=5, width_in=8, width_out=8))


        for t in cic_tests:
            with self.subTest(t):
                factor_log = t.factor_log
                factor = 1 << factor_log
                cic_order = t.order
                M = t.M

                # Build taps by convolving boxcar filter repeatedly.
                taps0 = [1 for _ in range(factor*M)]
                taps = [1]
                for i in range(cic_order):
                    taps = np.convolve(taps, taps0)

                # Compute the expected result.
                cic_gain = (factor*M)**cic_order
                width_gain = 2**(t.width_out - t.width_in)
                filtered_np = np.convolve(input_samples, taps)
                filtered_np = filtered_np[::factor]                           # decimate
                filtered_np = np.round(filtered_np * width_gain // cic_gain)  # scale
                filtered_np = filtered_np.astype(np .int32).tolist()          # convert to python list

                # Simulate DUT
                dut = CICDecimator(M, cic_order, t.rates, t.width_in, t.width_out, always_ready=True)
                filtered = self._filter(dut, input_samples, len(input_samples)//factor, oob={"factor":factor_log}, outfile=t.outfile)

                # As we have some rounding error, we expect some samples to differ at most by 1
                max_diff = np.max(np.abs(np.array(filtered) - np.array(filtered_np[:len(filtered)])))
                
                self.assertLessEqual(max_diff, 1)
                #self.assertListEqual(filtered_np[:len(filtered)], filtered)


class TestCICInterpolator(_TestFilter):

    def test_filter(self):
        num_samples = 1024

        test = namedtuple('CICInterpolatorTest', ['M', 'order', 'rates', 'factor_log', 'width_in', 'width_out', 'outfile'], defaults=(None,)*7)
        cic_tests = []

        # for different CIC orders...
        for o in [1,2,3,4]:
            # test signal bypass
            cic_tests.append(test(M=1, order=o, rates=(1,), factor_log=0, width_in=8, width_out=8))
            cic_tests.append(test(M=1, order=o, rates=(1,), factor_log=0, width_in=12, width_out=8))

            # test interpolation by 4 with different M values and minimum interpolation factors
            cic_tests.append(test(M=1, order=o, rates=(1, 2, 4, 8, 16, 32), factor_log=2, width_in=8, width_out=8))
            cic_tests.append(test(M=2, order=o, rates=(1, 2, 4, 8, 16, 32), factor_log=2, width_in=8, width_out=8))
            cic_tests.append(test(M=1, order=o, rates=(2, 4, 8, 16, 32), factor_log=2, width_in=8, width_out=8))
            cic_tests.append(test(M=2, order=o, rates=(2, 4, 8, 16, 32), factor_log=2, width_in=8, width_out=8))
            cic_tests.append(test(M=1, order=o, rates=(4, 8, 16, 32), factor_log=2, width_in=8, width_out=8))

            # different bit widths
            cic_tests.append(test(M=1, order=o, rates=(1, 2, 4, 8, 16, 32), factor_log=2, width_in=12, width_out=8))
            cic_tests.append(test(M=2, order=o, rates=(1, 2, 4, 8, 16, 32), factor_log=2, width_in=12, width_out=8))
            cic_tests.append(test(M=1, order=o, rates=(2, 4, 8, 16, 32), factor_log=2, width_in=12, width_out=8))

            # test fixed interpolation by 32
            cic_tests.append(test(M=1, order=o, rates=(32,), factor_log=5, width_in=8, width_out=8))

            cic_tests.append(test(M=1, order=o, rates=(32,), factor_log=5, width_in=12, width_out=8))

        for t in cic_tests:
            with self.subTest(t):

                input_samples = self._generate_samples(num_samples, t.width_in)

                factor_log = t.factor_log
                factor = 1 << factor_log
                cic_order = t.order
                M = t.M

                # Build taps by convolving boxcar filter repeatedly.
                taps0 = [1 for _ in range(factor*M)]
                taps = [1]
                for i in range(cic_order):
                    taps = np.convolve(taps, taps0)

                # Compute the expected result
                cic_gain = (factor*M)**cic_order // factor
                width_gain = 2**(t.width_out - t.width_in)
                filtered_np = np.zeros(factor * num_samples)
                filtered_np[::factor] = input_samples
                filtered_np = np.convolve(filtered_np, taps)
                filtered_np = np.round(filtered_np * width_gain / cic_gain)         # scale
                filtered_np = filtered_np.astype(np.int32).tolist()  # convert to python list

                # Simulate DUT
                dut = CICInterpolator(M, cic_order, t.rates, t.width_in, t.width_out, always_ready=False)
                filtered = self._filter(dut, input_samples, len(input_samples)//factor, oob={"factor":factor_log}, outfile=t.outfile)
                
                self.assertListEqual(filtered_np[:len(filtered)], filtered)


if __name__ == "__main__":
    unittest.main()