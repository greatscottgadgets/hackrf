#
# This file is part of HackRF.
#
# Copyright (c) 2025 Great Scott Gadgets <info@greatscottgadgets.com>
# SPDX-License-Identifier: BSD-3-Clause

from amaranth               import Elaboratable, Module, Signal, Mux, Instance, Cat, ClockSignal, DomainRenamer, EnableInserter
from amaranth.lib           import io, fifo, stream, wiring, cdc
from amaranth.lib.wiring    import Out, In, connect

from amaranth_future        import fixed

from board                  import PralinePlatform, ClockDomainGenerator
from interface              import MAX586xInterface
from interface.spi          import SPIRegisterInterface
from dsp.fir                import HalfBandDecimator, HalfBandInterpolator
from dsp.cic                import CICDecimator, CICInterpolator
from dsp.dc_block           import DCBlock
from dsp.quarter_shift      import QuarterShift
from dsp.nco                import NCO
from util                   import ClockConverter, IQSample, StreamSkidBuffer, LinearFeedbackShiftRegister


class MCUInterface(wiring.Component):
    adc_stream: In(stream.Signature(IQSample(8), always_ready=True))
    dac_stream: Out(stream.Signature(IQSample(8)))
    direction:  In(1)
    enable:     In(1)
    prbs:       In(1)
    
    def __init__(self, domain="sync"):
        self._domain = domain
        super().__init__()

    def elaborate(self, platform):
        m = Module()

        adc_stream = self.adc_stream
        dac_stream = self.dac_stream

        # Determine data transfer direction.
        direction = Signal()
        enable    = Signal()
        m.submodules.enable_cdc = cdc.FFSynchronizer(self.enable, enable, o_domain=self._domain)
        m.submodules.direction_cdc = cdc.FFSynchronizer(self.direction, direction, o_domain=self._domain)
        transfer_from_adc = (direction == 0)
        transfer_to_dac   = (direction == 1)

        # SGPIO clock and data lines.
        m.submodules.clk_out = clk_out = io.DDRBuffer("o", platform.request("host_clk", dir="-"), o_domain=self._domain)
        m.submodules.host_io = host_io = io.DDRBuffer('io', platform.request("host_data", dir="-"), i_domain=self._domain, o_domain=self._domain)

        # State machine to control SGPIO clock and data lines.
        tx_clk_en = Signal()
        rx_clk_en = Signal()
        m.d.sync += clk_out.o[0].eq(tx_clk_en)
        m.d.sync += clk_out.o[1].eq(rx_clk_en)
        m.d.sync += host_io.oe.eq(transfer_from_adc)

        data_to_host = Signal.like(adc_stream.p)
        m.d.comb += host_io.o[0].eq(data_to_host)
        m.d.comb += host_io.o[1].eq(data_to_host)

        tx_dly_write = Signal(3)
        host_io_prev_data = Signal(8)
        m.d.sync += tx_dly_write.eq(tx_dly_write << 1)
        m.d.sync += host_io_prev_data.eq(host_io.i[1])

        # Small TX FIFO to avoid overflows from the write delay.
        m.submodules.tx_fifo = tx_fifo = fifo.SyncFIFOBuffered(width=16, depth=8)
        m.d.comb += [
            tx_fifo.w_data      .eq(Cat(host_io_prev_data, host_io.i[1])),
            tx_fifo.w_en        .eq(tx_dly_write[-1]),
            dac_stream.p        .eq(tx_fifo.r_data),
            dac_stream.valid    .eq(tx_fifo.r_rdy),
            tx_fifo.r_en        .eq(dac_stream.ready),
        ]

        # Pseudo-random binary sequence generator.
        prbs_advance = Signal()
        prbs_count = Signal(2)
        m.submodules.prbs = prbs = EnableInserter(prbs_advance)(
            LinearFeedbackShiftRegister(degree=8, taps=[8,6,5,4], init=0b10110001))

        with m.FSM():
            with m.State("IDLE"):
                m.d.comb += tx_clk_en.eq(enable & transfer_to_dac & dac_stream.ready)
                m.d.comb += rx_clk_en.eq(enable & transfer_from_adc & adc_stream.valid)

                with m.If(self.prbs):
                    m.next = "PRBS"
                with m.Elif(rx_clk_en):
                    m.d.sync += data_to_host.eq(adc_stream.p)
                    m.next = "RX_Q"
                with m.Elif(tx_clk_en):
                    m.next = "TX_Q"

            with m.State("RX_Q"):
                m.d.comb += rx_clk_en.eq(1)
                m.d.sync += data_to_host.i.eq(data_to_host.q)
                m.next = "IDLE"

            with m.State("TX_Q"):
                m.d.comb += tx_clk_en.eq(1)
                m.d.sync += tx_dly_write[0].eq(1)  # delayed write
                m.next = "IDLE"

            with m.State("PRBS"):
                m.d.sync += host_io.oe.eq(1)
                m.d.sync += data_to_host.eq(prbs.value)
                m.d.comb += rx_clk_en.eq(prbs_count == 0)
                m.d.comb += prbs_advance.eq(prbs_count == 0)
                m.d.sync += prbs_count.eq(prbs_count + 1)
                with m.If(~self.prbs):
                    m.next = "IDLE"

        if self._domain != "sync":
            m = DomainRenamer(self._domain)(m)

        return m


