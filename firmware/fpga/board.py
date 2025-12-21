#!/usr/bin/env python3
#
# This file is part of HackRF.
#
# Copyright (c) 2024 Great Scott Gadgets <info@greatscottgadgets.com>
# SPDX-License-Identifier: BSD-3-Clause

from amaranth                   import Elaboratable, Signal, Instance, Module, ClockDomain
from amaranth.build             import Resource, Pins, PinsN, Clock, Attrs
from amaranth.vendor            import LatticeICE40Platform
from amaranth_boards.resources  import SPIResource

__all__ = ["PralinePlatform", "ClockDomainGenerator"]


class PralinePlatform(LatticeICE40Platform):
    device      = "iCE40UP5K"
    package     = "SG48"

    default_clk = "SB_HFOSC"  # 48 MHz internal oscillator
    hfosc_div   = 0           # Do not divide

    resources   = [
        Resource("fpga_clk", 0, Pins("39", dir="i"),
            Attrs(GLOBAL=True, IO_STANDARD="SB_LVCMOS")),

        # ADC/DAC interfaces.
        Resource("afe_clk", 0, Pins("35", dir="i"),
            Clock(40e6), Attrs(GLOBAL=True, IO_STANDARD="SB_LVCMOS")),
        Resource("dd", 0, Pins("38 37 36 34 32 31 28 27 26 25", dir="o"),
            Attrs(IO_STANDARD="SB_LVCMOS")),
        Resource("da", 0, Pins("46 45 44 43 42 41 40 39", dir="i"),
            Attrs(IO_STANDARD="SB_LVCMOS")),
        
        # SGPIO interface.
        Resource("host_clk", 0, Pins("20", dir="o"),
            Attrs(IO_STANDARD="SB_LVCMOS")),
        Resource("host_data", 0, Pins("21 19 6 13 10 3 4 18", dir="io"),
            Attrs(IO_STANDARD="SB_LVCMOS")),
        Resource("direction", 0, Pins("12", dir="i"),
            Attrs(IO_STANDARD="SB_LVCMOS")),
        Resource("disable", 0, Pins("23", dir="i"),
            Attrs(IO_STANDARD="SB_LVCMOS")),
        Resource("capture_en", 0, Pins("11", dir="o"),
            Attrs(IO_STANDARD="SB_LVCMOS")),

        # Other I/O.
        Resource("q_invert", 0, Pins("9", dir="i"),
            Attrs(IO_STANDARD="SB_LVCMOS")),
        Resource("trigger_in", 0, Pins("48", dir="i"),
            Attrs(IO_STANDARD="SB_LVCMOS")),
        Resource("trigger_out", 0, Pins("2", dir="o"),
            Attrs(IO_STANDARD="SB_LVCMOS")),

        # SPI can be used after configuration.
        SPIResource("spi", 0, role="peripheral",
            cs_n="16", clk="15", copi="17", cipo="14",
            attrs=Attrs(IO_STANDARD="SB_LVCMOS"),
        ),
    ]

    connectors = []


class ClockDomainGenerator(Elaboratable):

    @staticmethod
    def lut_delay(m, signal, *, depth):
        signal_out = signal
        for i in range(depth):
            signal_in  = signal_out
            signal_out = Signal()
            m.submodules += Instance("SB_LUT4",
                p_LUT_INIT=0xAAAA,  # Buffer configuration
                i_I0=signal_in,
                o_O=signal_out,
            )
        return signal_out

    def elaborate(self, platform):
        m = Module()

        # Define clock domains.
        m.domains.gck1 = cd_gck1 = ClockDomain(name="gck1", reset_less=True)  # analog front-end clock.

        # We need to delay `gck1` clock by at least 8ns, not possible with the PLL alone.
        # Each LUT introduces a minimum propagation delay of 9ns (best case).
        delayed_gck1 = self.lut_delay(m, platform.request("afe_clk").i, depth=2)
        m.d.comb += cd_gck1.clk.eq(delayed_gck1)
        platform.add_clock_constraint(delayed_gck1, 40e6)

        return m
