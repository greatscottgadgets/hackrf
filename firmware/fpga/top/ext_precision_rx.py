#
# This file is part of HackRF.
#
# Copyright (c) 2025 Great Scott Gadgets <info@greatscottgadgets.com>
# SPDX-License-Identifier: BSD-3-Clause

from amaranth               import Elaboratable, Module, Signal, Mux, Instance, Cat, ClockSignal, DomainRenamer
from amaranth.lib           import io, fifo, stream, wiring
from amaranth.lib.wiring    import Out, In, connect

from amaranth_future        import fixed

from board                  import PralinePlatform, ClockDomainGenerator
from interface              import MAX586xInterface
from interface.spi          import SPIRegisterInterface
from dsp.fir                import FIRFilter
from dsp.fir_mac16          import HalfBandDecimatorMAC16
from dsp.cic                import CICDecimator
from dsp.dc_block           import DCBlock
from dsp.quarter_shift      import QuarterShift
from util                   import ClockConverter, IQSample


class MCUInterface(wiring.Component):
    adc_stream: In(stream.Signature(IQSample(12), always_ready=True))
    direction:  In(1)
    enable:     In(1)
    
    def __init__(self, domain="sync"):
        self._domain = domain
        super().__init__()

    def elaborate(self, platform):
        m = Module()

        adc_stream = self.adc_stream

        # Determine data transfer direction.
        direction = Signal()
        enable    = Signal()
        m.d.sync += enable.eq(self.enable)
        m.d.sync += direction.eq(self.direction)
        transfer_from_adc = (direction == 0)

        # SGPIO clock and data lines.
        m.submodules.clk_out = clk_out = io.DDRBuffer("o", platform.request("host_clk", dir="-"), o_domain=self._domain)
        m.submodules.host_io = host_io = io.DDRBuffer('io', platform.request("host_data", dir="-"), i_domain=self._domain, o_domain=self._domain)

        # State machine to control SGPIO clock and data lines.
        rx_clk_en = Signal()
        m.d.sync += clk_out.o[1].eq(rx_clk_en)
        m.d.sync += host_io.oe.eq(transfer_from_adc)

        data_to_host = Signal.like(adc_stream.p)
        rx_data_buffer = Signal(8)
        m.d.comb += host_io.o[0].eq(rx_data_buffer)
        m.d.comb += host_io.o[1].eq(rx_data_buffer)

        with m.FSM():
            with m.State("IDLE"):
                m.d.comb += rx_clk_en.eq(enable & transfer_from_adc & adc_stream.valid)

                with m.If(rx_clk_en):
                    m.d.sync += rx_data_buffer.eq(adc_stream.p.i >> 8)
                    m.d.sync += data_to_host.eq(adc_stream.p)
                    m.next = "RX_I1"

            with m.State("RX_I1"):
                m.d.comb += rx_clk_en.eq(1)
                m.d.sync += rx_data_buffer.eq(data_to_host.i)
                m.next = "RX_Q0"

            with m.State("RX_Q0"):
                m.d.comb += rx_clk_en.eq(1)
                m.d.sync += rx_data_buffer.eq(data_to_host.q >> 8)
                m.next = "RX_Q1"

            with m.State("RX_Q1"):
                m.d.comb += rx_clk_en.eq(1)
                m.d.sync += rx_data_buffer.eq(data_to_host.q)
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
        taps_hb1 = [-2, 0, 5, 0, -10, 0,18, 0, -30, 0,53, 0,-101, 0, 323, 512, 323, 0,-101, 0, 53, 0, -30, 0,18, 0, -10, 0, 5, 0,-2]
        taps_hb1 = [ tap/1024 for tap in taps_hb1 ]

        taps_hb2 = [ -6, 0, 19,  0, -44,  0, 89,  0, -163,  0, 278,  0, -452,  0, 711,  0, -1113,  0, 1800, 0, -3298,  0, 10370, 16384, 10370,  0, -3298,  0, 1800,  0, -1113,  0, 711,  0, -452,  0,  278,  0, -163,  0, 89,  0, -44,  0, 19,  0, -6]
        taps_hb2 = [ tap/16384/2 for tap in taps_hb2 ]

        rx_chain = {
            # DC block and quarter shift.
            "dc_block":      DCBlock(width=8, num_channels=2, domain="gck1"),
            "quarter_shift": DomainRenamer("gck1")(QuarterShift()),

            # CIC mandatory first stage with compensator.
            "cic":          CICDecimator(2, 4, (4,8,16,32), width_in=8, width_out=12, num_channels=2, always_ready=True, domain="gck1"),
            "cic_comp":     DomainRenamer("gck1")(FIRFilter([-0.125, 0, 0.75, 0, -0.125], shape=fixed.SQ(11), shape_out=fixed.SQ(11), always_ready=True, num_channels=2)),

            # Final half-band decimator stages.
            "hbfir1":       HalfBandDecimatorMAC16(taps_hb1, data_shape=fixed.SQ(11), overclock_rate=4, always_ready=True, domain="gck1"),
            "hbfir2":       HalfBandDecimatorMAC16(taps_hb2, data_shape=fixed.SQ(11), overclock_rate=8, always_ready=True, domain="gck1"),

            # Clock domain conversion.
            "clkconv":      ClockConverter(IQSample(12), 4, "gck1", "sync", always_ready=True),
        }
        for k,v in rx_chain.items():
            m.submodules[f"rx_{k}"] = v

        # Connect receiver chain.
        last = adcdac_intf.adc_stream
        for block in rx_chain.values():
            connect(m, last, block.input)
            last = block.output
        connect(m, last, mcu_intf.adc_stream)

        # SPI register interface.
        spi_port = platform.request("spi")
        m.submodules.spi_regs = spi_regs = SPIRegisterInterface(spi_port)

        # Add control registers.
        ctrl     = spi_regs.add_register(0x01, init=0)
        rx_decim = spi_regs.add_register(0x02, init=0, size=3)
        #tx_intrp = spi_regs.add_register(0x04, init=0, size=3)

        m.d.comb += [
            # Trigger enable.
            flow_ctl.trigger_en                 .eq(ctrl[7]),

            # RX settings.
            rx_chain["dc_block"].enable         .eq(ctrl[0]),
            rx_chain["quarter_shift"].enable    .eq(ctrl[1]),
            rx_chain["quarter_shift"].up        .eq(ctrl[2]),

            # RX decimation rate.
            rx_chain["cic"].factor              .eq(rx_decim+2),
        ]

        return m


if __name__ == "__main__":
    plat = PralinePlatform()
    plat.build(Top())