class FlowAndTriggerControl(wiring.Component):
    trigger_en:  In(1)
    direction:   Out(1)  # async
    enable:      Out(1)  # async
    adc_capture: Out(1)
    dac_capture: Out(1)

    def __init__(self, domain):
        super().__init__()
        self._domain = domain

    def elaborate(self, platform):
        m = Module()

        #
        # Signal synchronization and trigger logic.
        #
        trigger_enable   = self.trigger_en
        trigger_in       =  platform.request("trigger_in").i
        trigger_out      =  platform.request("trigger_out").o
        host_data_enable = ~platform.request("disable").i
        m.d.comb += trigger_out.eq(host_data_enable)

        # Create a latch for the trigger input signal using a special FPGA primitive.
        trigger_in_latched = Signal()
        trigger_in_reg = Instance("SB_DFFES",
            i_D = 0,
            i_S = trigger_in,  # async set
            i_E = ~host_data_enable,
            i_C = ClockSignal(self._domain),
            o_Q = trigger_in_latched
        )
        m.submodules.trigger_in_reg = trigger_in_reg

        # Export signals for direction control and capture gating.
        m.d.comb += self.direction.eq(platform.request("direction").i)
        m.d.comb += self.enable.eq(host_data_enable)
        
        with m.If(host_data_enable):
            m.d[self._domain] += self.adc_capture.eq((trigger_in_latched | ~trigger_enable) & (self.direction == 0))
            m.d[self._domain] += self.dac_capture.eq((trigger_in_latched | ~trigger_enable) & (self.direction == 1))
        with m.Else():
            m.d[self._domain] += self.adc_capture.eq(0)
            m.d[self._domain] += self.dac_capture.eq(0)

        return m


