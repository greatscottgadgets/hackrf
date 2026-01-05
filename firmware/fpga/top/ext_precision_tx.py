#
# This file is part of HackRF.
#
# Copyright (c) 2025 Great Scott Gadgets <info@greatscottgadgets.com>
# SPDX-License-Identifier: BSD-3-Clause

from amaranth               import Elaboratable, Module, Signal, Instance, Cat, ClockSignal, DomainRenamer
from amaranth.lib           import io, fifo, stream, wiring
from amaranth.lib.wiring    import Out, In, connect

from amaranth_future        import fixed

from board                  import PralinePlatform, ClockDomainGenerator
from interface              import MAX586xInterface
from interface.spi          import SPIRegisterInterface
from dsp.fir                import FIRFilter
from dsp.fir_mac16          import HalfBandInterpolatorMAC16
from dsp.cic                import CICInterpolator
from util                   import ClockConverter, IQSample, StreamSkidBuffer


class MCUInterface(wiring.Component):
    dac_stream: Out(stream.Signature(IQSample(12)))
    direction:  In(1)
    enable:     In(1)
    
    def __init__(self, domain="sync"):
        self._domain = domain
        super().__init__()

    def elaborate(self, platform):
        m = Module()

        dac_stream = self.dac_stream

        # Determine data transfer direction.
        direction = Signal()
        enable    = Signal()
        m.d.sync += enable.eq(self.enable)
        m.d.sync += direction.eq(self.direction)
        transfer_to_dac   = (direction == 1)

        # SGPIO clock and data lines.
        m.submodules.clk_out = clk_out = io.DDRBuffer("o", platform.request("host_clk", dir="-"), o_domain=self._domain)
        m.submodules.host_io = host_io = io.DDRBuffer('io', platform.request("host_data", dir="-"), i_domain=self._domain, o_domain=self._domain)

        # State machine to control SGPIO clock and data lines.
        tx_clk_en = Signal()
        m.d.sync += clk_out.o[0].eq(tx_clk_en)

        tx_dly_write = Signal(4)
        tx_in_sample = Signal(4*8)
        m.d.sync += tx_dly_write.eq(tx_dly_write << 1)
        m.d.sync += tx_in_sample.eq(Cat(host_io.i[1], tx_in_sample))

        # Small TX FIFO to avoid overflows from the write delay.
        m.submodules.tx_fifo = tx_fifo = fifo.SyncFIFOBuffered(width=24, depth=4)
        m.d.comb += [
            tx_fifo.w_data.word_select(0, 12)   .eq(tx_in_sample[20:32]),
            tx_fifo.w_data.word_select(1, 12)   .eq(tx_in_sample[4:16]),
            tx_fifo.w_en                        .eq(tx_dly_write[-1]),
            dac_stream.p                        .eq(tx_fifo.r_data),
            dac_stream.valid                    .eq(tx_fifo.r_rdy),
            tx_fifo.r_en                        .eq(dac_stream.ready),
        ]

        with m.FSM():
            with m.State("IDLE"):
                m.d.comb += tx_clk_en.eq(enable & transfer_to_dac & dac_stream.ready)

                with m.If(tx_clk_en):
                    m.next = "TX_I1"

            with m.State("TX_I1"):
                m.d.comb += tx_clk_en.eq(1)
                m.next = "TX_Q0"

            with m.State("TX_Q0"):
                m.d.comb += tx_clk_en.eq(1)
                m.next = "TX_Q1"

            with m.State("TX_Q1"):
                m.d.comb += tx_clk_en.eq(1)
                m.d.sync += tx_dly_write[0].eq(1)  # delayed write
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

        m.d.comb += adcdac_intf.dac_capture.eq(flow_ctl.dac_capture)
        m.d.comb += adcdac_intf.q_invert.eq(platform.request("q_invert").i)
        m.d.comb += mcu_intf.direction.eq(flow_ctl.direction)
        m.d.comb += mcu_intf.enable.eq(flow_ctl.enable)

        # Half-band filter taps.
        taps_hb1 = [-2, 0, 5, 0, -10, 0,18, 0, -30, 0,53, 0,-101, 0, 323, 512, 323, 0,-101, 0, 53, 0, -30, 0,18, 0, -10, 0, 5, 0,-2]
        taps_hb1 = [ tap/1024 for tap in taps_hb1 ]

        taps_hb2 = [3, 0, -16, 0, 77, 128, 77, 0, -16, 0, 3]
        taps_hb2 = [ tap/256 for tap in taps_hb2 ]

        tx_chain = {
            # Clock domain conversion.
            "clkconv":          ClockConverter(IQSample(12), 4, "sync", "gck1", always_ready=False),

            # Half-band interpolation stages (+ skid buffers for timing closure).
            "hbfir1":           HalfBandInterpolatorMAC16(taps_hb1, data_shape=fixed.SQ(11),
                overclock_rate=8, num_channels=2, always_ready=False, domain="gck1"),
            "skid1":            DomainRenamer("gck1")(StreamSkidBuffer(IQSample(12), always_ready=False)),
            "hbfir2":           HalfBandInterpolatorMAC16(taps_hb2, data_shape=fixed.SQ(11),
                overclock_rate=4, num_channels=2, always_ready=False, domain="gck1"),
            "skid2":            DomainRenamer("gck1")(StreamSkidBuffer(IQSample(12), always_ready=False)),

            # CIC interpolation stage.
            "cic_comp":         DomainRenamer("gck1")(FIRFilter([-0.125, 0, 0.75, 0, -0.125], shape=fixed.SQ(11), shape_out=fixed.SQ(11), always_ready=False, num_channels=2)),

            "cic_interpolator": CICInterpolator(2, 4, (4, 8, 16, 32), 12, 8, num_channels=2, 
                always_ready=False, domain="gck1"),
        }
        for k,v in tx_chain.items():
            m.submodules[f"tx_{k}"] = v

        # Connect transmitter chain.
        last = mcu_intf.dac_stream
        for block in tx_chain.values():
            connect(m, last, block.input)
            last = block.output
        connect(m, last, adcdac_intf.dac_stream)


        # SPI register interface.
        spi_port = platform.request("spi")
        m.submodules.spi_regs = spi_regs = SPIRegisterInterface(spi_port)

        # Add control registers.
        ctrl     = spi_regs.add_register(0x01, init=0)
        tx_intrp = spi_regs.add_register(0x02, init=0, size=3)

        m.d.comb += [
            # Trigger enable.
            flow_ctl.trigger_en                 .eq(ctrl[7]),

            # TX interpolation rate.
            tx_chain["cic_interpolator"].factor .eq(tx_intrp + 2),
        ]

        return m


if __name__ == "__main__":
    plat = PralinePlatform()
    plat.build(Top())
