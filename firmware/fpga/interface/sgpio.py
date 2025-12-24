#
# This file is part of HackRF.
#
# Copyright (c) 2025 Great Scott Gadgets <info@greatscottgadgets.com>
# SPDX-License-Identifier: BSD-3-Clause

from amaranth               import Module, Signal, DomainRenamer, EnableInserter, ClockSignal, Instance
from amaranth.lib           import io, fifo, stream, wiring, cdc
from amaranth.lib.wiring    import Out, In

from util                   import LinearFeedbackShiftRegister


class SGPIOInterface(wiring.Component):
    
    def __init__(self, sample_width=8, rx_assignments=None, tx_assignments=None, domain="sync"):
        self.sample_width = sample_width
        if rx_assignments is None:
            rx_assignments = _default_rx_assignments(sample_width // 8)
        if tx_assignments is None:
            tx_assignments = _default_tx_assignments(sample_width // 8)
        self.rx_assignments = rx_assignments
        self.tx_assignments = tx_assignments
        self._domain = domain
        super().__init__({
            "adc_stream": In(stream.Signature(sample_width, always_ready=True)),
            "dac_stream": Out(stream.Signature(sample_width)),
            "trigger_en": In(1),
            "prbs":       In(1),
        })

    def elaborate(self, platform):
        m = Module()

        adc_stream = self.adc_stream
        dac_stream = self.dac_stream
        rx_cycles = len(self.rx_assignments)
        tx_cycles = len(self.tx_assignments)

        direction_i = platform.request("direction").i
        enable_i    = ~platform.request("disable").i
        capture_en  = platform.request("capture_en").o
        m.d.comb += capture_en.eq(1)

        # Determine data transfer direction.
        direction  = Signal()
        m.submodules.direction_cdc = cdc.FFSynchronizer(direction_i, direction, o_domain=self._domain)
        transfer_from_adc = (direction == 0)

        # SGPIO clock and data lines.
        tx_clk_en      = Signal()
        rx_clk_en      = Signal()
        data_to_host   = Signal(self.sample_width)
        byte_to_host   = Signal(8)
        data_from_host = Signal(self.sample_width)
        byte_from_host = Signal(8)

        m.submodules.clk_out = clk_out = io.DDRBuffer("o", platform.request("host_clk", dir="-"), o_domain=self._domain)
        m.submodules.host_io = host_io = io.DDRBuffer('io', platform.request("host_data", dir="-"), i_domain=self._domain, o_domain=self._domain)

        m.d.sync += clk_out.o[0].eq(tx_clk_en)
        m.d.sync += clk_out.o[1].eq(rx_clk_en)
        m.d.sync += host_io.oe.eq(transfer_from_adc)
        m.d.comb += host_io.o[0].eq(byte_to_host)
        m.d.comb += host_io.o[1].eq(byte_to_host)
        m.d.comb += byte_from_host.eq(host_io.i[1])

        # Transmission is handled differently to account for the latency before the data 
        # becomes available in the FPGA fabric. 
        ddr_in_latency = 2  # for iCE40 DDR inputs in Amaranth.
        tx_write_latency = tx_cycles + ddr_in_latency
        tx_write_pipe = Signal(tx_write_latency)
        m.d.sync += tx_write_pipe.eq(tx_write_pipe << 1)
        for i in range(tx_cycles-1):  # don't store last byte
            with m.If(tx_write_pipe[ddr_in_latency + i]):
                m.d.sync += self.tx_assignments[i](data_from_host, byte_from_host)

        # Small TX FIFO to avoid missing samples when the consumer deasserts its ready
        # signal and transfers are in progress.
        m.submodules.tx_fifo = tx_fifo = fifo.SyncFIFOBuffered(width=self.sample_width, depth=16)
        m.d.comb += [
            tx_fifo.w_data      .eq(data_from_host),
            self.tx_assignments[-1](tx_fifo.w_data, byte_from_host),
            tx_fifo.w_en        .eq(tx_write_pipe[-1]),
            dac_stream.p        .eq(tx_fifo.r_data),
            dac_stream.valid    .eq(tx_fifo.r_rdy),
            tx_fifo.r_en        .eq(dac_stream.ready),
        ]

        # Pseudo-random binary sequence generator.
        prbs_advance = Signal()
        prbs_count = Signal(2)
        m.submodules.prbs = prbs = EnableInserter(prbs_advance)(
            LinearFeedbackShiftRegister(degree=8, taps=[8,6,5,4], init=0b10110001))


        # Capture signal generation.
        capture = Signal()
        m.submodules.trigger_gen = trigger_gen = FlowAndTriggerControl(domain=self._domain)
        m.d.comb += [
            trigger_gen.enable.eq(enable_i),
            trigger_gen.trigger_en.eq(self.trigger_en),
            capture.eq(trigger_gen.capture),
        ]


        # Main state machine.
        with m.FSM():
            with m.State("IDLE"): 

                with m.If(transfer_from_adc):
                    with m.If(self.prbs):
                        m.next = "PRBS"
                    with m.Elif(adc_stream.valid & capture):
                        m.d.comb += rx_clk_en.eq(1)
                        m.d.sync += data_to_host.eq(adc_stream.p)
                        m.d.sync += byte_to_host.eq(self.rx_assignments[0](adc_stream.p))
                        if rx_cycles > 1:
                            m.next = "RX0"
                with m.Else():
                    with m.If(dac_stream.ready & capture):
                        m.d.comb += tx_clk_en.eq(1)
                        m.d.sync += tx_write_pipe[0].eq(capture)
                        if tx_cycles > 1:
                            m.next = "TX0"

            for i in range(rx_cycles-1):
                with m.State(f"RX{i}"):
                    m.d.comb += rx_clk_en.eq(1)
                    m.d.sync += byte_to_host.eq(self.rx_assignments[i+1](data_to_host))
                    m.next = "IDLE" if i == rx_cycles-2 else f"RX{i+1}"

            for i in range(tx_cycles-1):
                with m.State(f"TX{i}"):
                    m.d.comb += tx_clk_en.eq(1)
                    m.next = "IDLE" if i == tx_cycles-2 else f"TX{i+1}"

            with m.State("PRBS"): 
                m.d.comb += rx_clk_en.eq(prbs_count == 0)
                m.d.comb += prbs_advance.eq(prbs_count == 0)
                m.d.sync += byte_to_host.eq(prbs.value)
                m.d.sync += prbs_count.eq(prbs_count + 1)
                with m.If(~self.prbs):
                    m.next = "IDLE"

        # Convert to other clock domain if necessary.
        if self._domain != "sync":
            m = DomainRenamer(self._domain)(m)

        return m


def _default_rx_assignments(n):
    def rx_assignment(i):
        def _f(w):
            return w.word_select(i, 8)
        return _f
    return [ rx_assignment(i) for i in range(n) ]

def _default_tx_assignments(n):
    def tx_assignment(i):
        def _f(w, v):
            return w.word_select(i, 8).eq(v)
        return _f
    return [ tx_assignment(i) for i in range(n) ]


class FlowAndTriggerControl(wiring.Component):
    trigger_en:  In(1)
    enable:      In(1)
    capture:     Out(1)

    def __init__(self, domain):
        super().__init__()
        self._domain = domain

    def elaborate(self, platform):
        m = Module()

        #
        # Signal synchronization and trigger logic.
        #
        trigger_enable = self.trigger_en
        trigger_in     = platform.request("trigger_in").i
        trigger_out    = platform.request("trigger_out").o
        m.d.comb += trigger_out.eq(self.enable)

        # Create a latch for the trigger input signal using a special FPGA primitive.
        trigger_in_latched = Signal()
        trigger_in_reg = Instance("SB_DFFES",
            i_D = 0,
            i_S = trigger_in,  # async set
            i_E = ~self.enable,
            i_C = ClockSignal(self._domain),
            o_Q = trigger_in_latched
        )
        m.submodules.trigger_in_reg = trigger_in_reg

        # Export signal for capture gating.
        m.d[self._domain] += self.capture.eq(self.enable & (trigger_in_latched | ~trigger_enable))

        return m