class Top(Elaboratable):

    def elaborate(self, platform):
        m = Module()

        m.submodules.clkgen = ClockDomainGenerator()

        # Submodules.
        m.submodules.flow_ctl    = flow_ctl    = FlowAndTriggerControl(domain="gck1")
        m.submodules.adcdac_intf = adcdac_intf = MAX586xInterface(bb_domain="gck1")
        m.submodules.mcu_intf    = mcu_intf    = MCUInterface(domain="sync")

        m.d.comb += adcdac_intf.adc_capture.eq(flow_ctl.adc_capture)
        m.d.comb += adcdac_intf.dac_capture.eq(flow_ctl.dac_capture)
        m.d.comb += adcdac_intf.q_invert.eq(platform.request("q_invert").i)
        m.d.comb += mcu_intf.direction.eq(flow_ctl.direction)
        m.d.comb += mcu_intf.enable.eq(flow_ctl.enable)

        # Half-band filter taps.
        taps = [-2, 0, 7, 0, -18, 0, 41, 0, -92, 0, 320, 512, 320, 0, -92, 0, 41, 0, -18, 0, 7, 0, -2]
        taps = [ tap/1024 for tap in taps ]

        taps2 = [3, 0, -16, 0, 77, 128, 77, 0, -16, 0, 3]
        taps2 = [ tap/256 for tap in taps2 ]

        taps3 = [-9, 0, 73, 128, 73, 0, -9]
        taps3 = [ tap/256 for tap in taps3 ]

        taps4 = [-8, 0, 72, 128, 72, 0, -8]
        taps4 = [ tap/256 for tap in taps4 ]

        taps5 = [-1, 0, 9, 16, 9, 0, -1]
        taps5 = [ tap/32 for tap in taps5 ]

        common_rx_filter_opts = dict(
            data_shape=fixed.SQ(7),
            always_ready=True,
            domain="gck1",
        )

        rx_chain = {
            # DC block and quarter shift.
            "dc_block":      DCBlock(width=8, num_channels=2, domain="gck1"),
            "quarter_shift": DomainRenamer("gck1")(QuarterShift()),

            # Half-band decimation stages.
            "hbfir5":        HalfBandDecimator(taps5, **common_rx_filter_opts),
            "hbfir4":        HalfBandDecimator(taps4, **common_rx_filter_opts),
            "hbfir3":        HalfBandDecimator(taps3, **common_rx_filter_opts),
            "hbfir2":        HalfBandDecimator(taps2, **common_rx_filter_opts),
            "hbfir1":        HalfBandDecimator(taps, **common_rx_filter_opts),

            # Clock domain conversion.
            "clkconv":       ClockConverter(IQSample(8), 4, "gck1", "sync"),
        }
        for k,v in rx_chain.items():
            m.submodules[f"rx_{k}"] = v

        # Connect receiver chain.
        last = adcdac_intf.adc_stream
        for block in rx_chain.values():
            connect(m, last, block.input)
            last = block.output
        connect(m, last, mcu_intf.adc_stream)

        tx_chain = {
            # Clock domain conversion.
            "clkconv":          ClockConverter(IQSample(8), 4, "sync", "gck1", always_ready=False),

            # Half-band interpolation stages (+ skid buffers for timing closure).
            "hbfir1":           HalfBandInterpolator(taps, data_shape=fixed.SQ(7), 
                num_channels=2, always_ready=False, domain="gck1"),
            "skid2":            DomainRenamer("gck1")(StreamSkidBuffer(IQSample(8), always_ready=False)),
            "hbfir2":           HalfBandInterpolator(taps2, data_shape=fixed.SQ(7), 
                num_channels=2, always_ready=False, domain="gck1"),
            "skid3":            DomainRenamer("gck1")(StreamSkidBuffer(IQSample(8), always_ready=False)),

            # CIC interpolation stage.
            "cic_interpolator": CICInterpolator(1, 3, (1, 2, 4, 8), 8, 8, num_channels=2, 
                always_ready=False, domain="gck1"),
        }
        for k,v in tx_chain.items():
            m.submodules[f"tx_{k}"] = v

        # Connect transmitter chain.
        last = mcu_intf.dac_stream
        for block in tx_chain.values():
            connect(m, last, block.input)
            last = block.output
        # DAC can also be driven with an internal NCO.
        m.submodules.nco = nco = DomainRenamer("gck1")(NCO(phase_width=16, output_width=8))
        with m.If(nco.en):
            m.d.comb += [
                adcdac_intf.dac_stream.p.eq(nco.output),
                adcdac_intf.dac_stream.valid.eq(1),
                tx_chain["cic_interpolator"].output.ready.eq(1),
            ]
        with m.Else():
            connect(m, last, adcdac_intf.dac_stream)

        # SPI register interface.
        spi_port = platform.request("spi")
        m.submodules.spi_regs = spi_regs = SPIRegisterInterface(spi_port)

        # Add control registers.
        ctrl     = spi_regs.add_register(0x01, init=0)
        rx_decim = spi_regs.add_register(0x02, init=0, size=3)
        tx_ctrl  = spi_regs.add_register(0x03, init=0, size=1)
        tx_intrp = spi_regs.add_register(0x04, init=0, size=3)
        tx_pstep = spi_regs.add_register(0x05, init=0)

        m.d.sync += [
            # Trigger enable.
            flow_ctl.trigger_en                 .eq(ctrl[7]),

            # PRBS enable.
            mcu_intf.prbs                       .eq(ctrl[6]),

            # RX settings.
            rx_chain["dc_block"].enable         .eq(ctrl[0]),
            rx_chain["quarter_shift"].enable    .eq(ctrl[1]),
            rx_chain["quarter_shift"].up        .eq(ctrl[2]),

            # RX decimation rate.
            rx_chain["hbfir5"].enable           .eq(rx_decim > 4),
            rx_chain["hbfir4"].enable           .eq(rx_decim > 3),
            rx_chain["hbfir3"].enable           .eq(rx_decim > 2),
            rx_chain["hbfir2"].enable           .eq(rx_decim > 1),
            rx_chain["hbfir1"].enable           .eq(rx_decim > 0),

            # TX interpolation rate.
            tx_chain["cic_interpolator"].factor .eq(Mux(tx_intrp > 2, tx_intrp - 2, 0)),
            tx_chain["hbfir1"].enable           .eq(tx_intrp > 0),
            tx_chain["hbfir2"].enable           .eq(tx_intrp > 1),
        ]

        # TX NCO control.
        tx_pstep_gck1 = Signal(8)
        m.submodules.nco_phase_cdc = cdc.FFSynchronizer(tx_pstep, tx_pstep_gck1, o_domain="gck1")
        m.d.gck1 += [
            nco.en                              .eq(tx_ctrl[0]),
            nco.phase                           .eq(nco.phase + (tx_pstep_gck1 << 6)),
        ]

        return m


if __name__ == "__main__":
    plat = PralinePlatform()
    plat.build(Top())
