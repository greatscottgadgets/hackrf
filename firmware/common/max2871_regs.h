#ifndef MAX2871_REGS_H
#define MAX2871_REGS_H
#include <stdint.h>

#define MAX2871_VASA    (1 << 9)

void max2871_regs_init(void);
uint32_t max2871_get_register(int reg);

void max2871_set_INT(uint32_t v);
void max2871_set_N(uint32_t v);
void max2871_set_FRAC(uint32_t v);
void max2871_set_CPL(uint32_t v);
void max2871_set_CPT(uint32_t v);
void max2871_set_P(uint32_t v);
void max2871_set_M(uint32_t v);
void max2871_set_LDS(uint32_t v);
void max2871_set_SDN(uint32_t v);
void max2871_set_MUX(uint32_t v);
void max2871_set_DBR(uint32_t v);
void max2871_set_RDIV2(uint32_t v);
void max2871_set_R(uint32_t v);
void max2871_set_REG4DB(uint32_t v);
void max2871_set_CP(uint32_t v);
void max2871_set_LDF(uint32_t v);
void max2871_set_LDP(uint32_t v);
void max2871_set_PDP(uint32_t v);
void max2871_set_SHDN(uint32_t v);
void max2871_set_TRI(uint32_t v);
void max2871_set_RST(uint32_t v);
void max2871_set_VCO(uint32_t v);
void max2871_set_VAS_SHDN(uint32_t v);
void max2871_set_VAS_TEMP(uint32_t v);
void max2871_set_CSM(uint32_t v);
void max2871_set_MUTEDEL(uint32_t v);
void max2871_set_CDM(uint32_t v);
void max2871_set_CDIV(uint32_t v);
void max2871_set_SDLDO(uint32_t v);
void max2871_set_SDDIV(uint32_t v);
void max2871_set_SDREF(uint32_t v);
void max2871_set_BS(uint32_t v);
void max2871_set_FB(uint32_t v);
void max2871_set_DIVA(uint32_t v);
void max2871_set_SDVCO(uint32_t v);
void max2871_set_MTLD(uint32_t v);
void max2871_set_BDIV(uint32_t v);
void max2871_set_RFB_EN(uint32_t v);
void max2871_set_BPWR(uint32_t v);
void max2871_set_RFA_EN(uint32_t v);
void max2871_set_APWR(uint32_t v);
void max2871_set_SDPLL(uint32_t v);
void max2871_set_F01(uint32_t v);
void max2871_set_LD(uint32_t v);
void max2871_set_ADCS(uint32_t v);
void max2871_set_ADCM(uint32_t v);
#endif
