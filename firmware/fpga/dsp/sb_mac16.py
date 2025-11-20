#
# This file is part of HackRF.
#
# Copyright (c) 2025 Great Scott Gadgets <info@greatscottgadgets.com>
# SPDX-License-Identifier: BSD-3-Clause

from amaranth               import Module, Instance, Signal, Mux, Cat
from amaranth.lib           import wiring
from amaranth.lib.wiring    import In, Out
from amaranth.vendor        import SiliconBluePlatform

class SB_MAC16(wiring.Component):

    # Input ports
    CLK:        In(1)
    CE:         In(1)
    C:          In(16)
    A:          In(16)
    B:          In(16)
    D:          In(16)
    AHOLD:      In(1)
    BHOLD:      In(1)
    CHOLD:      In(1)
    DHOLD:      In(1)
    IRSTTOP:    In(1)
    IRSTBOT:    In(1)
    ORSTTOP:    In(1)
    ORSTBOT:    In(1)
    OLOADTOP:   In(1)
    OLOADBOT:   In(1)
    ADDSUBTOP:  In(1)
    ADDSUBBOT:  In(1)
    OHOLDTOP:   In(1)
    OHOLDBOT:   In(1)
    CI:         In(1)
    ACCUMCI:    In(1)
    SIGNEXTIN:  In(1)
    
    # Output ports
    O:          Out(32)
    CO:         Out(1)
    ACCUMCO:    Out(1)
    SIGNEXTOUT: Out(1)


    def __init__(self,
                 NEG_TRIGGER=0,
                 C_REG=0,
                 A_REG=0,
                 B_REG=0,
                 D_REG=0,
                 TOP_8x8_MULT_REG=0,
                 BOT_8x8_MULT_REG=0,
                 PIPELINE_16x16_MULT_REG1=0,
                 PIPELINE_16x16_MULT_REG2=0,
                 TOPOUTPUT_SELECT=0,
                 TOPADDSUB_LOWERINPUT=0,
                 TOPADDSUB_UPPERINPUT=0,
                 TOPADDSUB_CARRYSELECT=0,
                 BOTOUTPUT_SELECT=0,
                 BOTADDSUB_LOWERINPUT=0,
                 BOTADDSUB_UPPERINPUT=0,
                 BOTADDSUB_CARRYSELECT=0,
                 MODE_8x8=0,
                 A_SIGNED=0,
                 B_SIGNED=0):

        super().__init__()

        # Parameters
        self.parameters = dict(
            NEG_TRIGGER=NEG_TRIGGER,
            C_REG=C_REG,
            A_REG=A_REG,
            B_REG=B_REG,
            D_REG=D_REG,
            TOP_8x8_MULT_REG=TOP_8x8_MULT_REG,
            BOT_8x8_MULT_REG=BOT_8x8_MULT_REG,
            PIPELINE_16x16_MULT_REG1=PIPELINE_16x16_MULT_REG1,
            PIPELINE_16x16_MULT_REG2=PIPELINE_16x16_MULT_REG2,
            TOPOUTPUT_SELECT=TOPOUTPUT_SELECT,
            TOPADDSUB_LOWERINPUT=TOPADDSUB_LOWERINPUT,
            TOPADDSUB_UPPERINPUT=TOPADDSUB_UPPERINPUT,
            TOPADDSUB_CARRYSELECT=TOPADDSUB_CARRYSELECT,
            BOTOUTPUT_SELECT=BOTOUTPUT_SELECT,
            BOTADDSUB_LOWERINPUT=BOTADDSUB_LOWERINPUT,
            BOTADDSUB_UPPERINPUT=BOTADDSUB_UPPERINPUT,
            BOTADDSUB_CARRYSELECT=BOTADDSUB_CARRYSELECT,
            MODE_8x8=MODE_8x8,
            A_SIGNED=A_SIGNED,
            B_SIGNED=B_SIGNED,
        )


    def elaborate(self, platform):
        if isinstance(platform, SiliconBluePlatform):
            return self.elaborate_hard_macro()
        else:
            return self.elaborate_simulation()


    def elaborate_hard_macro(self):
        m = Module()
        m.submodules.sb_mac16 = Instance("SB_MAC16",
            # Parameters.
            **{ f"p_{k}": v for k, v in self.parameters.items() },

            # Inputs.
            i_CLK=self.CLK,
            i_CE=self.CE,
            i_C=self.C,
            i_A=self.A,
            i_B=self.B,
            i_D=self.D,
            i_IRSTTOP=self.IRSTTOP,
            i_IRSTBOT=self.IRSTBOT,
            i_ORSTTOP=self.ORSTTOP,
            i_ORSTBOT=self.ORSTBOT,
            i_AHOLD=self.AHOLD,
            i_BHOLD=self.BHOLD,
            i_CHOLD=self.CHOLD,
            i_DHOLD=self.DHOLD,
            i_OHOLDTOP=self.OHOLDTOP,
            i_OHOLDBOT=self.OHOLDBOT,
            i_ADDSUBTOP=self.ADDSUBTOP,
            i_ADDSUBBOT=self.ADDSUBBOT,
            i_OLOADTOP=self.OLOADTOP,
            i_OLOADBOT=self.OLOADBOT,
            i_CI=self.CI,
            i_ACCUMCI=self.ACCUMCI,
            i_SIGNEXTIN=self.SIGNEXTIN,

            # Outputs.
            o_O=self.O,
            o_CO=self.CO,
            o_ACCUMCO=self.ACCUMCO,
            o_SIGNEXTOUT=self.SIGNEXTOUT,
        )
        return m


    def elaborate_simulation(self):
        m = Module()

        p = self.parameters
        
        assert p["NEG_TRIGGER"] == 0, "Falling edge input clock polarity not supported in simulation."
        
        # Internal wire, compare Figure on page 133 of ICE Technology Library 3.0 and Fig 2 on page 2 of Lattice TN1295-DSP
	    # http://www.latticesemi.com/~/media/LatticeSemi/Documents/TechnicalBriefs/SBTICETechnologyLibrary201608.pdf
	    # https://www.latticesemi.com/-/media/LatticeSemi/Documents/ApplicationNotes/AD/DSPFunctionUsageGuideforICE40Devices.ashx
        iA = Signal(16)
        iB = Signal(16)
        iC = Signal(16)
        iD = Signal(16)
        iF = Signal(16)
        iJ = Signal(16)
        iK = Signal(16)
        iG = Signal(16)
        iL = Signal(32)
        iH = Signal(32)
        iW = Signal(16)
        iX = Signal(16)
        iP = Signal(16)
        iQ = Signal(16)
        iY = Signal(16)
        iZ = Signal(16)
        iR = Signal(16)
        iS = Signal(16)
        HCI = Signal()
        LCI = Signal()
        LCO = Signal()
        
        # Registers
        rC = Signal(16)
        rA = Signal(16)
        rB = Signal(16)
        rD = Signal(16)
        rF = Signal(16)
        rJ = Signal(16)
        rK = Signal(16)
        rG = Signal(16)
        rH = Signal(32)
        rQ = Signal(16)
        rS = Signal(16)
        
        # Regs C and A
        with m.If(self.IRSTTOP):
            m.d.sync += [
                rC.eq(0),
                rA.eq(0),
            ]
        with m.Elif(self.CE):
            with m.If(~self.CHOLD):
                m.d.sync += rC.eq(self.C)
            with m.If(~self.AHOLD):
                m.d.sync += rA.eq(self.A)
        
        m.d.comb += [
            iC.eq(rC if p["C_REG"] else self.C),
            iA.eq(rA if p["A_REG"] else self.A),
        ]
        
        # Regs B and D
        with m.If(self.IRSTBOT):
            m.d.sync += [
                rB.eq(0),
                rD.eq(0)
            ]
        with m.Elif(self.CE):
            with m.If(~self.BHOLD):
                m.d.sync += rB.eq(self.B)
            with m.If(~self.DHOLD):
                m.d.sync += rD.eq(self.D)
        
        m.d.comb += [
            iB.eq(rB if p["B_REG"] else self.B),
            iD.eq(rD if p["D_REG"] else self.D),
        ]
        
        # Multiplier Stage
        p_Ah_Bh = Signal(16)
        p_Al_Bh = Signal(16)
        p_Ah_Bl = Signal(16)
        p_Al_Bl = Signal(16)
        Ah = Signal(16)
        Al = Signal(16)
        Bh = Signal(16)
        Bl = Signal(16)
        
        m.d.comb += [
            Ah.eq(Cat(iA[8:16], Mux(p["A_SIGNED"], iA[15].replicate(8), 0))),
            Al.eq(Cat(iA[0:8], Mux(p["A_SIGNED"] & p["MODE_8x8"], iA[7].replicate(8), 0))),
            Bh.eq(Cat(iB[8:16], Mux(p["B_SIGNED"], iB[15].replicate(8), 0))),
            Bl.eq(Cat(iB[0:8], Mux(p["B_SIGNED"] & p["MODE_8x8"], iB[7].replicate(8), 0))),
            p_Ah_Bh.eq(Ah * Bh),       # F
            p_Al_Bh.eq(Al[0:8] * Bh),  # J
            p_Ah_Bl.eq(Ah * Bl[0:8]),  # K
            p_Al_Bl.eq(Al * Bl),       # G
        ]
        
        # Regs F and J
        with m.If(self.IRSTTOP):
            m.d.sync += [
                rF.eq(0),
                rJ.eq(0)
            ]
        with m.Elif(self.CE):
            m.d.sync += rF.eq(p_Ah_Bh)
            if not p["MODE_8x8"]:
                m.d.sync += rJ.eq(p_Al_Bh)
        
        m.d.comb += [
            iF.eq(rF if p["TOP_8x8_MULT_REG"] else p_Ah_Bh),
            iJ.eq(rJ if p["PIPELINE_16x16_MULT_REG1"] else p_Al_Bh),
        ]
        
        # Regs K and G
        with m.If(self.IRSTBOT):
            m.d.sync += [
                rK.eq(0),
                rG.eq(0)
            ]
        with m.Elif(self.CE):
            with m.If(~p["MODE_8x8"]):
                m.d.sync += rK.eq(p_Ah_Bl)
            m.d.sync += rG.eq(p_Al_Bl)
        
        m.d.comb += [
            iK.eq(rK if p["PIPELINE_16x16_MULT_REG1"] else p_Ah_Bl),
            iG.eq(rG if p["BOT_8x8_MULT_REG"] else p_Al_Bl),
        ]
        
        # Adder Stage
        iK_e = Signal(24)
        iJ_e = Signal(24)
        m.d.comb += [
            iK_e.eq(Cat(iK, Mux(p["A_SIGNED"], iK[15].replicate(8), 0))),
            iJ_e.eq(Cat(iJ, Mux(p["B_SIGNED"], iJ[15].replicate(8), 0))),
            iL.eq(iG + (iK_e << 8) + (iJ_e << 8) + (iF << 16)),
        ]
        
        # Reg H
        with m.If(self.IRSTBOT):
            m.d.sync += rH.eq(0)
        with m.Elif(self.CE):
            if not p["MODE_8x8"]:
                m.d.sync += rH.eq(iL)
        
        m.d.comb += iH.eq(rH if p["PIPELINE_16x16_MULT_REG2"] else iL)
        
        # Hi Output Stage
        XW = Signal(17)
        Oh = Signal(16)
        
        m.d.comb += [
            iW.eq([iQ, iC][p["TOPADDSUB_UPPERINPUT"]]),
            iX.eq([iA, iF, iH[16:32], iZ[15].replicate(16)][p["TOPADDSUB_LOWERINPUT"]]),
            XW.eq(iX + (iW ^ self.ADDSUBTOP.replicate(16)) + HCI),
            self.ACCUMCO.eq(XW[16]),
            self.CO.eq(self.ACCUMCO ^ self.ADDSUBTOP),
            iP.eq(Mux(self.OLOADTOP, iC, XW[0:16] ^ self.ADDSUBTOP.replicate(16))),
        ]
        
        with m.If(self.ORSTTOP):
            m.d.sync += rQ.eq(0)
        with m.Elif(self.CE):
            with m.If(~self.OHOLDTOP):
                m.d.sync += rQ.eq(iP)
        
        m.d.comb += [
            iQ.eq(rQ),
            Oh.eq([iP, iQ, iF, iH[16:32]][p["TOPOUTPUT_SELECT"]]),
            HCI.eq([0, 1, LCO, LCO ^ self.ADDSUBBOT][p["TOPADDSUB_CARRYSELECT"]]),
            self.SIGNEXTOUT.eq(iX[15]),
        ]
        
        # Lo Output Stage
        YZ = Signal(17)
        Ol = Signal(16)
        
        m.d.comb += [
            iY.eq([iS, iD][p["BOTADDSUB_UPPERINPUT"]]),
            iZ.eq([iB, iG, iH[0:16], self.SIGNEXTIN.replicate(16)][p["BOTADDSUB_LOWERINPUT"]]),
            YZ.eq(iZ + (iY ^ self.ADDSUBBOT.replicate(16)) + LCI),
            LCO.eq(YZ[16]),
            iR.eq(Mux(self.OLOADBOT, iD, YZ[0:16] ^ self.ADDSUBBOT.replicate(16))),
        ]
        
        with m.If(self.ORSTBOT):
            m.d.sync += rS.eq(0)
        with m.Elif(self.CE):
            with m.If(~self.OHOLDBOT):
                m.d.sync += rS.eq(iR)
        
        m.d.comb += [
            iS.eq(rS),
            Ol.eq([iR, iS, iG, iH[0:16]][p["BOTOUTPUT_SELECT"]]),
            LCI.eq([0, 1, self.ACCUMCI, self.CI][p["BOTADDSUB_CARRYSELECT"]]),
            self.O.eq(Cat(Ol, Oh)),
        ]
        
        return m
