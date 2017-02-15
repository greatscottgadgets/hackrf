#include "max2871_regs.h"
#include <stdint.h>

static uint32_t registers[6];

void max2871_regs_init(void)
{
    registers[0] = 0x007D0000;
    registers[1] = 0x2000FFF9;
    registers[2] = 0x00004042;
    registers[3] = 0x0000000B;
    registers[4] = 0x6180B23C;
    registers[5] = 0x00400005;
}

uint32_t max2871_get_register(int reg)
{
    return registers[reg];
}

void max2871_set_INT(uint32_t v)
{
    registers[0] &= ~(0x1 << 31);
    registers[0] |= v << 31;
}

void max2871_set_N(uint32_t v)
{
    registers[0] &= ~(0xFFFF << 15);
    registers[0] |= v << 15;
}

void max2871_set_FRAC(uint32_t v)
{
    registers[0] &= ~(0xFFF << 3);
    registers[0] |= v << 3;
}

void max2871_set_CPL(uint32_t v)
{
    registers[1] &= ~(0x3 << 29);
    registers[1] |= v << 29;
}

void max2871_set_CPT(uint32_t v)
{
    registers[1] &= ~(0x3 << 27);
    registers[1] |= v << 27;
}

void max2871_set_P(uint32_t v)
{
    registers[1] &= ~(0xFFF << 15);
    registers[1] |= v << 15;
}

void max2871_set_M(uint32_t v)
{
    registers[1] &= ~(0xFFF << 3);
    registers[1] |= v << 3;
}

void max2871_set_LDS(uint32_t v)
{
    registers[2] &= ~(0x1 << 31);
    registers[2] |= v << 31;
}

void max2871_set_SDN(uint32_t v)
{
    registers[2] &= ~(0x3 << 29);
    registers[2] |= v << 29;
}

void max2871_set_MUX(uint32_t v)
{
    registers[2] &= ~(0x7 << 26);
    registers[5] &= ~(0x1 << 18);
    registers[2] |= (v & 0x7) << 26;
    registers[5] |= ((v & 0x8) >> 3) << 18;
}

void max2871_set_DBR(uint32_t v)
{
    registers[2] &= ~(0x1 << 25);
    registers[2] |= v << 25;
}

void max2871_set_RDIV2(uint32_t v)
{
    registers[2] &= ~(0x1 << 24);
    registers[2] |= v << 24;
}

void max2871_set_R(uint32_t v)
{
    registers[2] &= ~(0x3FF << 14);
    registers[2] |= v << 14;
}

void max2871_set_REG4DB(uint32_t v)
{
    registers[2] &= ~(0x1 << 13);
    registers[2] |= v << 13;
}

void max2871_set_CP(uint32_t v)
{
    registers[2] &= ~(0xF << 9);
    registers[2] |= v << 9;
}

void max2871_set_LDF(uint32_t v)
{
    registers[2] &= ~(0x1 << 8);
    registers[2] |= v << 8;
}

void max2871_set_LDP(uint32_t v)
{
    registers[2] &= ~(0x1 << 7);
    registers[2] |= v << 7;
}

void max2871_set_PDP(uint32_t v)
{
    registers[2] &= ~(0x1 << 6);
    registers[2] |= v << 6;
}

void max2871_set_SHDN(uint32_t v)
{
    registers[2] &= ~(0x1 << 5);
    registers[2] |= v << 5;
}

void max2871_set_TRI(uint32_t v)
{
    registers[2] &= ~(0x1 << 4);
    registers[2] |= v << 4;
}

void max2871_set_RST(uint32_t v)
{
    registers[2] &= ~(0x1 << 3);
    registers[2] |= v << 3;
}

void max2871_set_VCO(uint32_t v)
{
    registers[3] &= ~(0x3F << 26);
    registers[3] |= v << 26;
}

void max2871_set_VAS_SHDN(uint32_t v)
{
    registers[3] &= ~(0x1 << 25);
    registers[3] |= v << 25;
}

void max2871_set_VAS_TEMP(uint32_t v)
{
    registers[3] &= ~(0x1 << 24);
    registers[3] |= v << 24;
}

void max2871_set_CSM(uint32_t v)
{
    registers[3] &= ~(0x1 << 18);
    registers[3] |= v << 18;
}

void max2871_set_MUTEDEL(uint32_t v)
{
    registers[3] &= ~(0x1 << 17);
    registers[3] |= v << 17;
}

void max2871_set_CDM(uint32_t v)
{
    registers[3] &= ~(0x3 << 15);
    registers[3] |= v << 15;
}

void max2871_set_CDIV(uint32_t v)
{
    registers[3] &= ~(0xFFF << 3);
    registers[3] |= v << 3;
}

void max2871_set_SDLDO(uint32_t v)
{
    registers[4] &= ~(0x1 << 28);
    registers[4] |= v << 28;
}

void max2871_set_SDDIV(uint32_t v)
{
    registers[4] &= ~(0x1 << 27);
    registers[4] |= v << 27;
}

void max2871_set_SDREF(uint32_t v)
{
    registers[4] &= ~(0x1 << 26);
    registers[4] |= v << 26;
}

void max2871_set_BS(uint32_t v)
{
    registers[4] &= ~(0x3 << 24);
    registers[4] &= ~(0xFF << 12);
    registers[4] |= ((v & 0x300) >> 8) << 24;
    registers[4] |= (v & 0xFF) << 12;
}

void max2871_set_FB(uint32_t v)
{
    registers[4] &= ~(0x1 << 23);
    registers[4] |= v << 23;
}

void max2871_set_DIVA(uint32_t v)
{
    registers[4] &= ~(0x7 << 20);
    registers[4] |= v << 20;
}

void max2871_set_SDVCO(uint32_t v)
{
    registers[4] &= ~(0x1 << 11);
    registers[4] |= v << 11;
}

void max2871_set_MTLD(uint32_t v)
{
    registers[4] &= ~(0x1 << 10);
    registers[4] |= v << 10;
}

void max2871_set_BDIV(uint32_t v)
{
    registers[4] &= ~(0x1 << 9);
    registers[4] |= v << 9;
}

void max2871_set_RFB_EN(uint32_t v)
{
    registers[4] &= ~(0x1 << 8);
    registers[4] |= v << 8;
}

void max2871_set_BPWR(uint32_t v)
{
    registers[4] &= ~(0x3 << 6);
    registers[4] |= v << 6;
}

void max2871_set_RFA_EN(uint32_t v)
{
    registers[4] &= ~(0x1 << 5);
    registers[4] |= v << 5;
}

void max2871_set_APWR(uint32_t v)
{
    registers[4] &= ~(0x3 << 3);
    registers[4] |= v << 3;
}

void max2871_set_SDPLL(uint32_t v)
{
    registers[5] &= ~(0x1 << 25);
    registers[5] |= v << 25;
}

void max2871_set_F01(uint32_t v)
{
    registers[5] &= ~(0x1 << 24);
    registers[5] |= v << 24;
}

void max2871_set_LD(uint32_t v)
{
    registers[5] &= ~(0x3 << 22);
    registers[5] |= v << 22;
}

void max2871_set_ADCS(uint32_t v)
{
    registers[5] &= ~(0x1 << 6);
    registers[5] |= v << 6;
}

void max2871_set_ADCM(uint32_t v)
{
    registers[5] &= ~(0x7 << 3);
    registers[5] |= v << 3;
}
