#include <libopencm3/cm3/common.h>
#include <libopencm3/lpc43xx/memorymap.h>

#define SCT_EVENT_COUNT 16

#undef SCT_DITHER_ENGINE_EN

/*
SCT Dither Engine only registers and fields:
EV[0:15]_CTRL[MATCHMEM, DIRECTION]
*/

/* --- SCT_CONFIG ---------------------------------------- */
#define SCT_CONFIG MMIO32(SCT_BASE + 0x00)

/* -- SCT_CONFIG_UNIFY: SCT operation */
#define SCT_CONFIG_UNIFY_SHIFT (0)
#define SCT_CONFIG_UNIFY_MASK (0x01 << SCT_CONFIG_UNIFY_SHIFT)
#define SCT_CONFIG_UNIFY(x) ((x) << SCT_CONFIG_UNIFY_SHIFT)

/* SCT_CONFIG_UNIFY_UNIFY values */
#define SCT_CONFIG_UNIFY_16_BIT SCT_CONFIG_UNIFY(0x00)  /* 16-bit.The SCT operates as two 16-bit counters named L and H. */
#define SCT_CONFIG_UNIFY_32_BIT SCT_CONFIG_UNIFY(0x01)  /* 32-bit. The SCT operates as a unified 32-bit counter. */

/* -- SCT_CONFIG_CLKMODE: SCT clock mode */
#define SCT_CONFIG_CLKMODE_SHIFT (1)
#define SCT_CONFIG_CLKMODE_MASK (0x03 << SCT_CONFIG_CLKMODE_SHIFT)
#define SCT_CONFIG_CLKMODE(x) ((x) << SCT_CONFIG_CLKMODE_SHIFT)

/* SCT_CONFIG_CLKMODE_CLKMODE values */
#define SCT_CONFIG_CLKMODE_BUS_CLOCK SCT_CONFIG_CLKMODE(0x00)  /* Bus clock. The bus clock clocks the SCT and prescalers. */
#define SCT_CONFIG_CLKMODE_PRESCALED_BUS_CLOCK SCT_CONFIG_CLKMODE(0x01)  /* Prescaled bus clock. The SCT clock is the bus clock, but the prescalers are  enabled to count only when sampling of the input selected by  the CKSEL field finds the selected edge. The minimum pulse  width on the clock input is 1 bus clock period. This mode is the high-performance  sampled-clock mode. */
#define SCT_CONFIG_CLKMODE_SCT_INPUT SCT_CONFIG_CLKMODE(0x02)  /* SCT Input. The input selected by  CKSEL clocks the SCT and prescalers. The input is synchronized to the bus clock and possibly inverted.  The minimum pulse width on the clock input is 1 bus clock  period. This mode is the low-power sampled-clock mode. */
#define SCT_CONFIG_CLKMODE_RESERVED SCT_CONFIG_CLKMODE(0x03)  /* Reserved. */

/* -- SCT_CONFIG_CKSEL: SCT clock select */
#define SCT_CONFIG_CKSEL_SHIFT (3)
#define SCT_CONFIG_CKSEL_MASK (0x0F << SCT_CONFIG_CKSEL_SHIFT)
#define SCT_CONFIG_CKSEL(x) ((x) << SCT_CONFIG_CKSEL_SHIFT)

/* SCT_CONFIG_CKSEL_CKSEL values */
#define SCT_CONFIG_CKSEL_RISING_EDGES_ON_INPUT_0 SCT_CONFIG_CKSEL(0x00)  /* Rising edges on input 0. */
#define SCT_CONFIG_CKSEL_FALLING_EDGES_ON_INPUT_0 SCT_CONFIG_CKSEL(0x01)  /* Falling edges on input 0. */
#define SCT_CONFIG_CKSEL_RISING_EDGES_ON_INPUT_1 SCT_CONFIG_CKSEL(0x02)  /* Rising edges on input 1. */
#define SCT_CONFIG_CKSEL_FALLING_EDGES_ON_INPUT_1 SCT_CONFIG_CKSEL(0x03)  /* Falling edges on input 1. */
#define SCT_CONFIG_CKSEL_RISING_EDGES_ON_INPUT_2 SCT_CONFIG_CKSEL(0x04)  /* Rising edges on input 2. */
#define SCT_CONFIG_CKSEL_FALLING_EDGES_ON_INPUT_2 SCT_CONFIG_CKSEL(0x05)  /* Falling edges on input 2. */
#define SCT_CONFIG_CKSEL_RISING_EDGES_ON_INPUT_3 SCT_CONFIG_CKSEL(0x06)  /* Rising edges on input 3. */
#define SCT_CONFIG_CKSEL_FALLING_EDGES_ON_INPUT_3 SCT_CONFIG_CKSEL(0x07)  /* Falling edges on input 3. */
#define SCT_CONFIG_CKSEL_RISING_EDGES_ON_INPUT_4 SCT_CONFIG_CKSEL(0x08)  /* Rising edges on input 4. */
#define SCT_CONFIG_CKSEL_FALLING_EDGES_ON_INPUT_4 SCT_CONFIG_CKSEL(0x09)  /* Falling edges on input 4. */
#define SCT_CONFIG_CKSEL_RISING_EDGES_ON_INPUT_5 SCT_CONFIG_CKSEL(0x0A)  /* Rising edges on input 5. */
#define SCT_CONFIG_CKSEL_FALLING_EDGES_ON_INPUT_5 SCT_CONFIG_CKSEL(0x0B)  /* Falling edges on input 5. */
#define SCT_CONFIG_CKSEL_RISING_EDGES_ON_INPUT_6 SCT_CONFIG_CKSEL(0x0C)  /* Rising edges on input 6. */
#define SCT_CONFIG_CKSEL_FALLING_EDGES_ON_INPUT_6 SCT_CONFIG_CKSEL(0x0D)  /* Falling edges on input 6. */
#define SCT_CONFIG_CKSEL_RISING_EDGES_ON_INPUT_7 SCT_CONFIG_CKSEL(0x0E)  /* Rising edges on input 7. */
#define SCT_CONFIG_CKSEL_FALLING_EDGES_ON_INPUT_7 SCT_CONFIG_CKSEL(0x0F)  /* Falling edges on input 7. */

/* -- SCT_CONFIG_NORELAOD_L: A 1 in this bit prevents the lower match and fractional match registers from being  reloaded from their respective reload registers. Software can  write to set or clear this bit at any time. This bit applies to both the  higher and lower registers when the UNIFY bit is set. */
#define SCT_CONFIG_NORELAOD_L_SHIFT (7)
#define SCT_CONFIG_NORELAOD_L_MASK (0x01 << SCT_CONFIG_NORELAOD_L_SHIFT)
#define SCT_CONFIG_NORELAOD_L(x) ((x) << SCT_CONFIG_NORELAOD_L_SHIFT)

/* -- SCT_CONFIG_NORELOAD_H: A 1 in this bit prevents the higher match and fractional match registers from being  reloaded from their respective reload registers. Software can  write to set or clear this bit at any time. This bit is not used when  the UNIFY bit is set. */
#define SCT_CONFIG_NORELOAD_H_SHIFT (8)
#define SCT_CONFIG_NORELOAD_H_MASK (0x01 << SCT_CONFIG_NORELOAD_H_SHIFT)
#define SCT_CONFIG_NORELOAD_H(x) ((x) << SCT_CONFIG_NORELOAD_H_SHIFT)

/* -- SCT_CONFIG_INSYNC: Synchronization for input n (bit 9 = input 0, bit 10 = input 1,..., bit 16 = input 7). A 1 in one of these bits subjects the corresponding input to  synchronization to the SCT clock, before it is used to create an  event. If an input is synchronous to the SCT clock, keep its bit 0 for  faster response. When the CKMODE field is 1x, the bit in this field, corresponding  to the input selected by the CKSEL field, is not used. */
#define SCT_CONFIG_INSYNC_SHIFT (9)
#define SCT_CONFIG_INSYNC_MASK (0xFF << SCT_CONFIG_INSYNC_SHIFT)
#define SCT_CONFIG_INSYNC(x) ((x) << SCT_CONFIG_INSYNC_SHIFT)

/* -- SCT_CONFIG_AUTOLIMIT_L: A one in this bit causes a match on match register 0 to be treated  as a de-facto LIMIT condition without the need to define an  associated event. As with any LIMIT event, this automatic limit causes the  counter to be cleared to zero in uni-directional mode or to change  the direction of count in bi-directional mode. Software can write to set or clear this bit at any time. This bit  applies to both the higher and lower registers when the UNIFY bit  is set. */
#define SCT_CONFIG_AUTOLIMIT_L_SHIFT (17)
#define SCT_CONFIG_AUTOLIMIT_L_MASK (0x01 << SCT_CONFIG_AUTOLIMIT_L_SHIFT)
#define SCT_CONFIG_AUTOLIMIT_L(x) ((x) << SCT_CONFIG_AUTOLIMIT_L_SHIFT)

/* -- SCT_CONFIG_AUTOLIMIT_H: A one in this bit will cause a match on match register 0 to be treated  as a de-facto LIMIT condition without the need to define an  associated event. As with any LIMIT event, this automatic limit causes the  counter to be cleared to zero in uni-directional mode or to change  the direction of count in bi-directional mode. Software can write to set or clear this bit at any time. This bit is  not used when the UNIFY bit is set. */
#define SCT_CONFIG_AUTOLIMIT_H_SHIFT (18)
#define SCT_CONFIG_AUTOLIMIT_H_MASK (0x01 << SCT_CONFIG_AUTOLIMIT_H_SHIFT)
#define SCT_CONFIG_AUTOLIMIT_H(x) ((x) << SCT_CONFIG_AUTOLIMIT_H_SHIFT)

/* --- SCT_CTRL ------------------------------------------ */
#define SCT_CTRL MMIO32(SCT_BASE + 0x04)

/* -- SCT_CTRL_DOWN_L: This bit is 1 when the L or unified counter is counting down. Hardware sets this bit   when the counter limit is reached and BIDIR is 1. Hardware clears this bit when the counter reaches 0 or when the counter is counting down and a limit condition occurs. */
#define SCT_CTRL_DOWN_L_SHIFT (0)
#define SCT_CTRL_DOWN_L_MASK (0x01 << SCT_CTRL_DOWN_L_SHIFT)
#define SCT_CTRL_DOWN_L(x) ((x) << SCT_CTRL_DOWN_L_SHIFT)

/* -- SCT_CTRL_STOP_L: When this bit is 1 and HALT is 0, the L or unified counter does not run but I/O  events related to the counter can occur. If such an event matches  the mask in the Start register, this bit is cleared and counting  resumes. */
#define SCT_CTRL_STOP_L_SHIFT (1)
#define SCT_CTRL_STOP_L_MASK (0x01 << SCT_CTRL_STOP_L_SHIFT)
#define SCT_CTRL_STOP_L(x) ((x) << SCT_CTRL_STOP_L_SHIFT)

/* -- SCT_CTRL_HALT_L: When this bit is 1, the L or unified counter does not run and no events can occur.  A reset sets this bit. When the HALT_L bit is one, the STOP_L bit is cleared. If you want to remove the halt condition and keep the SCT in the stop condition (not running), then you can change the halt and stop condition with one single write to this register. Once set, only software can clear this bit to restore counter operation. */
#define SCT_CTRL_HALT_L_SHIFT (2)
#define SCT_CTRL_HALT_L_MASK (0x01 << SCT_CTRL_HALT_L_SHIFT)
#define SCT_CTRL_HALT_L(x) ((x) << SCT_CTRL_HALT_L_SHIFT)

/* -- SCT_CTRL_CLRCTR_L: Writing a 1 to this bit clears the L or unified counter. This bit always reads as 0. */
#define SCT_CTRL_CLRCTR_L_SHIFT (3)
#define SCT_CTRL_CLRCTR_L_MASK (0x01 << SCT_CTRL_CLRCTR_L_SHIFT)
#define SCT_CTRL_CLRCTR_L(x) ((x) << SCT_CTRL_CLRCTR_L_SHIFT)

/* -- SCT_CTRL_BIDIR_L: L or unified counter direction select */
#define SCT_CTRL_BIDIR_L_SHIFT (4)
#define SCT_CTRL_BIDIR_L_MASK (0x01 << SCT_CTRL_BIDIR_L_SHIFT)
#define SCT_CTRL_BIDIR_L(x) ((x) << SCT_CTRL_BIDIR_L_SHIFT)

/* SCT_CTRL_BIDIR_L_BIDIR_L values */
#define SCT_CTRL_BIDIR_L_UP SCT_CTRL_BIDIR_L(0x00)  /* Up. The counter counts up to its limit condition, then is cleared to zero. */
#define SCT_CTRL_BIDIR_L_UPDOWN SCT_CTRL_BIDIR_L(0x01)  /* Up-down. The counter counts up to its limit, then counts down to a limit condition or to 0. */

/* -- SCT_CTRL_PRE_L: Specifies the factor by which the SCT clock is prescaled to produce the  L or unified counter clock. The counter clock is clocked at the rate of the SCT  clock divided by PRE_L+1. Clear the counter (by writing a 1  to the CLRCTR bit) whenever changing the PRE value. */
#define SCT_CTRL_PRE_L_SHIFT (5)
#define SCT_CTRL_PRE_L_MASK (0xFF << SCT_CTRL_PRE_L_SHIFT)
#define SCT_CTRL_PRE_L(x) ((x) << SCT_CTRL_PRE_L_SHIFT)

/* -- SCT_CTRL_DOWN_H: This bit is 1 when the H counter is counting down. Hardware sets this bit   when the counter limit is reached and BIDIR is 1. Hardware clears this bit when the counter reaches 0 or when the counter is counting down and a limit condition occurs. */
#define SCT_CTRL_DOWN_H_SHIFT (16)
#define SCT_CTRL_DOWN_H_MASK (0x01 << SCT_CTRL_DOWN_H_SHIFT)
#define SCT_CTRL_DOWN_H(x) ((x) << SCT_CTRL_DOWN_H_SHIFT)

/* -- SCT_CTRL_STOP_H: When this bit is 1 and HALT is 0, the H counter does not run but I/O  events related to the counter can occur. If such an event matches  the mask in the Start register, this bit is cleared and counting  resumes. */
#define SCT_CTRL_STOP_H_SHIFT (17)
#define SCT_CTRL_STOP_H_MASK (0x01 << SCT_CTRL_STOP_H_SHIFT)
#define SCT_CTRL_STOP_H(x) ((x) << SCT_CTRL_STOP_H_SHIFT)

/* -- SCT_CTRL_HALT_H: When this bit is 1, the H counter does not run and no events can occur.  A reset sets this bit. When the HALT_H bit is one, the STOP_H bit is cleared. If you want to remove the halt condition and keep the SCT in the stop condition (not running), then you can change the halt and stop condition with one single write to this register. Once set, this bit can only be cleared by software to restore counter operation. */
#define SCT_CTRL_HALT_H_SHIFT (18)
#define SCT_CTRL_HALT_H_MASK (0x01 << SCT_CTRL_HALT_H_SHIFT)
#define SCT_CTRL_HALT_H(x) ((x) << SCT_CTRL_HALT_H_SHIFT)

/* -- SCT_CTRL_CLRCTR_H: Writing a 1 to this bit clears the H counter. This bit always reads as 0. */
#define SCT_CTRL_CLRCTR_H_SHIFT (19)
#define SCT_CTRL_CLRCTR_H_MASK (0x01 << SCT_CTRL_CLRCTR_H_SHIFT)
#define SCT_CTRL_CLRCTR_H(x) ((x) << SCT_CTRL_CLRCTR_H_SHIFT)

/* -- SCT_CTRL_BIDIR_H: Direction select */
#define SCT_CTRL_BIDIR_H_SHIFT (20)
#define SCT_CTRL_BIDIR_H_MASK (0x01 << SCT_CTRL_BIDIR_H_SHIFT)
#define SCT_CTRL_BIDIR_H(x) ((x) << SCT_CTRL_BIDIR_H_SHIFT)

/* SCT_CTRL_BIDIR_H_BIDIR_H values */
#define SCT_CTRL_BIDIR_H_UP SCT_CTRL_BIDIR_H(0x00)  /* Up. The H counter counts up to its limit condition, then is cleared to zero. */
#define SCT_CTRL_BIDIR_H_UPDOWN SCT_CTRL_BIDIR_H(0x01)  /* Up-down. The H counter counts up to its limit, then counts down to a limit condition or to 0. */

/* -- SCT_CTRL_PRE_H: Specifies the factor by which the SCT clock is prescaled to produce the  H counter clock. The counter clock is clocked at the rate of the SCT  clock divided by PRELH+1. Clear the counter (by writing a 1  to the CLRCTR bit) whenever changing the PRE value. */
#define SCT_CTRL_PRE_H_SHIFT (21)
#define SCT_CTRL_PRE_H_MASK (0xFF << SCT_CTRL_PRE_H_SHIFT)
#define SCT_CTRL_PRE_H(x) ((x) << SCT_CTRL_PRE_H_SHIFT)

/* --- SCT_LIMIT ----------------------------------------- */
#define SCT_LIMIT MMIO32(SCT_BASE + 0x08)

/* -- SCT_LIMIT_LIMMSK_L: If bit n is one, event n is used as a counter limit event for the L or unified counter (event 0 = bit 0, event 1 = bit 1, event 15 = bit 15). */
#define SCT_LIMIT_LIMMSK_L_SHIFT (0)
#define SCT_LIMIT_LIMMSK_L_MASK (0xFFFF << SCT_LIMIT_LIMMSK_L_SHIFT)
#define SCT_LIMIT_LIMMSK_L(x) ((x) << SCT_LIMIT_LIMMSK_L_SHIFT)

/* -- SCT_LIMIT_LIMMSK_H: If bit n is one, event n is used as a counter limit event for the H counter (event 0 = bit 16, event 1 = bit 17, event 15 = bit 31). */
#define SCT_LIMIT_LIMMSK_H_SHIFT (16)
#define SCT_LIMIT_LIMMSK_H_MASK (0xFFFF << SCT_LIMIT_LIMMSK_H_SHIFT)
#define SCT_LIMIT_LIMMSK_H(x) ((x) << SCT_LIMIT_LIMMSK_H_SHIFT)

/* --- SCT_HALT ------------------------------------------ */
#define SCT_HALT MMIO32(SCT_BASE + 0x0C)

/* -- SCT_HALT_HALTMSK_L: If bit n is one, event n sets the HALT_L bit in the CTRL register (event 0 = bit 0, event 1 = bit 1, event 15 = bit 15). */
#define SCT_HALT_HALTMSK_L_SHIFT (0)
#define SCT_HALT_HALTMSK_L_MASK (0xFFFF << SCT_HALT_HALTMSK_L_SHIFT)
#define SCT_HALT_HALTMSK_L(x) ((x) << SCT_HALT_HALTMSK_L_SHIFT)

/* -- SCT_HALT_HALTMSK_H: If bit n is one, event n sets the HALT_H bit in the CTRL register (event 0 = bit 16, event 1 = bit 17, event 15 = bit 31). */
#define SCT_HALT_HALTMSK_H_SHIFT (16)
#define SCT_HALT_HALTMSK_H_MASK (0xFFFF << SCT_HALT_HALTMSK_H_SHIFT)
#define SCT_HALT_HALTMSK_H(x) ((x) << SCT_HALT_HALTMSK_H_SHIFT)

/* --- SCT_STOP ------------------------------------------ */
#define SCT_STOP MMIO32(SCT_BASE + 0x10)

/* -- SCT_STOP_STOPMSK_L: If bit n is one, event n sets the STOP_L bit in the CTRL register (event 0 = bit 0, event 1 = bit 1, event 15 = bit 15). */
#define SCT_STOP_STOPMSK_L_SHIFT (0)
#define SCT_STOP_STOPMSK_L_MASK (0xFFFF << SCT_STOP_STOPMSK_L_SHIFT)
#define SCT_STOP_STOPMSK_L(x) ((x) << SCT_STOP_STOPMSK_L_SHIFT)

/* -- SCT_STOP_STOPMSK_H: If bit n is one, event n sets the STOP_H bit in the CTRL register (event 0 = bit 16, event 1 = bit 17, event 15 = bit 31). */
#define SCT_STOP_STOPMSK_H_SHIFT (16)
#define SCT_STOP_STOPMSK_H_MASK (0xFFFF << SCT_STOP_STOPMSK_H_SHIFT)
#define SCT_STOP_STOPMSK_H(x) ((x) << SCT_STOP_STOPMSK_H_SHIFT)

/* --- SCT_START ----------------------------------------- */
#define SCT_START MMIO32(SCT_BASE + 0x14)

/* -- SCT_START_STARTMSK_L: If bit n is one, event n clears the STOP_L bit in the CTRL register (event 0 = bit 0, event 1 = bit 1, event 15 = bit 15). */
#define SCT_START_STARTMSK_L_SHIFT (0)
#define SCT_START_STARTMSK_L_MASK (0xFFFF << SCT_START_STARTMSK_L_SHIFT)
#define SCT_START_STARTMSK_L(x) ((x) << SCT_START_STARTMSK_L_SHIFT)

/* -- SCT_START_STARTMSK_H: If bit n is one, event n clears the STOP_H bit in the CTRL register (event 0 = bit 16, event 1 = bit 17, event 15 = bit 31). */
#define SCT_START_STARTMSK_H_SHIFT (16)
#define SCT_START_STARTMSK_H_MASK (0xFFFF << SCT_START_STARTMSK_H_SHIFT)
#define SCT_START_STARTMSK_H(x) ((x) << SCT_START_STARTMSK_H_SHIFT)

/* --- SCT_DITHER ---------------------------------------- */
#define SCT_DITHER MMIO32(SCT_BASE + 0x18)

/* -- SCT_DITHER_DITHMSK_L: If bit n is one, the event n causes the dither engine to advance to the next element in the dither pattern at the start of the next counter cycle of the 16-bit low counter or the unified counter (event 0 = bit 0, event 1 = bit 1, event 15 = bit 15). If all bits are set to 0, the dither pattern automatically advances at the start of every new counter cycle. */
#define SCT_DITHER_DITHMSK_L_SHIFT (0)
#define SCT_DITHER_DITHMSK_L_MASK (0xFFFF << SCT_DITHER_DITHMSK_L_SHIFT)
#define SCT_DITHER_DITHMSK_L(x) ((x) << SCT_DITHER_DITHMSK_L_SHIFT)

/* -- SCT_DITHER_DITHMSK_H: If bit n is one, the event n causes the dither engine to advance to the next element in the dither pattern at the start of the next counter cycle of the 16-bit high counter (event 0 = bit 0, event 1 = bit 1, event 15 = bit 15). If all bits are set to 0, the dither pattern automatically advances at the start of every new counter cycle. */
#define SCT_DITHER_DITHMSK_H_SHIFT (16)
#define SCT_DITHER_DITHMSK_H_MASK (0xFFFF << SCT_DITHER_DITHMSK_H_SHIFT)
#define SCT_DITHER_DITHMSK_H(x) ((x) << SCT_DITHER_DITHMSK_H_SHIFT)

/* --- SCT_COUNT ----------------------------------------- */
#define SCT_COUNT MMIO32(SCT_BASE + 0x40)

/* -- SCT_COUNT_CTR_L: When UNIFY = 0, read or write the 16-bit L counter value. When UNIFY = 1, read or write the lower 16 bits of the 32-bit unified counter. */
#define SCT_COUNT_CTR_L_SHIFT (0)
#define SCT_COUNT_CTR_L_MASK (0xFFFF << SCT_COUNT_CTR_L_SHIFT)
#define SCT_COUNT_CTR_L(x) ((x) << SCT_COUNT_CTR_L_SHIFT)

/* -- SCT_COUNT_CTR_H: When UNIFY = 0, read or write the 16-bit H counter value. When UNIFY = 1, read or write the upper 16 bits of the 32-bit unified counter. */
#define SCT_COUNT_CTR_H_SHIFT (16)
#define SCT_COUNT_CTR_H_MASK (0xFFFF << SCT_COUNT_CTR_H_SHIFT)
#define SCT_COUNT_CTR_H(x) ((x) << SCT_COUNT_CTR_H_SHIFT)

/* --- SCT_STATE ----------------------------------------- */
#define SCT_STATE MMIO32(SCT_BASE + 0x44)

/* -- SCT_STATE_STATE_L: State variable. */
#define SCT_STATE_STATE_L_SHIFT (0)
#define SCT_STATE_STATE_L_MASK (0x1F << SCT_STATE_STATE_L_SHIFT)
#define SCT_STATE_STATE_L(x) ((x) << SCT_STATE_STATE_L_SHIFT)

/* -- SCT_STATE_STATE_H: State variable. */
#define SCT_STATE_STATE_H_SHIFT (16)
#define SCT_STATE_STATE_H_MASK (0x1F << SCT_STATE_STATE_H_SHIFT)
#define SCT_STATE_STATE_H(x) ((x) << SCT_STATE_STATE_H_SHIFT)

/* --- SCT_INPUT ----------------------------------------- */
#define SCT_INPUT MMIO32(SCT_BASE + 0x48)

/* -- SCT_INPUT_AIN0: Real-time status of input 0. */
#define SCT_INPUT_AIN0_SHIFT (0)
#define SCT_INPUT_AIN0_MASK (0x01 << SCT_INPUT_AIN0_SHIFT)
#define SCT_INPUT_AIN0(x) ((x) << SCT_INPUT_AIN0_SHIFT)

/* -- SCT_INPUT_AIN1: Real-time status of input 1. */
#define SCT_INPUT_AIN1_SHIFT (1)
#define SCT_INPUT_AIN1_MASK (0x01 << SCT_INPUT_AIN1_SHIFT)
#define SCT_INPUT_AIN1(x) ((x) << SCT_INPUT_AIN1_SHIFT)

/* -- SCT_INPUT_AIN2: Real-time status of input 2. */
#define SCT_INPUT_AIN2_SHIFT (2)
#define SCT_INPUT_AIN2_MASK (0x01 << SCT_INPUT_AIN2_SHIFT)
#define SCT_INPUT_AIN2(x) ((x) << SCT_INPUT_AIN2_SHIFT)

/* -- SCT_INPUT_AIN3: Real-time status of input 3. */
#define SCT_INPUT_AIN3_SHIFT (3)
#define SCT_INPUT_AIN3_MASK (0x01 << SCT_INPUT_AIN3_SHIFT)
#define SCT_INPUT_AIN3(x) ((x) << SCT_INPUT_AIN3_SHIFT)

/* -- SCT_INPUT_AIN4: Real-time status of input 4. */
#define SCT_INPUT_AIN4_SHIFT (4)
#define SCT_INPUT_AIN4_MASK (0x01 << SCT_INPUT_AIN4_SHIFT)
#define SCT_INPUT_AIN4(x) ((x) << SCT_INPUT_AIN4_SHIFT)

/* -- SCT_INPUT_AIN5: Real-time status of input 5. */
#define SCT_INPUT_AIN5_SHIFT (5)
#define SCT_INPUT_AIN5_MASK (0x01 << SCT_INPUT_AIN5_SHIFT)
#define SCT_INPUT_AIN5(x) ((x) << SCT_INPUT_AIN5_SHIFT)

/* -- SCT_INPUT_AIN6: Real-time status of input 6. */
#define SCT_INPUT_AIN6_SHIFT (6)
#define SCT_INPUT_AIN6_MASK (0x01 << SCT_INPUT_AIN6_SHIFT)
#define SCT_INPUT_AIN6(x) ((x) << SCT_INPUT_AIN6_SHIFT)

/* -- SCT_INPUT_AIN7: Real-time status of input 7. */
#define SCT_INPUT_AIN7_SHIFT (7)
#define SCT_INPUT_AIN7_MASK (0x01 << SCT_INPUT_AIN7_SHIFT)
#define SCT_INPUT_AIN7(x) ((x) << SCT_INPUT_AIN7_SHIFT)

/* -- SCT_INPUT_SIN0: Input 0 state synchronized to the SCT clock. */
#define SCT_INPUT_SIN0_SHIFT (16)
#define SCT_INPUT_SIN0_MASK (0x01 << SCT_INPUT_SIN0_SHIFT)
#define SCT_INPUT_SIN0(x) ((x) << SCT_INPUT_SIN0_SHIFT)

/* -- SCT_INPUT_SIN1: Input 1 state synchronized to the SCT clock. */
#define SCT_INPUT_SIN1_SHIFT (17)
#define SCT_INPUT_SIN1_MASK (0x01 << SCT_INPUT_SIN1_SHIFT)
#define SCT_INPUT_SIN1(x) ((x) << SCT_INPUT_SIN1_SHIFT)

/* -- SCT_INPUT_SIN2: Input 2 state synchronized to the SCT clock. */
#define SCT_INPUT_SIN2_SHIFT (18)
#define SCT_INPUT_SIN2_MASK (0x01 << SCT_INPUT_SIN2_SHIFT)
#define SCT_INPUT_SIN2(x) ((x) << SCT_INPUT_SIN2_SHIFT)

/* -- SCT_INPUT_SIN3: Input 3 state synchronized to the SCT clock. */
#define SCT_INPUT_SIN3_SHIFT (19)
#define SCT_INPUT_SIN3_MASK (0x01 << SCT_INPUT_SIN3_SHIFT)
#define SCT_INPUT_SIN3(x) ((x) << SCT_INPUT_SIN3_SHIFT)

/* -- SCT_INPUT_SIN4: Input 4 state synchronized to the SCT clock. */
#define SCT_INPUT_SIN4_SHIFT (20)
#define SCT_INPUT_SIN4_MASK (0x01 << SCT_INPUT_SIN4_SHIFT)
#define SCT_INPUT_SIN4(x) ((x) << SCT_INPUT_SIN4_SHIFT)

/* -- SCT_INPUT_SIN5: Input 5 state synchronized to the SCT clock. */
#define SCT_INPUT_SIN5_SHIFT (21)
#define SCT_INPUT_SIN5_MASK (0x01 << SCT_INPUT_SIN5_SHIFT)
#define SCT_INPUT_SIN5(x) ((x) << SCT_INPUT_SIN5_SHIFT)

/* -- SCT_INPUT_SIN6: Input 6 state synchronized to the SCT clock. */
#define SCT_INPUT_SIN6_SHIFT (22)
#define SCT_INPUT_SIN6_MASK (0x01 << SCT_INPUT_SIN6_SHIFT)
#define SCT_INPUT_SIN6(x) ((x) << SCT_INPUT_SIN6_SHIFT)

/* -- SCT_INPUT_SIN7: Input 7 state synchronized to the SCT clock. */
#define SCT_INPUT_SIN7_SHIFT (23)
#define SCT_INPUT_SIN7_MASK (0x01 << SCT_INPUT_SIN7_SHIFT)
#define SCT_INPUT_SIN7(x) ((x) << SCT_INPUT_SIN7_SHIFT)

/* --- SCT_REGMODE --------------------------------------- */
#define SCT_REGMODE MMIO32(SCT_BASE + 0x4C)

/* -- SCT_REGMODE_REGMOD_L: Each bit controls one pair of match/capture registers (register 0 = bit 0, register 1 = bit 1,..., register 15 = bit 15).  0 = registers operate as match registers. 1 = registers operate as capture registers. */
#define SCT_REGMODE_REGMOD_L_SHIFT (0)
#define SCT_REGMODE_REGMOD_L_MASK (0xFFFF << SCT_REGMODE_REGMOD_L_SHIFT)
#define SCT_REGMODE_REGMOD_L(x) ((x) << SCT_REGMODE_REGMOD_L_SHIFT)

/* -- SCT_REGMODE_REGMOD_H: Each bit controls one pair of match/capture registers (register 0 = bit 16, register 1 = bit 17,..., register 15 = bit 31). 0 = registers operate as match registers. 1 = registers operate as capture registers. */
#define SCT_REGMODE_REGMOD_H_SHIFT (16)
#define SCT_REGMODE_REGMOD_H_MASK (0xFFFF << SCT_REGMODE_REGMOD_H_SHIFT)
#define SCT_REGMODE_REGMOD_H(x) ((x) << SCT_REGMODE_REGMOD_H_SHIFT)

/* --- SCT_OUTPUT ---------------------------------------- */
#define SCT_OUTPUT MMIO32(SCT_BASE + 0x50)

/* -- SCT_OUTPUT_OUT: Writing a 1 to bit n makes the corresponding output HIGH. 0 makes the corresponding output LOW (output 0 = bit 0, output 1 = bit 1,..., output 15 = bit 15). */
#define SCT_OUTPUT_OUT_SHIFT (0)
#define SCT_OUTPUT_OUT_MASK (0xFFFF << SCT_OUTPUT_OUT_SHIFT)
#define SCT_OUTPUT_OUT(x) ((x) << SCT_OUTPUT_OUT_SHIFT)

/* --- SCT_OUTPUTDIRCTRL --------------------------------- */
#define SCT_OUTPUTDIRCTRL MMIO32(SCT_BASE + 0x54)

/* -- SCT_OUTPUTDIRCTRL_SETCLR0: Set/clear operation on output 0. Value 0x3 is reserved. Do not program this value. */
#define SCT_OUTPUTDIRCTRL_SETCLR0_SHIFT (0)
#define SCT_OUTPUTDIRCTRL_SETCLR0_MASK (0x03 << SCT_OUTPUTDIRCTRL_SETCLR0_SHIFT)
#define SCT_OUTPUTDIRCTRL_SETCLR0(x) ((x) << SCT_OUTPUTDIRCTRL_SETCLR0_SHIFT)

/* SCT_OUTPUTDIRCTRL_SETCLR0_SETCLR0 values */
#define SCT_OUTPUTDIRCTRL_SETCLR0_INDEPENDENT SCT_OUTPUTDIRCTRL_SETCLR0(0x00)  /* Independent. Set and clear do not depend on any counter. */
#define SCT_OUTPUTDIRCTRL_SETCLR0_L_COUNTER SCT_OUTPUTDIRCTRL_SETCLR0(0x01)  /* L counter. Set and clear are reversed when counter L or the unified counter is counting down. */
#define SCT_OUTPUTDIRCTRL_SETCLR0_H_COUNTER SCT_OUTPUTDIRCTRL_SETCLR0(0x02)  /* H counter. Set and clear are reversed when counter H is counting down. Do not use if UNIFY = 1. */

/* -- SCT_OUTPUTDIRCTRL_SETCLR1: Set/clear operation on output 1. Value 0x3 is reserved. Do not program this value. */
#define SCT_OUTPUTDIRCTRL_SETCLR1_SHIFT (2)
#define SCT_OUTPUTDIRCTRL_SETCLR1_MASK (0x03 << SCT_OUTPUTDIRCTRL_SETCLR1_SHIFT)
#define SCT_OUTPUTDIRCTRL_SETCLR1(x) ((x) << SCT_OUTPUTDIRCTRL_SETCLR1_SHIFT)

/* SCT_OUTPUTDIRCTRL_SETCLR1_SETCLR1 values */
#define SCT_OUTPUTDIRCTRL_SETCLR1_INDEPENDENT SCT_OUTPUTDIRCTRL_SETCLR1(0x00)  /* Independent. Set and clear do not depend on any counter. */
#define SCT_OUTPUTDIRCTRL_SETCLR1_L_COUNTER SCT_OUTPUTDIRCTRL_SETCLR1(0x01)  /* L counter. Set and clear are reversed when counter L or the unified counter is counting down. */
#define SCT_OUTPUTDIRCTRL_SETCLR1_H_COUNTER SCT_OUTPUTDIRCTRL_SETCLR1(0x02)  /* H counter. Set and clear are reversed when counter H is counting down. Do not use if UNIFY = 1. */

/* -- SCT_OUTPUTDIRCTRL_SETCLR2: Set/clear operation on output 2. Value 0x3 is reserved. Do not program this value. */
#define SCT_OUTPUTDIRCTRL_SETCLR2_SHIFT (4)
#define SCT_OUTPUTDIRCTRL_SETCLR2_MASK (0x03 << SCT_OUTPUTDIRCTRL_SETCLR2_SHIFT)
#define SCT_OUTPUTDIRCTRL_SETCLR2(x) ((x) << SCT_OUTPUTDIRCTRL_SETCLR2_SHIFT)

/* SCT_OUTPUTDIRCTRL_SETCLR2_SETCLR2 values */
#define SCT_OUTPUTDIRCTRL_SETCLR2_INDEPENDENT SCT_OUTPUTDIRCTRL_SETCLR2(0x00)  /* Independent. Set and clear do not depend on any counter. */
#define SCT_OUTPUTDIRCTRL_SETCLR2_L_COUNTER SCT_OUTPUTDIRCTRL_SETCLR2(0x01)  /* L counter. Set and clear are reversed when counter L or the unified counter is counting down. */
#define SCT_OUTPUTDIRCTRL_SETCLR2_H_COUNTER SCT_OUTPUTDIRCTRL_SETCLR2(0x02)  /* H counter. Set and clear are reversed when counter H is counting down. Do not use if UNIFY = 1. */

/* -- SCT_OUTPUTDIRCTRL_SETCLR3: Set/clear operation on output 3. Value 0x3 is reserved. Do not program this value. */
#define SCT_OUTPUTDIRCTRL_SETCLR3_SHIFT (6)
#define SCT_OUTPUTDIRCTRL_SETCLR3_MASK (0x03 << SCT_OUTPUTDIRCTRL_SETCLR3_SHIFT)
#define SCT_OUTPUTDIRCTRL_SETCLR3(x) ((x) << SCT_OUTPUTDIRCTRL_SETCLR3_SHIFT)

/* SCT_OUTPUTDIRCTRL_SETCLR3_SETCLR3 values */
#define SCT_OUTPUTDIRCTRL_SETCLR3_INDEPENDENT SCT_OUTPUTDIRCTRL_SETCLR3(0x00)  /* Independent. Set and clear do not depend on any counter. */
#define SCT_OUTPUTDIRCTRL_SETCLR3_L_COUNTER SCT_OUTPUTDIRCTRL_SETCLR3(0x01)  /* L counter. Set and clear are reversed when counter L or the unified counter is counting down. */
#define SCT_OUTPUTDIRCTRL_SETCLR3_H_COUNTER SCT_OUTPUTDIRCTRL_SETCLR3(0x02)  /* H counter. Set and clear are reversed when counter H is counting down. Do not use if UNIFY = 1. */

/* -- SCT_OUTPUTDIRCTRL_SETCLR4: Set/clear operation on output 4. Value 0x3 is reserved. Do not program this value. */
#define SCT_OUTPUTDIRCTRL_SETCLR4_SHIFT (8)
#define SCT_OUTPUTDIRCTRL_SETCLR4_MASK (0x03 << SCT_OUTPUTDIRCTRL_SETCLR4_SHIFT)
#define SCT_OUTPUTDIRCTRL_SETCLR4(x) ((x) << SCT_OUTPUTDIRCTRL_SETCLR4_SHIFT)

/* SCT_OUTPUTDIRCTRL_SETCLR4_SETCLR4 values */
#define SCT_OUTPUTDIRCTRL_SETCLR4_INDEPENDENT SCT_OUTPUTDIRCTRL_SETCLR4(0x00)  /* Independent. Set and clear do not depend on any counter. */
#define SCT_OUTPUTDIRCTRL_SETCLR4_L_COUNTER SCT_OUTPUTDIRCTRL_SETCLR4(0x01)  /* L counter. Set and clear are reversed when counter L or the unified counter is counting down. */
#define SCT_OUTPUTDIRCTRL_SETCLR4_H_COUNTER SCT_OUTPUTDIRCTRL_SETCLR4(0x02)  /* H counter. Set and clear are reversed when counter H is counting down. Do not use if UNIFY = 1. */

/* -- SCT_OUTPUTDIRCTRL_SETCLR5: Set/clear operation on output 5. Value 0x3 is reserved. Do not program this value. */
#define SCT_OUTPUTDIRCTRL_SETCLR5_SHIFT (10)
#define SCT_OUTPUTDIRCTRL_SETCLR5_MASK (0x03 << SCT_OUTPUTDIRCTRL_SETCLR5_SHIFT)
#define SCT_OUTPUTDIRCTRL_SETCLR5(x) ((x) << SCT_OUTPUTDIRCTRL_SETCLR5_SHIFT)

/* SCT_OUTPUTDIRCTRL_SETCLR5_SETCLR5 values */
#define SCT_OUTPUTDIRCTRL_SETCLR5_INDEPENDENT SCT_OUTPUTDIRCTRL_SETCLR5(0x00)  /* Independent. Set and clear do not depend on any counter. */
#define SCT_OUTPUTDIRCTRL_SETCLR5_L_COUNTER SCT_OUTPUTDIRCTRL_SETCLR5(0x01)  /* L counter. Set and clear are reversed when counter L or the unified counter is counting down. */
#define SCT_OUTPUTDIRCTRL_SETCLR5_H_COUNTER SCT_OUTPUTDIRCTRL_SETCLR5(0x02)  /* H counter. Set and clear are reversed when counter H is counting down. Do not use if UNIFY = 1. */

/* -- SCT_OUTPUTDIRCTRL_SETCLR6: Set/clear operation on output 6. Value 0x3 is reserved. Do not program this value. */
#define SCT_OUTPUTDIRCTRL_SETCLR6_SHIFT (12)
#define SCT_OUTPUTDIRCTRL_SETCLR6_MASK (0x03 << SCT_OUTPUTDIRCTRL_SETCLR6_SHIFT)
#define SCT_OUTPUTDIRCTRL_SETCLR6(x) ((x) << SCT_OUTPUTDIRCTRL_SETCLR6_SHIFT)

/* SCT_OUTPUTDIRCTRL_SETCLR6_SETCLR6 values */
#define SCT_OUTPUTDIRCTRL_SETCLR6_INDEPENDENT SCT_OUTPUTDIRCTRL_SETCLR6(0x00)  /* Independent. Set and clear do not depend on any counter. */
#define SCT_OUTPUTDIRCTRL_SETCLR6_L_COUNTER SCT_OUTPUTDIRCTRL_SETCLR6(0x01)  /* L counter. Set and clear are reversed when counter L or the unified counter is counting down. */
#define SCT_OUTPUTDIRCTRL_SETCLR6_H_COUNTER SCT_OUTPUTDIRCTRL_SETCLR6(0x02)  /* H counter. Set and clear are reversed when counter H is counting down. Do not use if UNIFY = 1. */

/* -- SCT_OUTPUTDIRCTRL_SETCLR7: Set/clear operation on output 7. Value 0x3 is reserved. Do not program this value. */
#define SCT_OUTPUTDIRCTRL_SETCLR7_SHIFT (14)
#define SCT_OUTPUTDIRCTRL_SETCLR7_MASK (0x03 << SCT_OUTPUTDIRCTRL_SETCLR7_SHIFT)
#define SCT_OUTPUTDIRCTRL_SETCLR7(x) ((x) << SCT_OUTPUTDIRCTRL_SETCLR7_SHIFT)

/* SCT_OUTPUTDIRCTRL_SETCLR7_SETCLR7 values */
#define SCT_OUTPUTDIRCTRL_SETCLR7_INDEPENDENT SCT_OUTPUTDIRCTRL_SETCLR7(0x00)  /* Independent. Set and clear do not depend on any counter. */
#define SCT_OUTPUTDIRCTRL_SETCLR7_L_COUNTER SCT_OUTPUTDIRCTRL_SETCLR7(0x01)  /* L counter. Set and clear are reversed when counter L or the unified counter is counting down. */
#define SCT_OUTPUTDIRCTRL_SETCLR7_H_COUNTER SCT_OUTPUTDIRCTRL_SETCLR7(0x02)  /* H counter. Set and clear are reversed when counter H is counting down. Do not use if UNIFY = 1. */

/* -- SCT_OUTPUTDIRCTRL_SETCLR8: Set/clear operation on output 8. Value 0x3 is reserved. Do not program this value. */
#define SCT_OUTPUTDIRCTRL_SETCLR8_SHIFT (16)
#define SCT_OUTPUTDIRCTRL_SETCLR8_MASK (0x03 << SCT_OUTPUTDIRCTRL_SETCLR8_SHIFT)
#define SCT_OUTPUTDIRCTRL_SETCLR8(x) ((x) << SCT_OUTPUTDIRCTRL_SETCLR8_SHIFT)

/* SCT_OUTPUTDIRCTRL_SETCLR8_SETCLR8 values */
#define SCT_OUTPUTDIRCTRL_SETCLR8_INDEPENDENT SCT_OUTPUTDIRCTRL_SETCLR8(0x00)  /* Independent. Set and clear do not depend on any counter. */
#define SCT_OUTPUTDIRCTRL_SETCLR8_L_COUNTER SCT_OUTPUTDIRCTRL_SETCLR8(0x01)  /* L counter. Set and clear are reversed when counter L or the unified counter is counting down. */
#define SCT_OUTPUTDIRCTRL_SETCLR8_H_COUNTER SCT_OUTPUTDIRCTRL_SETCLR8(0x02)  /* H counter. Set and clear are reversed when counter H is counting down. Do not use if UNIFY = 1. */

/* -- SCT_OUTPUTDIRCTRL_SETCLR9: Set/clear operation on output 9. Value 0x3 is reserved. Do not program this value. */
#define SCT_OUTPUTDIRCTRL_SETCLR9_SHIFT (18)
#define SCT_OUTPUTDIRCTRL_SETCLR9_MASK (0x03 << SCT_OUTPUTDIRCTRL_SETCLR9_SHIFT)
#define SCT_OUTPUTDIRCTRL_SETCLR9(x) ((x) << SCT_OUTPUTDIRCTRL_SETCLR9_SHIFT)

/* SCT_OUTPUTDIRCTRL_SETCLR9_SETCLR9 values */
#define SCT_OUTPUTDIRCTRL_SETCLR9_INDEPENDENT SCT_OUTPUTDIRCTRL_SETCLR9(0x00)  /* Independent. Set and clear do not depend on any counter. */
#define SCT_OUTPUTDIRCTRL_SETCLR9_L_COUNTER SCT_OUTPUTDIRCTRL_SETCLR9(0x01)  /* L counter. Set and clear are reversed when counter L or the unified counter is counting down. */
#define SCT_OUTPUTDIRCTRL_SETCLR9_H_COUNTER SCT_OUTPUTDIRCTRL_SETCLR9(0x02)  /* H counter. Set and clear are reversed when counter H is counting down. Do not use if UNIFY = 1. */

/* -- SCT_OUTPUTDIRCTRL_SETCLR10: Set/clear operation on output 5. Value 0x3 is reserved. Do not program this value. */
#define SCT_OUTPUTDIRCTRL_SETCLR10_SHIFT (20)
#define SCT_OUTPUTDIRCTRL_SETCLR10_MASK (0x03 << SCT_OUTPUTDIRCTRL_SETCLR10_SHIFT)
#define SCT_OUTPUTDIRCTRL_SETCLR10(x) ((x) << SCT_OUTPUTDIRCTRL_SETCLR10_SHIFT)

/* SCT_OUTPUTDIRCTRL_SETCLR10_SETCLR10 values */
#define SCT_OUTPUTDIRCTRL_SETCLR10_INDEPENDENT SCT_OUTPUTDIRCTRL_SETCLR10(0x00)  /* Independent. Set and clear do not depend on any counter. */
#define SCT_OUTPUTDIRCTRL_SETCLR10_L_COUNTER SCT_OUTPUTDIRCTRL_SETCLR10(0x01)  /* L counter. Set and clear are reversed when counter L or the unified counter is counting down. */
#define SCT_OUTPUTDIRCTRL_SETCLR10_H_COUNTER SCT_OUTPUTDIRCTRL_SETCLR10(0x02)  /* H counter. Set and clear are reversed when counter H is counting down. Do not use if UNIFY = 1. */

/* -- SCT_OUTPUTDIRCTRL_SETCLR11: Set/clear operation on output 11. Value 0x3 is reserved. Do not program this value. */
#define SCT_OUTPUTDIRCTRL_SETCLR11_SHIFT (22)
#define SCT_OUTPUTDIRCTRL_SETCLR11_MASK (0x03 << SCT_OUTPUTDIRCTRL_SETCLR11_SHIFT)
#define SCT_OUTPUTDIRCTRL_SETCLR11(x) ((x) << SCT_OUTPUTDIRCTRL_SETCLR11_SHIFT)

/* SCT_OUTPUTDIRCTRL_SETCLR11_SETCLR11 values */
#define SCT_OUTPUTDIRCTRL_SETCLR11_INDEPENDENT SCT_OUTPUTDIRCTRL_SETCLR11(0x00)  /* Independent. Set and clear do not depend on any counter. */
#define SCT_OUTPUTDIRCTRL_SETCLR11_L_COUNTER SCT_OUTPUTDIRCTRL_SETCLR11(0x01)  /* L counter. Set and clear are reversed when counter L or the unified counter is counting down. */
#define SCT_OUTPUTDIRCTRL_SETCLR11_H_COUNTER SCT_OUTPUTDIRCTRL_SETCLR11(0x02)  /* H counter. Set and clear are reversed when counter H is counting down. Do not use if UNIFY = 1. */

/* -- SCT_OUTPUTDIRCTRL_SETCLR12: Set/clear operation on output 12. Value 0x3 is reserved. Do not program this value. */
#define SCT_OUTPUTDIRCTRL_SETCLR12_SHIFT (24)
#define SCT_OUTPUTDIRCTRL_SETCLR12_MASK (0x03 << SCT_OUTPUTDIRCTRL_SETCLR12_SHIFT)
#define SCT_OUTPUTDIRCTRL_SETCLR12(x) ((x) << SCT_OUTPUTDIRCTRL_SETCLR12_SHIFT)

/* SCT_OUTPUTDIRCTRL_SETCLR12_SETCLR12 values */
#define SCT_OUTPUTDIRCTRL_SETCLR12_INDEPENDENT SCT_OUTPUTDIRCTRL_SETCLR12(0x00)  /* Independent. Set and clear do not depend on any counter. */
#define SCT_OUTPUTDIRCTRL_SETCLR12_L_COUNTER SCT_OUTPUTDIRCTRL_SETCLR12(0x01)  /* L counter. Set and clear are reversed when counter L or the unified counter is counting down. */
#define SCT_OUTPUTDIRCTRL_SETCLR12_H_COUNTER SCT_OUTPUTDIRCTRL_SETCLR12(0x02)  /* H counter. Set and clear are reversed when counter H is counting down. Do not use if UNIFY = 1. */

/* -- SCT_OUTPUTDIRCTRL_SETCLR13: Set/clear operation on output 13. Value 0x3 is reserved. Do not program this value. */
#define SCT_OUTPUTDIRCTRL_SETCLR13_SHIFT (26)
#define SCT_OUTPUTDIRCTRL_SETCLR13_MASK (0x03 << SCT_OUTPUTDIRCTRL_SETCLR13_SHIFT)
#define SCT_OUTPUTDIRCTRL_SETCLR13(x) ((x) << SCT_OUTPUTDIRCTRL_SETCLR13_SHIFT)

/* SCT_OUTPUTDIRCTRL_SETCLR13_SETCLR13 values */
#define SCT_OUTPUTDIRCTRL_SETCLR13_INDEPENDENT SCT_OUTPUTDIRCTRL_SETCLR13(0x00)  /* Independent. Set and clear do not depend on any counter. */
#define SCT_OUTPUTDIRCTRL_SETCLR13_L_COUNTER SCT_OUTPUTDIRCTRL_SETCLR13(0x01)  /* L counter. Set and clear are reversed when counter L or the unified counter is counting down. */
#define SCT_OUTPUTDIRCTRL_SETCLR13_H_COUNTER SCT_OUTPUTDIRCTRL_SETCLR13(0x02)  /* H counter. Set and clear are reversed when counter H is counting down. Do not use if UNIFY = 1. */

/* -- SCT_OUTPUTDIRCTRL_SETCLR14: Set/clear operation on output 14. Value 0x3 is reserved. Do not program this value. */
#define SCT_OUTPUTDIRCTRL_SETCLR14_SHIFT (28)
#define SCT_OUTPUTDIRCTRL_SETCLR14_MASK (0x03 << SCT_OUTPUTDIRCTRL_SETCLR14_SHIFT)
#define SCT_OUTPUTDIRCTRL_SETCLR14(x) ((x) << SCT_OUTPUTDIRCTRL_SETCLR14_SHIFT)

/* SCT_OUTPUTDIRCTRL_SETCLR14_SETCLR14 values */
#define SCT_OUTPUTDIRCTRL_SETCLR14_INDEPENDENT SCT_OUTPUTDIRCTRL_SETCLR14(0x00)  /* Independent. Set and clear do not depend on any counter. */
#define SCT_OUTPUTDIRCTRL_SETCLR14_L_COUNTER SCT_OUTPUTDIRCTRL_SETCLR14(0x01)  /* L counter. Set and clear are reversed when counter L or the unified counter is counting down. */
#define SCT_OUTPUTDIRCTRL_SETCLR14_H_COUNTER SCT_OUTPUTDIRCTRL_SETCLR14(0x02)  /* H counter. Set and clear are reversed when counter H is counting down. Do not use if UNIFY = 1. */

/* -- SCT_OUTPUTDIRCTRL_SETCLR15: Set/clear operation on output 15. Value 0x3 is reserved. Do not program this value. */
#define SCT_OUTPUTDIRCTRL_SETCLR15_SHIFT (30)
#define SCT_OUTPUTDIRCTRL_SETCLR15_MASK (0x03 << SCT_OUTPUTDIRCTRL_SETCLR15_SHIFT)
#define SCT_OUTPUTDIRCTRL_SETCLR15(x) ((x) << SCT_OUTPUTDIRCTRL_SETCLR15_SHIFT)

/* SCT_OUTPUTDIRCTRL_SETCLR15_SETCLR15 values */
#define SCT_OUTPUTDIRCTRL_SETCLR15_INDEPENDENT SCT_OUTPUTDIRCTRL_SETCLR15(0x00)  /* Independent. Set and clear do not depend on any counter. */
#define SCT_OUTPUTDIRCTRL_SETCLR15_L_COUNTER SCT_OUTPUTDIRCTRL_SETCLR15(0x01)  /* L counter. Set and clear are reversed when counter L or the unified counter is counting down. */
#define SCT_OUTPUTDIRCTRL_SETCLR15_H_COUNTER SCT_OUTPUTDIRCTRL_SETCLR15(0x02)  /* H counter. Set and clear are reversed when counter H is counting down. Do not use if UNIFY = 1. */

/* --- SCT_RES ------------------------------------------- */
#define SCT_RES MMIO32(SCT_BASE + 0x58)

/* -- SCT_RES_O0RES: Effect of simultaneous set and clear on output 0. */
#define SCT_RES_O0RES_SHIFT (0)
#define SCT_RES_O0RES_MASK (0x03 << SCT_RES_O0RES_SHIFT)
#define SCT_RES_O0RES(x) ((x) << SCT_RES_O0RES_SHIFT)

/* SCT_RES_O0RES_O0RES values */
#define SCT_RES_O0RES_NO_CHANGE SCT_RES_O0RES(0x00)  /* No change. */
#define SCT_RES_O0RES_SET_OUTPUT_OR_CLEAR SCT_RES_O0RES(0x01)  /* Set output (or clear based on the SETCLR0 field). */
#define SCT_RES_O0RES_CLEAR_OUTPUT_OR_SET SCT_RES_O0RES(0x02)  /* Clear output (or set based on the SETCLR0 field). */
#define SCT_RES_O0RES_TOGGLE_OUTPUT SCT_RES_O0RES(0x03)  /* Toggle output. */

/* -- SCT_RES_O1RES: Effect of simultaneous set and clear on output 1. */
#define SCT_RES_O1RES_SHIFT (2)
#define SCT_RES_O1RES_MASK (0x03 << SCT_RES_O1RES_SHIFT)
#define SCT_RES_O1RES(x) ((x) << SCT_RES_O1RES_SHIFT)

/* SCT_RES_O1RES_O1RES values */
#define SCT_RES_O1RES_NO_CHANGE SCT_RES_O1RES(0x00)  /* No change. */
#define SCT_RES_O1RES_SET_OUTPUT_OR_CLEAR SCT_RES_O1RES(0x01)  /* Set output (or clear based on the SETCLR1 field). */
#define SCT_RES_O1RES_CLEAR_OUTPUT_OR_SET SCT_RES_O1RES(0x02)  /* Clear output (or set based on the SETCLR1 field). */
#define SCT_RES_O1RES_TOGGLE_OUTPUT SCT_RES_O1RES(0x03)  /* Toggle output. */

/* -- SCT_RES_O2RES: Effect of simultaneous set and clear on output 2. */
#define SCT_RES_O2RES_SHIFT (4)
#define SCT_RES_O2RES_MASK (0x03 << SCT_RES_O2RES_SHIFT)
#define SCT_RES_O2RES(x) ((x) << SCT_RES_O2RES_SHIFT)

/* SCT_RES_O2RES_O2RES values */
#define SCT_RES_O2RES_NO_CHANGE SCT_RES_O2RES(0x00)  /* No change. */
#define SCT_RES_O2RES_SET_OUTPUT_OR_CLEAR SCT_RES_O2RES(0x01)  /* Set output (or clear based on the SETCLR2 field). */
#define SCT_RES_O2RES_CLEAR_OUTPUT_N_OR_S SCT_RES_O2RES(0x02)  /* Clear output n (or set based on the SETCLR2 field). */
#define SCT_RES_O2RES_TOGGLE_OUTPUT SCT_RES_O2RES(0x03)  /* Toggle output. */

/* -- SCT_RES_O3RES: Effect of simultaneous set and clear on output 3. */
#define SCT_RES_O3RES_SHIFT (6)
#define SCT_RES_O3RES_MASK (0x03 << SCT_RES_O3RES_SHIFT)
#define SCT_RES_O3RES(x) ((x) << SCT_RES_O3RES_SHIFT)

/* SCT_RES_O3RES_O3RES values */
#define SCT_RES_O3RES_NO_CHANGE SCT_RES_O3RES(0x00)  /* No change. */
#define SCT_RES_O3RES_SET_OUTPUT_OR_CLEAR SCT_RES_O3RES(0x01)  /* Set output (or clear based on the SETCLR3 field). */
#define SCT_RES_O3RES_CLEAR_OUTPUT_OR_SET SCT_RES_O3RES(0x02)  /* Clear output (or set based on the SETCLR3 field). */
#define SCT_RES_O3RES_TOGGLE_OUTPUT SCT_RES_O3RES(0x03)  /* Toggle output. */

/* -- SCT_RES_O4RES: Effect of simultaneous set and clear on output 4. */
#define SCT_RES_O4RES_SHIFT (8)
#define SCT_RES_O4RES_MASK (0x03 << SCT_RES_O4RES_SHIFT)
#define SCT_RES_O4RES(x) ((x) << SCT_RES_O4RES_SHIFT)

/* SCT_RES_O4RES_O4RES values */
#define SCT_RES_O4RES_NO_CHANGE SCT_RES_O4RES(0x00)  /* No change. */
#define SCT_RES_O4RES_SET_OUTPUT_OR_CLEAR SCT_RES_O4RES(0x01)  /* Set output (or clear based on the SETCLR4 field). */
#define SCT_RES_O4RES_CLEAR_OUTPUT_OR_SET SCT_RES_O4RES(0x02)  /* Clear output (or set based on the SETCLR4 field). */
#define SCT_RES_O4RES_TOGGLE_OUTPUT SCT_RES_O4RES(0x03)  /* Toggle output. */

/* -- SCT_RES_O5RES: Effect of simultaneous set and clear on output 5. */
#define SCT_RES_O5RES_SHIFT (10)
#define SCT_RES_O5RES_MASK (0x03 << SCT_RES_O5RES_SHIFT)
#define SCT_RES_O5RES(x) ((x) << SCT_RES_O5RES_SHIFT)

/* SCT_RES_O5RES_O5RES values */
#define SCT_RES_O5RES_NO_CHANGE SCT_RES_O5RES(0x00)  /* No change. */
#define SCT_RES_O5RES_SET_OUTPUT_OR_CLEAR SCT_RES_O5RES(0x01)  /* Set output (or clear based on the SETCLR5 field). */
#define SCT_RES_O5RES_CLEAR_OUTPUT_OR_SET SCT_RES_O5RES(0x02)  /* Clear output (or set based on the SETCLR5 field). */
#define SCT_RES_O5RES_TOGGLE_OUTPUT SCT_RES_O5RES(0x03)  /* Toggle output. */

/* -- SCT_RES_O6RES: Effect of simultaneous set and clear on output 6. */
#define SCT_RES_O6RES_SHIFT (12)
#define SCT_RES_O6RES_MASK (0x03 << SCT_RES_O6RES_SHIFT)
#define SCT_RES_O6RES(x) ((x) << SCT_RES_O6RES_SHIFT)

/* SCT_RES_O6RES_O6RES values */
#define SCT_RES_O6RES_NO_CHANGE SCT_RES_O6RES(0x00)  /* No change. */
#define SCT_RES_O6RES_SET_OUTPUT_OR_CLEAR SCT_RES_O6RES(0x01)  /* Set output (or clear based on the SETCLR6 field). */
#define SCT_RES_O6RES_CLEAR_OUTPUT_OR_SET SCT_RES_O6RES(0x02)  /* Clear output (or set based on the SETCLR6 field). */
#define SCT_RES_O6RES_TOGGLE_OUTPUT SCT_RES_O6RES(0x03)  /* Toggle output. */

/* -- SCT_RES_O7RES: Effect of simultaneous set and clear on output 7. */
#define SCT_RES_O7RES_SHIFT (14)
#define SCT_RES_O7RES_MASK (0x03 << SCT_RES_O7RES_SHIFT)
#define SCT_RES_O7RES(x) ((x) << SCT_RES_O7RES_SHIFT)

/* SCT_RES_O7RES_O7RES values */
#define SCT_RES_O7RES_NO_CHANGE SCT_RES_O7RES(0x00)  /* No change. */
#define SCT_RES_O7RES_SET_OUTPUT_OR_CLEAR SCT_RES_O7RES(0x01)  /* Set output (or clear based on the SETCLR7 field). */
#define SCT_RES_O7RES_CLEAR_OUTPUT_OR_SET SCT_RES_O7RES(0x02)  /* Clear output (or set based on the SETCLR7 field). */
#define SCT_RES_O7RES_TOGGLE_OUTPUT SCT_RES_O7RES(0x03)  /* Toggle output. */

/* -- SCT_RES_O8RES: Effect of simultaneous set and clear on output 8. */
#define SCT_RES_O8RES_SHIFT (16)
#define SCT_RES_O8RES_MASK (0x03 << SCT_RES_O8RES_SHIFT)
#define SCT_RES_O8RES(x) ((x) << SCT_RES_O8RES_SHIFT)

/* SCT_RES_O8RES_O8RES values */
#define SCT_RES_O8RES_NO_CHANGE SCT_RES_O8RES(0x00)  /* No change. */
#define SCT_RES_O8RES_SET_OUTPUT_OR_CLEAR SCT_RES_O8RES(0x01)  /* Set output (or clear based on the SETCLR8 field). */
#define SCT_RES_O8RES_CLEAR_OUTPUT_OR_SET SCT_RES_O8RES(0x02)  /* Clear output (or set based on the SETCLR8 field). */
#define SCT_RES_O8RES_TOGGLE_OUTPUT SCT_RES_O8RES(0x03)  /* Toggle output. */

/* -- SCT_RES_O9RES: Effect of simultaneous set and clear on output 9. */
#define SCT_RES_O9RES_SHIFT (18)
#define SCT_RES_O9RES_MASK (0x03 << SCT_RES_O9RES_SHIFT)
#define SCT_RES_O9RES(x) ((x) << SCT_RES_O9RES_SHIFT)

/* SCT_RES_O9RES_O9RES values */
#define SCT_RES_O9RES_NO_CHANGE SCT_RES_O9RES(0x00)  /* No change. */
#define SCT_RES_O9RES_SET_OUTPUT_OR_CLEAR SCT_RES_O9RES(0x01)  /* Set output (or clear based on the SETCLR9 field). */
#define SCT_RES_O9RES_CLEAR_OUTPUT_OR_SET SCT_RES_O9RES(0x02)  /* Clear output (or set based on the SETCLR9 field). */
#define SCT_RES_O9RES_TOGGLE_OUTPUT SCT_RES_O9RES(0x03)  /* Toggle output. */

/* -- SCT_RES_O10RES: Effect of simultaneous set and clear on output 10. */
#define SCT_RES_O10RES_SHIFT (20)
#define SCT_RES_O10RES_MASK (0x03 << SCT_RES_O10RES_SHIFT)
#define SCT_RES_O10RES(x) ((x) << SCT_RES_O10RES_SHIFT)

/* SCT_RES_O10RES_O10RES values */
#define SCT_RES_O10RES_NO_CHANGE SCT_RES_O10RES(0x00)  /* No change. */
#define SCT_RES_O10RES_SET_OUTPUT_OR_CLEAR SCT_RES_O10RES(0x01)  /* Set output (or clear based on the SETCLR10 field). */
#define SCT_RES_O10RES_CLEAR_OUTPUT_OR_SET SCT_RES_O10RES(0x02)  /* Clear output (or set based on the SETCLR10 field). */
#define SCT_RES_O10RES_TOGGLE_OUTPUT SCT_RES_O10RES(0x03)  /* Toggle output. */

/* -- SCT_RES_O11RES: Effect of simultaneous set and clear on output 11. */
#define SCT_RES_O11RES_SHIFT (22)
#define SCT_RES_O11RES_MASK (0x03 << SCT_RES_O11RES_SHIFT)
#define SCT_RES_O11RES(x) ((x) << SCT_RES_O11RES_SHIFT)

/* SCT_RES_O11RES_O11RES values */
#define SCT_RES_O11RES_NO_CHANGE SCT_RES_O11RES(0x00)  /* No change. */
#define SCT_RES_O11RES_SET_OUTPUT_OR_CLEAR SCT_RES_O11RES(0x01)  /* Set output (or clear based on the SETCLR11 field). */
#define SCT_RES_O11RES_CLEAR_OUTPUT_OR_SET SCT_RES_O11RES(0x02)  /* Clear output (or set based on the SETCLR11 field). */
#define SCT_RES_O11RES_TOGGLE_OUTPUT SCT_RES_O11RES(0x03)  /* Toggle output. */

/* -- SCT_RES_O12RES: Effect of simultaneous set and clear on output 12. */
#define SCT_RES_O12RES_SHIFT (24)
#define SCT_RES_O12RES_MASK (0x03 << SCT_RES_O12RES_SHIFT)
#define SCT_RES_O12RES(x) ((x) << SCT_RES_O12RES_SHIFT)

/* SCT_RES_O12RES_O12RES values */
#define SCT_RES_O12RES_NO_CHANGE SCT_RES_O12RES(0x00)  /* No change. */
#define SCT_RES_O12RES_SET_OUTPUT_OR_CLEAR SCT_RES_O12RES(0x01)  /* Set output (or clear based on the SETCLR12 field). */
#define SCT_RES_O12RES_CLEAR_OUTPUT_OR_SET SCT_RES_O12RES(0x02)  /* Clear output (or set based on the SETCLR12 field). */
#define SCT_RES_O12RES_TOGGLE_OUTPUT SCT_RES_O12RES(0x03)  /* Toggle output. */

/* -- SCT_RES_O13RES: Effect of simultaneous set and clear on output 13. */
#define SCT_RES_O13RES_SHIFT (26)
#define SCT_RES_O13RES_MASK (0x03 << SCT_RES_O13RES_SHIFT)
#define SCT_RES_O13RES(x) ((x) << SCT_RES_O13RES_SHIFT)

/* SCT_RES_O13RES_O13RES values */
#define SCT_RES_O13RES_NO_CHANGE SCT_RES_O13RES(0x00)  /* No change. */
#define SCT_RES_O13RES_SET_OUTPUT_OR_CLEAR SCT_RES_O13RES(0x01)  /* Set output (or clear based on the SETCLR13 field). */
#define SCT_RES_O13RES_CLEAR_OUTPUT_OR_SET SCT_RES_O13RES(0x02)  /* Clear output (or set based on the SETCLR13 field). */
#define SCT_RES_O13RES_TOGGLE_OUTPUT SCT_RES_O13RES(0x03)  /* Toggle output. */

/* -- SCT_RES_O14RES: Effect of simultaneous set and clear on output 14. */
#define SCT_RES_O14RES_SHIFT (28)
#define SCT_RES_O14RES_MASK (0x03 << SCT_RES_O14RES_SHIFT)
#define SCT_RES_O14RES(x) ((x) << SCT_RES_O14RES_SHIFT)

/* SCT_RES_O14RES_O14RES values */
#define SCT_RES_O14RES_NO_CHANGE SCT_RES_O14RES(0x00)  /* No change. */
#define SCT_RES_O14RES_SET_OUTPUT_OR_CLEAR SCT_RES_O14RES(0x01)  /* Set output (or clear based on the SETCLR14 field). */
#define SCT_RES_O14RES_CLEAR_OUTPUT_OR_SET SCT_RES_O14RES(0x02)  /* Clear output (or set based on the SETCLR14 field). */
#define SCT_RES_O14RES_TOGGLE_OUTPUT SCT_RES_O14RES(0x03)  /* Toggle output. */

/* -- SCT_RES_O15RES: Effect of simultaneous set and clear on output 15. */
#define SCT_RES_O15RES_SHIFT (30)
#define SCT_RES_O15RES_MASK (0x03 << SCT_RES_O15RES_SHIFT)
#define SCT_RES_O15RES(x) ((x) << SCT_RES_O15RES_SHIFT)

/* SCT_RES_O15RES_O15RES values */
#define SCT_RES_O15RES_NO_CHANGE SCT_RES_O15RES(0x00)  /* No change. */
#define SCT_RES_O15RES_SET_OUTPUT_OR_CLEAR SCT_RES_O15RES(0x01)  /* Set output (or clear based on the SETCLR15 field). */
#define SCT_RES_O15RES_CLEAR_OUTPUT_OR_SET SCT_RES_O15RES(0x02)  /* Clear output (or set based on the SETCLR15 field). */
#define SCT_RES_O15RES_TOGGLE_OUTPUT SCT_RES_O15RES(0x03)  /* Toggle output. */

/* --- SCT_DMAREQ0 --------------------------------------- */
#define SCT_DMAREQ0 MMIO32(SCT_BASE + 0x5C)

/* -- SCT_DMAREQ0_DEV_00: If bit n is one, event n sets DMA request 0 (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_DMAREQ0_DEV_00_SHIFT (0)
#define SCT_DMAREQ0_DEV_00_MASK (0x01 << SCT_DMAREQ0_DEV_00_SHIFT)
#define SCT_DMAREQ0_DEV_00(x) ((x) << SCT_DMAREQ0_DEV_00_SHIFT)

/* -- SCT_DMAREQ0_DEV_01: If bit n is one, event n sets DMA request 0 (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_DMAREQ0_DEV_01_SHIFT (1)
#define SCT_DMAREQ0_DEV_01_MASK (0x01 << SCT_DMAREQ0_DEV_01_SHIFT)
#define SCT_DMAREQ0_DEV_01(x) ((x) << SCT_DMAREQ0_DEV_01_SHIFT)

/* -- SCT_DMAREQ0_DEV_02: If bit n is one, event n sets DMA request 0 (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_DMAREQ0_DEV_02_SHIFT (2)
#define SCT_DMAREQ0_DEV_02_MASK (0x01 << SCT_DMAREQ0_DEV_02_SHIFT)
#define SCT_DMAREQ0_DEV_02(x) ((x) << SCT_DMAREQ0_DEV_02_SHIFT)

/* -- SCT_DMAREQ0_DEV_03: If bit n is one, event n sets DMA request 0 (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_DMAREQ0_DEV_03_SHIFT (3)
#define SCT_DMAREQ0_DEV_03_MASK (0x01 << SCT_DMAREQ0_DEV_03_SHIFT)
#define SCT_DMAREQ0_DEV_03(x) ((x) << SCT_DMAREQ0_DEV_03_SHIFT)

/* -- SCT_DMAREQ0_DEV_04: If bit n is one, event n sets DMA request 0 (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_DMAREQ0_DEV_04_SHIFT (4)
#define SCT_DMAREQ0_DEV_04_MASK (0x01 << SCT_DMAREQ0_DEV_04_SHIFT)
#define SCT_DMAREQ0_DEV_04(x) ((x) << SCT_DMAREQ0_DEV_04_SHIFT)

/* -- SCT_DMAREQ0_DEV_05: If bit n is one, event n sets DMA request 0 (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_DMAREQ0_DEV_05_SHIFT (5)
#define SCT_DMAREQ0_DEV_05_MASK (0x01 << SCT_DMAREQ0_DEV_05_SHIFT)
#define SCT_DMAREQ0_DEV_05(x) ((x) << SCT_DMAREQ0_DEV_05_SHIFT)

/* -- SCT_DMAREQ0_DEV_06: If bit n is one, event n sets DMA request 0 (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_DMAREQ0_DEV_06_SHIFT (6)
#define SCT_DMAREQ0_DEV_06_MASK (0x01 << SCT_DMAREQ0_DEV_06_SHIFT)
#define SCT_DMAREQ0_DEV_06(x) ((x) << SCT_DMAREQ0_DEV_06_SHIFT)

/* -- SCT_DMAREQ0_DEV_07: If bit n is one, event n sets DMA request 0 (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_DMAREQ0_DEV_07_SHIFT (7)
#define SCT_DMAREQ0_DEV_07_MASK (0x01 << SCT_DMAREQ0_DEV_07_SHIFT)
#define SCT_DMAREQ0_DEV_07(x) ((x) << SCT_DMAREQ0_DEV_07_SHIFT)

/* -- SCT_DMAREQ0_DEV_08: If bit n is one, event n sets DMA request 0 (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_DMAREQ0_DEV_08_SHIFT (8)
#define SCT_DMAREQ0_DEV_08_MASK (0x01 << SCT_DMAREQ0_DEV_08_SHIFT)
#define SCT_DMAREQ0_DEV_08(x) ((x) << SCT_DMAREQ0_DEV_08_SHIFT)

/* -- SCT_DMAREQ0_DEV_09: If bit n is one, event n sets DMA request 0 (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_DMAREQ0_DEV_09_SHIFT (9)
#define SCT_DMAREQ0_DEV_09_MASK (0x01 << SCT_DMAREQ0_DEV_09_SHIFT)
#define SCT_DMAREQ0_DEV_09(x) ((x) << SCT_DMAREQ0_DEV_09_SHIFT)

/* -- SCT_DMAREQ0_DEV_010: If bit n is one, event n sets DMA request 0 (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_DMAREQ0_DEV_010_SHIFT (10)
#define SCT_DMAREQ0_DEV_010_MASK (0x01 << SCT_DMAREQ0_DEV_010_SHIFT)
#define SCT_DMAREQ0_DEV_010(x) ((x) << SCT_DMAREQ0_DEV_010_SHIFT)

/* -- SCT_DMAREQ0_DEV_011: If bit n is one, event n sets DMA request 0 (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_DMAREQ0_DEV_011_SHIFT (11)
#define SCT_DMAREQ0_DEV_011_MASK (0x01 << SCT_DMAREQ0_DEV_011_SHIFT)
#define SCT_DMAREQ0_DEV_011(x) ((x) << SCT_DMAREQ0_DEV_011_SHIFT)

/* -- SCT_DMAREQ0_DEV_012: If bit n is one, event n sets DMA request 0 (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_DMAREQ0_DEV_012_SHIFT (12)
#define SCT_DMAREQ0_DEV_012_MASK (0x01 << SCT_DMAREQ0_DEV_012_SHIFT)
#define SCT_DMAREQ0_DEV_012(x) ((x) << SCT_DMAREQ0_DEV_012_SHIFT)

/* -- SCT_DMAREQ0_DEV_013: If bit n is one, event n sets DMA request 0 (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_DMAREQ0_DEV_013_SHIFT (13)
#define SCT_DMAREQ0_DEV_013_MASK (0x01 << SCT_DMAREQ0_DEV_013_SHIFT)
#define SCT_DMAREQ0_DEV_013(x) ((x) << SCT_DMAREQ0_DEV_013_SHIFT)

/* -- SCT_DMAREQ0_DEV_014: If bit n is one, event n sets DMA request 0 (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_DMAREQ0_DEV_014_SHIFT (14)
#define SCT_DMAREQ0_DEV_014_MASK (0x01 << SCT_DMAREQ0_DEV_014_SHIFT)
#define SCT_DMAREQ0_DEV_014(x) ((x) << SCT_DMAREQ0_DEV_014_SHIFT)

/* -- SCT_DMAREQ0_DEV_015: If bit n is one, event n sets DMA request 0 (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_DMAREQ0_DEV_015_SHIFT (15)
#define SCT_DMAREQ0_DEV_015_MASK (0x01 << SCT_DMAREQ0_DEV_015_SHIFT)
#define SCT_DMAREQ0_DEV_015(x) ((x) << SCT_DMAREQ0_DEV_015_SHIFT)

/* -- SCT_DMAREQ0_DRL0: A 1 in this bit makes the SCT set DMA request 0 when it loads the  Match_L/Unified registers from the Reload_L/Unified registers. */
#define SCT_DMAREQ0_DRL0_SHIFT (30)
#define SCT_DMAREQ0_DRL0_MASK (0x01 << SCT_DMAREQ0_DRL0_SHIFT)
#define SCT_DMAREQ0_DRL0(x) ((x) << SCT_DMAREQ0_DRL0_SHIFT)

/* -- SCT_DMAREQ0_DRQ0: This read-only bit indicates the state of DMA Request 0 */
#define SCT_DMAREQ0_DRQ0_SHIFT (31)
#define SCT_DMAREQ0_DRQ0_MASK (0x01 << SCT_DMAREQ0_DRQ0_SHIFT)
#define SCT_DMAREQ0_DRQ0(x) ((x) << SCT_DMAREQ0_DRQ0_SHIFT)

/* --- SCT_DMAREQ1 --------------------------------------- */
#define SCT_DMAREQ1 MMIO32(SCT_BASE + 0x60)

/* -- SCT_DMAREQ1_DEV_10: If bit n is one, event n sets DMA request 1 (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_DMAREQ1_DEV_10_SHIFT (0)
#define SCT_DMAREQ1_DEV_10_MASK (0x01 << SCT_DMAREQ1_DEV_10_SHIFT)
#define SCT_DMAREQ1_DEV_10(x) ((x) << SCT_DMAREQ1_DEV_10_SHIFT)

/* -- SCT_DMAREQ1_DEV_11: If bit n is one, event n sets DMA request 1 (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_DMAREQ1_DEV_11_SHIFT (1)
#define SCT_DMAREQ1_DEV_11_MASK (0x01 << SCT_DMAREQ1_DEV_11_SHIFT)
#define SCT_DMAREQ1_DEV_11(x) ((x) << SCT_DMAREQ1_DEV_11_SHIFT)

/* -- SCT_DMAREQ1_DEV_12: If bit n is one, event n sets DMA request 1 (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_DMAREQ1_DEV_12_SHIFT (2)
#define SCT_DMAREQ1_DEV_12_MASK (0x01 << SCT_DMAREQ1_DEV_12_SHIFT)
#define SCT_DMAREQ1_DEV_12(x) ((x) << SCT_DMAREQ1_DEV_12_SHIFT)

/* -- SCT_DMAREQ1_DEV_13: If bit n is one, event n sets DMA request 1 (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_DMAREQ1_DEV_13_SHIFT (3)
#define SCT_DMAREQ1_DEV_13_MASK (0x01 << SCT_DMAREQ1_DEV_13_SHIFT)
#define SCT_DMAREQ1_DEV_13(x) ((x) << SCT_DMAREQ1_DEV_13_SHIFT)

/* -- SCT_DMAREQ1_DEV_14: If bit n is one, event n sets DMA request 1 (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_DMAREQ1_DEV_14_SHIFT (4)
#define SCT_DMAREQ1_DEV_14_MASK (0x01 << SCT_DMAREQ1_DEV_14_SHIFT)
#define SCT_DMAREQ1_DEV_14(x) ((x) << SCT_DMAREQ1_DEV_14_SHIFT)

/* -- SCT_DMAREQ1_DEV_15: If bit n is one, event n sets DMA request 1 (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_DMAREQ1_DEV_15_SHIFT (5)
#define SCT_DMAREQ1_DEV_15_MASK (0x01 << SCT_DMAREQ1_DEV_15_SHIFT)
#define SCT_DMAREQ1_DEV_15(x) ((x) << SCT_DMAREQ1_DEV_15_SHIFT)

/* -- SCT_DMAREQ1_DEV_16: If bit n is one, event n sets DMA request 1 (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_DMAREQ1_DEV_16_SHIFT (6)
#define SCT_DMAREQ1_DEV_16_MASK (0x01 << SCT_DMAREQ1_DEV_16_SHIFT)
#define SCT_DMAREQ1_DEV_16(x) ((x) << SCT_DMAREQ1_DEV_16_SHIFT)

/* -- SCT_DMAREQ1_DEV_17: If bit n is one, event n sets DMA request 1 (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_DMAREQ1_DEV_17_SHIFT (7)
#define SCT_DMAREQ1_DEV_17_MASK (0x01 << SCT_DMAREQ1_DEV_17_SHIFT)
#define SCT_DMAREQ1_DEV_17(x) ((x) << SCT_DMAREQ1_DEV_17_SHIFT)

/* -- SCT_DMAREQ1_DEV_18: If bit n is one, event n sets DMA request 1 (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_DMAREQ1_DEV_18_SHIFT (8)
#define SCT_DMAREQ1_DEV_18_MASK (0x01 << SCT_DMAREQ1_DEV_18_SHIFT)
#define SCT_DMAREQ1_DEV_18(x) ((x) << SCT_DMAREQ1_DEV_18_SHIFT)

/* -- SCT_DMAREQ1_DEV_19: If bit n is one, event n sets DMA request 1 (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_DMAREQ1_DEV_19_SHIFT (9)
#define SCT_DMAREQ1_DEV_19_MASK (0x01 << SCT_DMAREQ1_DEV_19_SHIFT)
#define SCT_DMAREQ1_DEV_19(x) ((x) << SCT_DMAREQ1_DEV_19_SHIFT)

/* -- SCT_DMAREQ1_DEV_110: If bit n is one, event n sets DMA request 1 (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_DMAREQ1_DEV_110_SHIFT (10)
#define SCT_DMAREQ1_DEV_110_MASK (0x01 << SCT_DMAREQ1_DEV_110_SHIFT)
#define SCT_DMAREQ1_DEV_110(x) ((x) << SCT_DMAREQ1_DEV_110_SHIFT)

/* -- SCT_DMAREQ1_DEV_111: If bit n is one, event n sets DMA request 1 (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_DMAREQ1_DEV_111_SHIFT (11)
#define SCT_DMAREQ1_DEV_111_MASK (0x01 << SCT_DMAREQ1_DEV_111_SHIFT)
#define SCT_DMAREQ1_DEV_111(x) ((x) << SCT_DMAREQ1_DEV_111_SHIFT)

/* -- SCT_DMAREQ1_DEV_112: If bit n is one, event n sets DMA request 1 (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_DMAREQ1_DEV_112_SHIFT (12)
#define SCT_DMAREQ1_DEV_112_MASK (0x01 << SCT_DMAREQ1_DEV_112_SHIFT)
#define SCT_DMAREQ1_DEV_112(x) ((x) << SCT_DMAREQ1_DEV_112_SHIFT)

/* -- SCT_DMAREQ1_DEV_113: If bit n is one, event n sets DMA request 1 (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_DMAREQ1_DEV_113_SHIFT (13)
#define SCT_DMAREQ1_DEV_113_MASK (0x01 << SCT_DMAREQ1_DEV_113_SHIFT)
#define SCT_DMAREQ1_DEV_113(x) ((x) << SCT_DMAREQ1_DEV_113_SHIFT)

/* -- SCT_DMAREQ1_DEV_114: If bit n is one, event n sets DMA request 1 (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_DMAREQ1_DEV_114_SHIFT (14)
#define SCT_DMAREQ1_DEV_114_MASK (0x01 << SCT_DMAREQ1_DEV_114_SHIFT)
#define SCT_DMAREQ1_DEV_114(x) ((x) << SCT_DMAREQ1_DEV_114_SHIFT)

/* -- SCT_DMAREQ1_DEV_115: If bit n is one, event n sets DMA request 1 (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_DMAREQ1_DEV_115_SHIFT (15)
#define SCT_DMAREQ1_DEV_115_MASK (0x01 << SCT_DMAREQ1_DEV_115_SHIFT)
#define SCT_DMAREQ1_DEV_115(x) ((x) << SCT_DMAREQ1_DEV_115_SHIFT)

/* -- SCT_DMAREQ1_DRL1: A 1 in this bit makes the SCT set DMA request 1 when it loads the  Match L/Unified registers from the Reload L/Unified registers. */
#define SCT_DMAREQ1_DRL1_SHIFT (30)
#define SCT_DMAREQ1_DRL1_MASK (0x01 << SCT_DMAREQ1_DRL1_SHIFT)
#define SCT_DMAREQ1_DRL1(x) ((x) << SCT_DMAREQ1_DRL1_SHIFT)

/* -- SCT_DMAREQ1_DRQ1: This read-only bit indicates the state of DMA Request 1. */
#define SCT_DMAREQ1_DRQ1_SHIFT (31)
#define SCT_DMAREQ1_DRQ1_MASK (0x01 << SCT_DMAREQ1_DRQ1_SHIFT)
#define SCT_DMAREQ1_DRQ1(x) ((x) << SCT_DMAREQ1_DRQ1_SHIFT)

/* --- SCT_EVEN ------------------------------------------ */
#define SCT_EVEN MMIO32(SCT_BASE + 0xF0)

/* -- SCT_EVEN_IEN0: The SCT requests interrupt when bit n of this register and the event flag register are both one (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_EVEN_IEN0_SHIFT (0)
#define SCT_EVEN_IEN0_MASK (0x01 << SCT_EVEN_IEN0_SHIFT)
#define SCT_EVEN_IEN0(x) ((x) << SCT_EVEN_IEN0_SHIFT)

/* -- SCT_EVEN_IEN1: The SCT requests interrupt when bit n of this register and the event flag register are both one (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_EVEN_IEN1_SHIFT (1)
#define SCT_EVEN_IEN1_MASK (0x01 << SCT_EVEN_IEN1_SHIFT)
#define SCT_EVEN_IEN1(x) ((x) << SCT_EVEN_IEN1_SHIFT)

/* -- SCT_EVEN_IEN2: The SCT requests interrupt when bit n of this register and the event flag register are both one (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_EVEN_IEN2_SHIFT (2)
#define SCT_EVEN_IEN2_MASK (0x01 << SCT_EVEN_IEN2_SHIFT)
#define SCT_EVEN_IEN2(x) ((x) << SCT_EVEN_IEN2_SHIFT)

/* -- SCT_EVEN_IEN3: The SCT requests interrupt when bit n of this register and the event flag register are both one (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_EVEN_IEN3_SHIFT (3)
#define SCT_EVEN_IEN3_MASK (0x01 << SCT_EVEN_IEN3_SHIFT)
#define SCT_EVEN_IEN3(x) ((x) << SCT_EVEN_IEN3_SHIFT)

/* -- SCT_EVEN_IEN4: The SCT requests interrupt when bit n of this register and the event flag register are both one (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_EVEN_IEN4_SHIFT (4)
#define SCT_EVEN_IEN4_MASK (0x01 << SCT_EVEN_IEN4_SHIFT)
#define SCT_EVEN_IEN4(x) ((x) << SCT_EVEN_IEN4_SHIFT)

/* -- SCT_EVEN_IEN5: The SCT requests interrupt when bit n of this register and the event flag register are both one (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_EVEN_IEN5_SHIFT (5)
#define SCT_EVEN_IEN5_MASK (0x01 << SCT_EVEN_IEN5_SHIFT)
#define SCT_EVEN_IEN5(x) ((x) << SCT_EVEN_IEN5_SHIFT)

/* -- SCT_EVEN_IEN6: The SCT requests interrupt when bit n of this register and the event flag register are both one (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_EVEN_IEN6_SHIFT (6)
#define SCT_EVEN_IEN6_MASK (0x01 << SCT_EVEN_IEN6_SHIFT)
#define SCT_EVEN_IEN6(x) ((x) << SCT_EVEN_IEN6_SHIFT)

/* -- SCT_EVEN_IEN7: The SCT requests interrupt when bit n of this register and the event flag register are both one (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_EVEN_IEN7_SHIFT (7)
#define SCT_EVEN_IEN7_MASK (0x01 << SCT_EVEN_IEN7_SHIFT)
#define SCT_EVEN_IEN7(x) ((x) << SCT_EVEN_IEN7_SHIFT)

/* -- SCT_EVEN_IEN8: The SCT requests interrupt when bit n of this register and the event flag register are both one (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_EVEN_IEN8_SHIFT (8)
#define SCT_EVEN_IEN8_MASK (0x01 << SCT_EVEN_IEN8_SHIFT)
#define SCT_EVEN_IEN8(x) ((x) << SCT_EVEN_IEN8_SHIFT)

/* -- SCT_EVEN_IEN9: The SCT requests interrupt when bit n of this register and the event flag register are both one (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_EVEN_IEN9_SHIFT (9)
#define SCT_EVEN_IEN9_MASK (0x01 << SCT_EVEN_IEN9_SHIFT)
#define SCT_EVEN_IEN9(x) ((x) << SCT_EVEN_IEN9_SHIFT)

/* -- SCT_EVEN_IEN10: The SCT requests interrupt when bit n of this register and the event flag register are both one (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_EVEN_IEN10_SHIFT (10)
#define SCT_EVEN_IEN10_MASK (0x01 << SCT_EVEN_IEN10_SHIFT)
#define SCT_EVEN_IEN10(x) ((x) << SCT_EVEN_IEN10_SHIFT)

/* -- SCT_EVEN_IEN11: The SCT requests interrupt when bit n of this register and the event flag register are both one (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_EVEN_IEN11_SHIFT (11)
#define SCT_EVEN_IEN11_MASK (0x01 << SCT_EVEN_IEN11_SHIFT)
#define SCT_EVEN_IEN11(x) ((x) << SCT_EVEN_IEN11_SHIFT)

/* -- SCT_EVEN_IEN12: The SCT requests interrupt when bit n of this register and the event flag register are both one (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_EVEN_IEN12_SHIFT (12)
#define SCT_EVEN_IEN12_MASK (0x01 << SCT_EVEN_IEN12_SHIFT)
#define SCT_EVEN_IEN12(x) ((x) << SCT_EVEN_IEN12_SHIFT)

/* -- SCT_EVEN_IEN13: The SCT requests interrupt when bit n of this register and the event flag register are both one (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_EVEN_IEN13_SHIFT (13)
#define SCT_EVEN_IEN13_MASK (0x01 << SCT_EVEN_IEN13_SHIFT)
#define SCT_EVEN_IEN13(x) ((x) << SCT_EVEN_IEN13_SHIFT)

/* -- SCT_EVEN_IEN14: The SCT requests interrupt when bit n of this register and the event flag register are both one (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_EVEN_IEN14_SHIFT (14)
#define SCT_EVEN_IEN14_MASK (0x01 << SCT_EVEN_IEN14_SHIFT)
#define SCT_EVEN_IEN14(x) ((x) << SCT_EVEN_IEN14_SHIFT)

/* -- SCT_EVEN_IEN15: The SCT requests interrupt when bit n of this register and the event flag register are both one (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_EVEN_IEN15_SHIFT (15)
#define SCT_EVEN_IEN15_MASK (0x01 << SCT_EVEN_IEN15_SHIFT)
#define SCT_EVEN_IEN15(x) ((x) << SCT_EVEN_IEN15_SHIFT)

/* --- SCT_EVFLAG ---------------------------------------- */
#define SCT_EVFLAG MMIO32(SCT_BASE + 0xF4)

/* -- SCT_EVFLAG_FLAG0: Bit n is one if event n has occurred since reset or a 1 was last written to this bit (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_EVFLAG_FLAG0_SHIFT (0)
#define SCT_EVFLAG_FLAG0_MASK (0x01 << SCT_EVFLAG_FLAG0_SHIFT)
#define SCT_EVFLAG_FLAG0(x) ((x) << SCT_EVFLAG_FLAG0_SHIFT)

/* -- SCT_EVFLAG_FLAG1: Bit n is one if event n has occurred since reset or a 1 was last written to this bit (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_EVFLAG_FLAG1_SHIFT (1)
#define SCT_EVFLAG_FLAG1_MASK (0x01 << SCT_EVFLAG_FLAG1_SHIFT)
#define SCT_EVFLAG_FLAG1(x) ((x) << SCT_EVFLAG_FLAG1_SHIFT)

/* -- SCT_EVFLAG_FLAG2: Bit n is one if event n has occurred since reset or a 1 was last written to this bit (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_EVFLAG_FLAG2_SHIFT (2)
#define SCT_EVFLAG_FLAG2_MASK (0x01 << SCT_EVFLAG_FLAG2_SHIFT)
#define SCT_EVFLAG_FLAG2(x) ((x) << SCT_EVFLAG_FLAG2_SHIFT)

/* -- SCT_EVFLAG_FLAG3: Bit n is one if event n has occurred since reset or a 1 was last written to this bit (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_EVFLAG_FLAG3_SHIFT (3)
#define SCT_EVFLAG_FLAG3_MASK (0x01 << SCT_EVFLAG_FLAG3_SHIFT)
#define SCT_EVFLAG_FLAG3(x) ((x) << SCT_EVFLAG_FLAG3_SHIFT)

/* -- SCT_EVFLAG_FLAG4: Bit n is one if event n has occurred since reset or a 1 was last written to this bit (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_EVFLAG_FLAG4_SHIFT (4)
#define SCT_EVFLAG_FLAG4_MASK (0x01 << SCT_EVFLAG_FLAG4_SHIFT)
#define SCT_EVFLAG_FLAG4(x) ((x) << SCT_EVFLAG_FLAG4_SHIFT)

/* -- SCT_EVFLAG_FLAG5: Bit n is one if event n has occurred since reset or a 1 was last written to this bit (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_EVFLAG_FLAG5_SHIFT (5)
#define SCT_EVFLAG_FLAG5_MASK (0x01 << SCT_EVFLAG_FLAG5_SHIFT)
#define SCT_EVFLAG_FLAG5(x) ((x) << SCT_EVFLAG_FLAG5_SHIFT)

/* -- SCT_EVFLAG_FLAG6: Bit n is one if event n has occurred since reset or a 1 was last written to this bit (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_EVFLAG_FLAG6_SHIFT (6)
#define SCT_EVFLAG_FLAG6_MASK (0x01 << SCT_EVFLAG_FLAG6_SHIFT)
#define SCT_EVFLAG_FLAG6(x) ((x) << SCT_EVFLAG_FLAG6_SHIFT)

/* -- SCT_EVFLAG_FLAG7: Bit n is one if event n has occurred since reset or a 1 was last written to this bit (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_EVFLAG_FLAG7_SHIFT (7)
#define SCT_EVFLAG_FLAG7_MASK (0x01 << SCT_EVFLAG_FLAG7_SHIFT)
#define SCT_EVFLAG_FLAG7(x) ((x) << SCT_EVFLAG_FLAG7_SHIFT)

/* -- SCT_EVFLAG_FLAG8: Bit n is one if event n has occurred since reset or a 1 was last written to this bit (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_EVFLAG_FLAG8_SHIFT (8)
#define SCT_EVFLAG_FLAG8_MASK (0x01 << SCT_EVFLAG_FLAG8_SHIFT)
#define SCT_EVFLAG_FLAG8(x) ((x) << SCT_EVFLAG_FLAG8_SHIFT)

/* -- SCT_EVFLAG_FLAG9: Bit n is one if event n has occurred since reset or a 1 was last written to this bit (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_EVFLAG_FLAG9_SHIFT (9)
#define SCT_EVFLAG_FLAG9_MASK (0x01 << SCT_EVFLAG_FLAG9_SHIFT)
#define SCT_EVFLAG_FLAG9(x) ((x) << SCT_EVFLAG_FLAG9_SHIFT)

/* -- SCT_EVFLAG_FLAG10: Bit n is one if event n has occurred since reset or a 1 was last written to this bit (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_EVFLAG_FLAG10_SHIFT (10)
#define SCT_EVFLAG_FLAG10_MASK (0x01 << SCT_EVFLAG_FLAG10_SHIFT)
#define SCT_EVFLAG_FLAG10(x) ((x) << SCT_EVFLAG_FLAG10_SHIFT)

/* -- SCT_EVFLAG_FLAG11: Bit n is one if event n has occurred since reset or a 1 was last written to this bit (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_EVFLAG_FLAG11_SHIFT (11)
#define SCT_EVFLAG_FLAG11_MASK (0x01 << SCT_EVFLAG_FLAG11_SHIFT)
#define SCT_EVFLAG_FLAG11(x) ((x) << SCT_EVFLAG_FLAG11_SHIFT)

/* -- SCT_EVFLAG_FLAG12: Bit n is one if event n has occurred since reset or a 1 was last written to this bit (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_EVFLAG_FLAG12_SHIFT (12)
#define SCT_EVFLAG_FLAG12_MASK (0x01 << SCT_EVFLAG_FLAG12_SHIFT)
#define SCT_EVFLAG_FLAG12(x) ((x) << SCT_EVFLAG_FLAG12_SHIFT)

/* -- SCT_EVFLAG_FLAG13: Bit n is one if event n has occurred since reset or a 1 was last written to this bit (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_EVFLAG_FLAG13_SHIFT (13)
#define SCT_EVFLAG_FLAG13_MASK (0x01 << SCT_EVFLAG_FLAG13_SHIFT)
#define SCT_EVFLAG_FLAG13(x) ((x) << SCT_EVFLAG_FLAG13_SHIFT)

/* -- SCT_EVFLAG_FLAG14: Bit n is one if event n has occurred since reset or a 1 was last written to this bit (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_EVFLAG_FLAG14_SHIFT (14)
#define SCT_EVFLAG_FLAG14_MASK (0x01 << SCT_EVFLAG_FLAG14_SHIFT)
#define SCT_EVFLAG_FLAG14(x) ((x) << SCT_EVFLAG_FLAG14_SHIFT)

/* -- SCT_EVFLAG_FLAG15: Bit n is one if event n has occurred since reset or a 1 was last written to this bit (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_EVFLAG_FLAG15_SHIFT (15)
#define SCT_EVFLAG_FLAG15_MASK (0x01 << SCT_EVFLAG_FLAG15_SHIFT)
#define SCT_EVFLAG_FLAG15(x) ((x) << SCT_EVFLAG_FLAG15_SHIFT)

/* --- SCT_CONEN ----------------------------------------- */
#define SCT_CONEN MMIO32(SCT_BASE + 0xF8)

/* -- SCT_CONEN_NCEN0: The SCT requests interrupt when bit n of this register and the SCT conflict flag register are both one (output 0 = bit 0, output 1 = bit 1,..., output 15 = bit 15). */
#define SCT_CONEN_NCEN0_SHIFT (0)
#define SCT_CONEN_NCEN0_MASK (0x01 << SCT_CONEN_NCEN0_SHIFT)
#define SCT_CONEN_NCEN0(x) ((x) << SCT_CONEN_NCEN0_SHIFT)

/* -- SCT_CONEN_NCEN1: The SCT requests interrupt when bit n of this register and the SCT conflict flag register are both one (output 0 = bit 0, output 1 = bit 1,..., output 15 = bit 15). */
#define SCT_CONEN_NCEN1_SHIFT (1)
#define SCT_CONEN_NCEN1_MASK (0x01 << SCT_CONEN_NCEN1_SHIFT)
#define SCT_CONEN_NCEN1(x) ((x) << SCT_CONEN_NCEN1_SHIFT)

/* -- SCT_CONEN_NCEN2: The SCT requests interrupt when bit n of this register and the SCT conflict flag register are both one (output 0 = bit 0, output 1 = bit 1,..., output 15 = bit 15). */
#define SCT_CONEN_NCEN2_SHIFT (2)
#define SCT_CONEN_NCEN2_MASK (0x01 << SCT_CONEN_NCEN2_SHIFT)
#define SCT_CONEN_NCEN2(x) ((x) << SCT_CONEN_NCEN2_SHIFT)

/* -- SCT_CONEN_NCEN3: The SCT requests interrupt when bit n of this register and the SCT conflict flag register are both one (output 0 = bit 0, output 1 = bit 1,..., output 15 = bit 15). */
#define SCT_CONEN_NCEN3_SHIFT (3)
#define SCT_CONEN_NCEN3_MASK (0x01 << SCT_CONEN_NCEN3_SHIFT)
#define SCT_CONEN_NCEN3(x) ((x) << SCT_CONEN_NCEN3_SHIFT)

/* -- SCT_CONEN_NCEN4: The SCT requests interrupt when bit n of this register and the SCT conflict flag register are both one (output 0 = bit 0, output 1 = bit 1,..., output 15 = bit 15). */
#define SCT_CONEN_NCEN4_SHIFT (4)
#define SCT_CONEN_NCEN4_MASK (0x01 << SCT_CONEN_NCEN4_SHIFT)
#define SCT_CONEN_NCEN4(x) ((x) << SCT_CONEN_NCEN4_SHIFT)

/* -- SCT_CONEN_NCEN5: The SCT requests interrupt when bit n of this register and the SCT conflict flag register are both one (output 0 = bit 0, output 1 = bit 1,..., output 15 = bit 15). */
#define SCT_CONEN_NCEN5_SHIFT (5)
#define SCT_CONEN_NCEN5_MASK (0x01 << SCT_CONEN_NCEN5_SHIFT)
#define SCT_CONEN_NCEN5(x) ((x) << SCT_CONEN_NCEN5_SHIFT)

/* -- SCT_CONEN_NCEN6: The SCT requests interrupt when bit n of this register and the SCT conflict flag register are both one (output 0 = bit 0, output 1 = bit 1,..., output 15 = bit 15). */
#define SCT_CONEN_NCEN6_SHIFT (6)
#define SCT_CONEN_NCEN6_MASK (0x01 << SCT_CONEN_NCEN6_SHIFT)
#define SCT_CONEN_NCEN6(x) ((x) << SCT_CONEN_NCEN6_SHIFT)

/* -- SCT_CONEN_NCEN7: The SCT requests interrupt when bit n of this register and the SCT conflict flag register are both one (output 0 = bit 0, output 1 = bit 1,..., output 15 = bit 15). */
#define SCT_CONEN_NCEN7_SHIFT (7)
#define SCT_CONEN_NCEN7_MASK (0x01 << SCT_CONEN_NCEN7_SHIFT)
#define SCT_CONEN_NCEN7(x) ((x) << SCT_CONEN_NCEN7_SHIFT)

/* -- SCT_CONEN_NCEN8: The SCT requests interrupt when bit n of this register and the SCT conflict flag register are both one (output 0 = bit 0, output 1 = bit 1,..., output 15 = bit 15). */
#define SCT_CONEN_NCEN8_SHIFT (8)
#define SCT_CONEN_NCEN8_MASK (0x01 << SCT_CONEN_NCEN8_SHIFT)
#define SCT_CONEN_NCEN8(x) ((x) << SCT_CONEN_NCEN8_SHIFT)

/* -- SCT_CONEN_NCEN9: The SCT requests interrupt when bit n of this register and the SCT conflict flag register are both one (output 0 = bit 0, output 1 = bit 1,..., output 15 = bit 15). */
#define SCT_CONEN_NCEN9_SHIFT (9)
#define SCT_CONEN_NCEN9_MASK (0x01 << SCT_CONEN_NCEN9_SHIFT)
#define SCT_CONEN_NCEN9(x) ((x) << SCT_CONEN_NCEN9_SHIFT)

/* -- SCT_CONEN_NCEN10: The SCT requests interrupt when bit n of this register and the SCT conflict flag register are both one (output 0 = bit 0, output 1 = bit 1,..., output 15 = bit 15). */
#define SCT_CONEN_NCEN10_SHIFT (10)
#define SCT_CONEN_NCEN10_MASK (0x01 << SCT_CONEN_NCEN10_SHIFT)
#define SCT_CONEN_NCEN10(x) ((x) << SCT_CONEN_NCEN10_SHIFT)

/* -- SCT_CONEN_NCEN11: The SCT requests interrupt when bit n of this register and the SCT conflict flag register are both one (output 0 = bit 0, output 1 = bit 1,..., output 15 = bit 15). */
#define SCT_CONEN_NCEN11_SHIFT (11)
#define SCT_CONEN_NCEN11_MASK (0x01 << SCT_CONEN_NCEN11_SHIFT)
#define SCT_CONEN_NCEN11(x) ((x) << SCT_CONEN_NCEN11_SHIFT)

/* -- SCT_CONEN_NCEN12: The SCT requests interrupt when bit n of this register and the SCT conflict flag register are both one (output 0 = bit 0, output 1 = bit 1,..., output 15 = bit 15). */
#define SCT_CONEN_NCEN12_SHIFT (12)
#define SCT_CONEN_NCEN12_MASK (0x01 << SCT_CONEN_NCEN12_SHIFT)
#define SCT_CONEN_NCEN12(x) ((x) << SCT_CONEN_NCEN12_SHIFT)

/* -- SCT_CONEN_NCEN13: The SCT requests interrupt when bit n of this register and the SCT conflict flag register are both one (output 0 = bit 0, output 1 = bit 1,..., output 15 = bit 15). */
#define SCT_CONEN_NCEN13_SHIFT (13)
#define SCT_CONEN_NCEN13_MASK (0x01 << SCT_CONEN_NCEN13_SHIFT)
#define SCT_CONEN_NCEN13(x) ((x) << SCT_CONEN_NCEN13_SHIFT)

/* -- SCT_CONEN_NCEN14: The SCT requests interrupt when bit n of this register and the SCT conflict flag register are both one (output 0 = bit 0, output 1 = bit 1,..., output 15 = bit 15). */
#define SCT_CONEN_NCEN14_SHIFT (14)
#define SCT_CONEN_NCEN14_MASK (0x01 << SCT_CONEN_NCEN14_SHIFT)
#define SCT_CONEN_NCEN14(x) ((x) << SCT_CONEN_NCEN14_SHIFT)

/* -- SCT_CONEN_NCEN15: The SCT requests interrupt when bit n of this register and the SCT conflict flag register are both one (output 0 = bit 0, output 1 = bit 1,..., output 15 = bit 15). */
#define SCT_CONEN_NCEN15_SHIFT (15)
#define SCT_CONEN_NCEN15_MASK (0x01 << SCT_CONEN_NCEN15_SHIFT)
#define SCT_CONEN_NCEN15(x) ((x) << SCT_CONEN_NCEN15_SHIFT)

/* --- SCT_CONFLAG --------------------------------------- */
#define SCT_CONFLAG MMIO32(SCT_BASE + 0xFC)

/* -- SCT_CONFLAG_NCFLAG0: Bit n is one if a no-change conflict event occurred on output n since  reset or a 1 was last written to this bit (output 0 = bit 0, output 1 = bit 1,..., output 15 = bit 15). */
#define SCT_CONFLAG_NCFLAG0_SHIFT (0)
#define SCT_CONFLAG_NCFLAG0_MASK (0x01 << SCT_CONFLAG_NCFLAG0_SHIFT)
#define SCT_CONFLAG_NCFLAG0(x) ((x) << SCT_CONFLAG_NCFLAG0_SHIFT)

/* -- SCT_CONFLAG_NCFLAG1: Bit n is one if a no-change conflict event occurred on output n since  reset or a 1 was last written to this bit (output 0 = bit 0, output 1 = bit 1,..., output 15 = bit 15). */
#define SCT_CONFLAG_NCFLAG1_SHIFT (1)
#define SCT_CONFLAG_NCFLAG1_MASK (0x01 << SCT_CONFLAG_NCFLAG1_SHIFT)
#define SCT_CONFLAG_NCFLAG1(x) ((x) << SCT_CONFLAG_NCFLAG1_SHIFT)

/* -- SCT_CONFLAG_NCFLAG2: Bit n is one if a no-change conflict event occurred on output n since  reset or a 1 was last written to this bit (output 0 = bit 0, output 1 = bit 1,..., output 15 = bit 15). */
#define SCT_CONFLAG_NCFLAG2_SHIFT (2)
#define SCT_CONFLAG_NCFLAG2_MASK (0x01 << SCT_CONFLAG_NCFLAG2_SHIFT)
#define SCT_CONFLAG_NCFLAG2(x) ((x) << SCT_CONFLAG_NCFLAG2_SHIFT)

/* -- SCT_CONFLAG_NCFLAG3: Bit n is one if a no-change conflict event occurred on output n since  reset or a 1 was last written to this bit (output 0 = bit 0, output 1 = bit 1,..., output 15 = bit 15). */
#define SCT_CONFLAG_NCFLAG3_SHIFT (3)
#define SCT_CONFLAG_NCFLAG3_MASK (0x01 << SCT_CONFLAG_NCFLAG3_SHIFT)
#define SCT_CONFLAG_NCFLAG3(x) ((x) << SCT_CONFLAG_NCFLAG3_SHIFT)

/* -- SCT_CONFLAG_NCFLAG4: Bit n is one if a no-change conflict event occurred on output n since  reset or a 1 was last written to this bit (output 0 = bit 0, output 1 = bit 1,..., output 15 = bit 15). */
#define SCT_CONFLAG_NCFLAG4_SHIFT (4)
#define SCT_CONFLAG_NCFLAG4_MASK (0x01 << SCT_CONFLAG_NCFLAG4_SHIFT)
#define SCT_CONFLAG_NCFLAG4(x) ((x) << SCT_CONFLAG_NCFLAG4_SHIFT)

/* -- SCT_CONFLAG_NCFLAG5: Bit n is one if a no-change conflict event occurred on output n since  reset or a 1 was last written to this bit (output 0 = bit 0, output 1 = bit 1,..., output 15 = bit 15). */
#define SCT_CONFLAG_NCFLAG5_SHIFT (5)
#define SCT_CONFLAG_NCFLAG5_MASK (0x01 << SCT_CONFLAG_NCFLAG5_SHIFT)
#define SCT_CONFLAG_NCFLAG5(x) ((x) << SCT_CONFLAG_NCFLAG5_SHIFT)

/* -- SCT_CONFLAG_NCFLAG6: Bit n is one if a no-change conflict event occurred on output n since  reset or a 1 was last written to this bit (output 0 = bit 0, output 1 = bit 1,..., output 15 = bit 15). */
#define SCT_CONFLAG_NCFLAG6_SHIFT (6)
#define SCT_CONFLAG_NCFLAG6_MASK (0x01 << SCT_CONFLAG_NCFLAG6_SHIFT)
#define SCT_CONFLAG_NCFLAG6(x) ((x) << SCT_CONFLAG_NCFLAG6_SHIFT)

/* -- SCT_CONFLAG_NCFLAG7: Bit n is one if a no-change conflict event occurred on output n since  reset or a 1 was last written to this bit (output 0 = bit 0, output 1 = bit 1,..., output 15 = bit 15). */
#define SCT_CONFLAG_NCFLAG7_SHIFT (7)
#define SCT_CONFLAG_NCFLAG7_MASK (0x01 << SCT_CONFLAG_NCFLAG7_SHIFT)
#define SCT_CONFLAG_NCFLAG7(x) ((x) << SCT_CONFLAG_NCFLAG7_SHIFT)

/* -- SCT_CONFLAG_NCFLAG8: Bit n is one if a no-change conflict event occurred on output n since  reset or a 1 was last written to this bit (output 0 = bit 0, output 1 = bit 1,..., output 15 = bit 15). */
#define SCT_CONFLAG_NCFLAG8_SHIFT (8)
#define SCT_CONFLAG_NCFLAG8_MASK (0x01 << SCT_CONFLAG_NCFLAG8_SHIFT)
#define SCT_CONFLAG_NCFLAG8(x) ((x) << SCT_CONFLAG_NCFLAG8_SHIFT)

/* -- SCT_CONFLAG_NCFLAG9: Bit n is one if a no-change conflict event occurred on output n since  reset or a 1 was last written to this bit (output 0 = bit 0, output 1 = bit 1,..., output 15 = bit 15). */
#define SCT_CONFLAG_NCFLAG9_SHIFT (9)
#define SCT_CONFLAG_NCFLAG9_MASK (0x01 << SCT_CONFLAG_NCFLAG9_SHIFT)
#define SCT_CONFLAG_NCFLAG9(x) ((x) << SCT_CONFLAG_NCFLAG9_SHIFT)

/* -- SCT_CONFLAG_NCFLAG10: Bit n is one if a no-change conflict event occurred on output n since  reset or a 1 was last written to this bit (output 0 = bit 0, output 1 = bit 1,..., output 15 = bit 15). */
#define SCT_CONFLAG_NCFLAG10_SHIFT (10)
#define SCT_CONFLAG_NCFLAG10_MASK (0x01 << SCT_CONFLAG_NCFLAG10_SHIFT)
#define SCT_CONFLAG_NCFLAG10(x) ((x) << SCT_CONFLAG_NCFLAG10_SHIFT)

/* -- SCT_CONFLAG_NCFLAG11: Bit n is one if a no-change conflict event occurred on output n since  reset or a 1 was last written to this bit (output 0 = bit 0, output 1 = bit 1,..., output 15 = bit 15). */
#define SCT_CONFLAG_NCFLAG11_SHIFT (11)
#define SCT_CONFLAG_NCFLAG11_MASK (0x01 << SCT_CONFLAG_NCFLAG11_SHIFT)
#define SCT_CONFLAG_NCFLAG11(x) ((x) << SCT_CONFLAG_NCFLAG11_SHIFT)

/* -- SCT_CONFLAG_NCFLAG12: Bit n is one if a no-change conflict event occurred on output n since  reset or a 1 was last written to this bit (output 0 = bit 0, output 1 = bit 1,..., output 15 = bit 15). */
#define SCT_CONFLAG_NCFLAG12_SHIFT (12)
#define SCT_CONFLAG_NCFLAG12_MASK (0x01 << SCT_CONFLAG_NCFLAG12_SHIFT)
#define SCT_CONFLAG_NCFLAG12(x) ((x) << SCT_CONFLAG_NCFLAG12_SHIFT)

/* -- SCT_CONFLAG_NCFLAG13: Bit n is one if a no-change conflict event occurred on output n since  reset or a 1 was last written to this bit (output 0 = bit 0, output 1 = bit 1,..., output 15 = bit 15). */
#define SCT_CONFLAG_NCFLAG13_SHIFT (13)
#define SCT_CONFLAG_NCFLAG13_MASK (0x01 << SCT_CONFLAG_NCFLAG13_SHIFT)
#define SCT_CONFLAG_NCFLAG13(x) ((x) << SCT_CONFLAG_NCFLAG13_SHIFT)

/* -- SCT_CONFLAG_NCFLAG14: Bit n is one if a no-change conflict event occurred on output n since  reset or a 1 was last written to this bit (output 0 = bit 0, output 1 = bit 1,..., output 15 = bit 15). */
#define SCT_CONFLAG_NCFLAG14_SHIFT (14)
#define SCT_CONFLAG_NCFLAG14_MASK (0x01 << SCT_CONFLAG_NCFLAG14_SHIFT)
#define SCT_CONFLAG_NCFLAG14(x) ((x) << SCT_CONFLAG_NCFLAG14_SHIFT)

/* -- SCT_CONFLAG_NCFLAG15: Bit n is one if a no-change conflict event occurred on output n since  reset or a 1 was last written to this bit (output 0 = bit 0, output 1 = bit 1,..., output 15 = bit 15). */
#define SCT_CONFLAG_NCFLAG15_SHIFT (15)
#define SCT_CONFLAG_NCFLAG15_MASK (0x01 << SCT_CONFLAG_NCFLAG15_SHIFT)
#define SCT_CONFLAG_NCFLAG15(x) ((x) << SCT_CONFLAG_NCFLAG15_SHIFT)

/* -- SCT_CONFLAG_BUSERRL: The most recent bus error from this SCT involved writing CTR  L/Unified, STATE L/Unified, MATCH L/Unified, or the Output register when the  L/U counter was not halted. A word write to certain L  and H registers can be half successful and half unsuccessful. */
#define SCT_CONFLAG_BUSERRL_SHIFT (30)
#define SCT_CONFLAG_BUSERRL_MASK (0x01 << SCT_CONFLAG_BUSERRL_SHIFT)
#define SCT_CONFLAG_BUSERRL(x) ((x) << SCT_CONFLAG_BUSERRL_SHIFT)

/* -- SCT_CONFLAG_BUSERRH: The most recent bus error from this SCT involved writing CTR H,  STATE H, MATCH H, or the Output register when the H  counter was not halted. */
#define SCT_CONFLAG_BUSERRH_SHIFT (31)
#define SCT_CONFLAG_BUSERRH_MASK (0x01 << SCT_CONFLAG_BUSERRH_SHIFT)
#define SCT_CONFLAG_BUSERRH(x) ((x) << SCT_CONFLAG_BUSERRH_SHIFT)

/* --- SCT_MATCHn ---------------------------------------- */
#define SCT_MATCHn(n) MMIO32(SCT_BASE + 0x100 + (n*4))

/* -- SCT_MATCHn_MATCH_L: When UNIFY = 0, read or write the 16-bit value to be  compared to the L counter. When UNIFY = 1, read or write the lower 16 bits of the 32-bit value to be compared to the unified counter. */
#define SCT_MATCHn_MATCH_L_SHIFT (0)
#define SCT_MATCHn_MATCH_L_MASK (0xFFFF << SCT_MATCHn_MATCH_L_SHIFT)
#define SCT_MATCHn_MATCH_L(x) ((x) << SCT_MATCHn_MATCH_L_SHIFT)

/* -- SCT_MATCHn_MATCH_H: When UNIFY = 0, read or write the 16-bit value to be  compared to the H counter. When UNIFY = 1, read or write the upper 16 bits of the 32-bit value to be compared to the unified counter. */
#define SCT_MATCHn_MATCH_H_SHIFT (16)
#define SCT_MATCHn_MATCH_H_MASK (0xFFFF << SCT_MATCHn_MATCH_H_SHIFT)
#define SCT_MATCHn_MATCH_H(x) ((x) << SCT_MATCHn_MATCH_H_SHIFT)

/* --- SCT_FRACMAT0 -------------------------------------- */
#define SCT_FRACMAT0 MMIO32(SCT_BASE + 0x140)

/* --- SCT_FRACMAT1 -------------------------------------- */
#define SCT_FRACMAT1 MMIO32(SCT_BASE + 0x144)

/* --- SCT_FRACMAT2 -------------------------------------- */
#define SCT_FRACMAT2 MMIO32(SCT_BASE + 0x148)

/* --- SCT_FRACMAT3 -------------------------------------- */
#define SCT_FRACMAT3 MMIO32(SCT_BASE + 0x14C)

/* --- SCT_FRACMAT4 -------------------------------------- */
#define SCT_FRACMAT4 MMIO32(SCT_BASE + 0x150)

/* --- SCT_FRACMAT5 -------------------------------------- */
#define SCT_FRACMAT5 MMIO32(SCT_BASE + 0x154)

/* -- SCT_FRACMATn_FRACMAT_L: When UNIFY = 0, read or write the 4-bit value specifying the dither pattern to be applied to the corresponding MATCHn_L register (n = 0 to 5). When UNIFY = 1, the value applies to the unified, 32-bit fractional match register. */
#define SCT_FRACMATn_FRACMAT_L_SHIFT (0)
#define SCT_FRACMATn_FRACMAT_L_MASK (0x0F << SCT_FRACMATn_FRACMAT_L_SHIFT)
#define SCT_FRACMATn_FRACMAT_L(x) ((x) << SCT_FRACMATn_FRACMAT_L_SHIFT)

/* -- SCT_FRACMATn_FRACMAT_H: When UNIFY = 0, read or write 4-bit value specifying the dither pattern to be applied to the corresponding MATCHn_H register (n = 0 to 5). */
#define SCT_FRACMATn_FRACMAT_H_SHIFT (16)
#define SCT_FRACMATn_FRACMAT_H_MASK (0x0F << SCT_FRACMATn_FRACMAT_H_SHIFT)
#define SCT_FRACMATn_FRACMAT_H(x) ((x) << SCT_FRACMATn_FRACMAT_H_SHIFT)

/* --- SCT_CAP0 ------------------------------------------ */
#define SCT_CAP0 MMIO32(SCT_BASE + 0x100)

/* --- SCT_CAP1 ------------------------------------------ */
#define SCT_CAP1 MMIO32(SCT_BASE + 0x104)

/* --- SCT_CAP2 ------------------------------------------ */
#define SCT_CAP2 MMIO32(SCT_BASE + 0x108)

/* --- SCT_CAP3 ------------------------------------------ */
#define SCT_CAP3 MMIO32(SCT_BASE + 0x10C)

/* --- SCT_CAP4 ------------------------------------------ */
#define SCT_CAP4 MMIO32(SCT_BASE + 0x110)

/* --- SCT_CAP5 ------------------------------------------ */
#define SCT_CAP5 MMIO32(SCT_BASE + 0x114)

/* --- SCT_CAP6 ------------------------------------------ */
#define SCT_CAP6 MMIO32(SCT_BASE + 0x118)

/* --- SCT_CAP7 ------------------------------------------ */
#define SCT_CAP7 MMIO32(SCT_BASE + 0x11C)

/* --- SCT_CAP8 ------------------------------------------ */
#define SCT_CAP8 MMIO32(SCT_BASE + 0x120)

/* --- SCT_CAP9 ------------------------------------------ */
#define SCT_CAP9 MMIO32(SCT_BASE + 0x124)

/* --- SCT_CAP10 ----------------------------------------- */
#define SCT_CAP10 MMIO32(SCT_BASE + 0x128)

/* --- SCT_CAP11 ----------------------------------------- */
#define SCT_CAP11 MMIO32(SCT_BASE + 0x12C)

/* --- SCT_CAP12 ----------------------------------------- */
#define SCT_CAP12 MMIO32(SCT_BASE + 0x130)

/* --- SCT_CAP13 ----------------------------------------- */
#define SCT_CAP13 MMIO32(SCT_BASE + 0x134)

/* --- SCT_CAP14 ----------------------------------------- */
#define SCT_CAP14 MMIO32(SCT_BASE + 0x138)

/* --- SCT_CAP15 ----------------------------------------- */
#define SCT_CAP15 MMIO32(SCT_BASE + 0x13C)

/* -- SCT_CAPn_CAP_L: When UNIFY = 0, read the 16-bit counter value at which this register was last captured. When UNIFY = 1, read the lower 16 bits of the 32-bit value at which this register was last captured. */
#define SCT_CAPn_CAP_L_SHIFT (0)
#define SCT_CAPn_CAP_L_MASK (0xFFFF << SCT_CAPn_CAP_L_SHIFT)
#define SCT_CAPn_CAP_L(x) ((x) << SCT_CAPn_CAP_L_SHIFT)

/* -- SCT_CAPn_CAP_H: When UNIFY = 0, read the 16-bit counter value at which this register was last captured. When UNIFY = 1, read the upper 16 bits of the 32-bit value at which this register was last captured. */
#define SCT_CAPn_CAP_H_SHIFT (16)
#define SCT_CAPn_CAP_H_MASK (0xFFFF << SCT_CAPn_CAP_H_SHIFT)
#define SCT_CAPn_CAP_H(x) ((x) << SCT_CAPn_CAP_H_SHIFT)

/* --- SCT_MATCHRELn ------------------------------------- */
#define SCT_MATCHRELn(n) MMIO32(SCT_BASE + 0x200 + (n*4))

/* -- SCT_MATCHRELn_RELOAD_L: When UNIFY = 0, read or write the 16-bit value to be loaded into the MATCHn_L register. When UNIFY = 1, read or write the lower 16 bits of the 32-bit value to be loaded into the MATCHn register. */
#define SCT_MATCHRELn_RELOAD_L_SHIFT (0)
#define SCT_MATCHRELn_RELOAD_L_MASK (0xFFFF << SCT_MATCHRELn_RELOAD_L_SHIFT)
#define SCT_MATCHRELn_RELOAD_L(x) ((x) << SCT_MATCHRELn_RELOAD_L_SHIFT)

/* -- SCT_MATCHRELn_RELOAD_H: When UNIFY = 0, read or write the 16-bit to be loaded into the MATCHn_H register. When UNIFY = 1, read or write the upper 16 bits of the 32-bit value to be loaded into the MATCHn register. */
#define SCT_MATCHRELn_RELOAD_H_SHIFT (16)
#define SCT_MATCHRELn_RELOAD_H_MASK (0xFFFF << SCT_MATCHRELn_RELOAD_H_SHIFT)
#define SCT_MATCHRELn_RELOAD_H(x) ((x) << SCT_MATCHRELn_RELOAD_H_SHIFT)

/* --- SCT_FRACMATREL0 ----------------------------------- */
#define SCT_FRACMATREL0 MMIO32(SCT_BASE + 0x240)

/* --- SCT_FRACMATREL1 ----------------------------------- */
#define SCT_FRACMATREL1 MMIO32(SCT_BASE + 0x244)

/* --- SCT_FRACMATREL2 ----------------------------------- */
#define SCT_FRACMATREL2 MMIO32(SCT_BASE + 0x248)

/* --- SCT_FRACMATREL3 ----------------------------------- */
#define SCT_FRACMATREL3 MMIO32(SCT_BASE + 0x24C)

/* --- SCT_FRACMATREL4 ----------------------------------- */
#define SCT_FRACMATREL4 MMIO32(SCT_BASE + 0x250)

/* --- SCT_FRACMATREL5 ----------------------------------- */
#define SCT_FRACMATREL5 MMIO32(SCT_BASE + 0x254)

/* -- SCT_FRACMATRELn_RELFRAC_L: When UNIFY = 0, read or write the 4-bit value to be loaded into the FRACMATn_L register. When UNIFY = 1, read or write the lower 4 bits to be loaded into the FRACMATn register. */
#define SCT_FRACMATRELn_RELFRAC_L_SHIFT (0)
#define SCT_FRACMATRELn_RELFRAC_L_MASK (0x0F << SCT_FRACMATRELn_RELFRAC_L_SHIFT)
#define SCT_FRACMATRELn_RELFRAC_L(x) ((x) << SCT_FRACMATRELn_RELFRAC_L_SHIFT)

/* -- SCT_FRACMATRELn_RELFRAC_H: When UNIFY = 0, read or write the 4-bit value to be loaded into the FRACMATn_H register. When UNIFY = 1, read or write the upper 4 bits with the 4-bit value to be loaded into the FRACMATn register. */
#define SCT_FRACMATRELn_RELFRAC_H_SHIFT (16)
#define SCT_FRACMATRELn_RELFRAC_H_MASK (0x0F << SCT_FRACMATRELn_RELFRAC_H_SHIFT)
#define SCT_FRACMATRELn_RELFRAC_H(x) ((x) << SCT_FRACMATRELn_RELFRAC_H_SHIFT)

/* --- SCT_CAPCTRL0 -------------------------------------- */
#define SCT_CAPCTRL0 MMIO32(SCT_BASE + 0x200)

/* --- SCT_CAPCTRL1 -------------------------------------- */
#define SCT_CAPCTRL1 MMIO32(SCT_BASE + 0x204)

/* --- SCT_CAPCTRL2 -------------------------------------- */
#define SCT_CAPCTRL2 MMIO32(SCT_BASE + 0x208)

/* --- SCT_CAPCTRL3 -------------------------------------- */
#define SCT_CAPCTRL3 MMIO32(SCT_BASE + 0x20C)

/* --- SCT_CAPCTRL4 -------------------------------------- */
#define SCT_CAPCTRL4 MMIO32(SCT_BASE + 0x210)

/* --- SCT_CAPCTRL5 -------------------------------------- */
#define SCT_CAPCTRL5 MMIO32(SCT_BASE + 0x214)

/* --- SCT_CAPCTRL6 -------------------------------------- */
#define SCT_CAPCTRL6 MMIO32(SCT_BASE + 0x218)

/* --- SCT_CAPCTRL7 -------------------------------------- */
#define SCT_CAPCTRL7 MMIO32(SCT_BASE + 0x21C)

/* --- SCT_CAPCTRL8 -------------------------------------- */
#define SCT_CAPCTRL8 MMIO32(SCT_BASE + 0x220)

/* --- SCT_CAPCTRL9 -------------------------------------- */
#define SCT_CAPCTRL9 MMIO32(SCT_BASE + 0x224)

/* --- SCT_CAPCTRL10 ------------------------------------- */
#define SCT_CAPCTRL10 MMIO32(SCT_BASE + 0x228)

/* --- SCT_CAPCTRL11 ------------------------------------- */
#define SCT_CAPCTRL11 MMIO32(SCT_BASE + 0x22C)

/* --- SCT_CAPCTRL12 ------------------------------------- */
#define SCT_CAPCTRL12 MMIO32(SCT_BASE + 0x230)

/* --- SCT_CAPCTRL13 ------------------------------------- */
#define SCT_CAPCTRL13 MMIO32(SCT_BASE + 0x234)

/* --- SCT_CAPCTRL14 ------------------------------------- */
#define SCT_CAPCTRL14 MMIO32(SCT_BASE + 0x238)

/* --- SCT_CAPCTRL15 ------------------------------------- */
#define SCT_CAPCTRL15 MMIO32(SCT_BASE + 0x23C)

/* -- SCT_CAPCTRLn_CAPCON_L: If bit m is one, event m causes the CAPn_L (UNIFY = 0) or the CAPn (UNIFY = 1) register to be loaded (event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15). */
#define SCT_CAPCTRLn_CAPCON_L_SHIFT (0)
#define SCT_CAPCTRLn_CAPCON_L_MASK (0xFFFF << SCT_CAPCTRLn_CAPCON_L_SHIFT)
#define SCT_CAPCTRLn_CAPCON_L(x) ((x) << SCT_CAPCTRLn_CAPCON_L_SHIFT)

/* -- SCT_CAPCTRLn_CAPCON_H: If bit m is one, event m causes the CAPn_H (UNIFY = 0) register to be loaded (event 0 = bit 16, event 1 = bit 17,..., event 15 = bit 31). */
#define SCT_CAPCTRLn_CAPCON_H_SHIFT (16)
#define SCT_CAPCTRLn_CAPCON_H_MASK (0xFFFF << SCT_CAPCTRLn_CAPCON_H_SHIFT)
#define SCT_CAPCTRLn_CAPCON_H(x) ((x) << SCT_CAPCTRLn_CAPCON_H_SHIFT)

/* --- SCT_EVn_STATE ------------------------------------- */
#define SCT_EVn_STATE(n) MMIO32(SCT_BASE + 0x300 + (n*8))

/* -- SCT_EVn_STATE_STATEMSK0: If bit m is one, event n (n= 0 to 15) happens in state m of the counter selected by the HEVENT bit (m = state number; state 0 = bit 0, state 1= bit 1,..., state 31 = bit 31). */
#define SCT_EVn_STATE_STATEMSK0_SHIFT (0)
#define SCT_EVn_STATE_STATEMSK0_MASK (0x01 << SCT_EVn_STATE_STATEMSK0_SHIFT)
#define SCT_EVn_STATE_STATEMSK0(x) ((x) << SCT_EVn_STATE_STATEMSK0_SHIFT)

/* -- SCT_EVn_STATE_STATEMSK1: If bit m is one, event n (n= 0 to 15) happens in state m of the counter selected by the HEVENT bit (m = state number; state 0 = bit 0, state 1= bit 1,..., state 31 = bit 31). */
#define SCT_EVn_STATE_STATEMSK1_SHIFT (1)
#define SCT_EVn_STATE_STATEMSK1_MASK (0x01 << SCT_EVn_STATE_STATEMSK1_SHIFT)
#define SCT_EVn_STATE_STATEMSK1(x) ((x) << SCT_EVn_STATE_STATEMSK1_SHIFT)

/* -- SCT_EVn_STATE_STATEMSK2: If bit m is one, event n (n= 0 to 15) happens in state m of the counter selected by the HEVENT bit (m = state number; state 0 = bit 0, state 1= bit 1,..., state 31 = bit 31). */
#define SCT_EVn_STATE_STATEMSK2_SHIFT (2)
#define SCT_EVn_STATE_STATEMSK2_MASK (0x01 << SCT_EVn_STATE_STATEMSK2_SHIFT)
#define SCT_EVn_STATE_STATEMSK2(x) ((x) << SCT_EVn_STATE_STATEMSK2_SHIFT)

/* -- SCT_EVn_STATE_STATEMSK3: If bit m is one, event n (n= 0 to 15) happens in state m of the counter selected by the HEVENT bit (m = state number; state 0 = bit 0, state 1= bit 1,..., state 31 = bit 31). */
#define SCT_EVn_STATE_STATEMSK3_SHIFT (3)
#define SCT_EVn_STATE_STATEMSK3_MASK (0x01 << SCT_EVn_STATE_STATEMSK3_SHIFT)
#define SCT_EVn_STATE_STATEMSK3(x) ((x) << SCT_EVn_STATE_STATEMSK3_SHIFT)

/* -- SCT_EVn_STATE_STATEMSK4: If bit m is one, event n (n= 0 to 15) happens in state m of the counter selected by the HEVENT bit (m = state number; state 0 = bit 0, state 1= bit 1,..., state 31 = bit 31). */
#define SCT_EVn_STATE_STATEMSK4_SHIFT (4)
#define SCT_EVn_STATE_STATEMSK4_MASK (0x01 << SCT_EVn_STATE_STATEMSK4_SHIFT)
#define SCT_EVn_STATE_STATEMSK4(x) ((x) << SCT_EVn_STATE_STATEMSK4_SHIFT)

/* -- SCT_EVn_STATE_STATEMSK5: If bit m is one, event n (n= 0 to 15) happens in state m of the counter selected by the HEVENT bit (m = state number; state 0 = bit 0, state 1= bit 1,..., state 31 = bit 31). */
#define SCT_EVn_STATE_STATEMSK5_SHIFT (5)
#define SCT_EVn_STATE_STATEMSK5_MASK (0x01 << SCT_EVn_STATE_STATEMSK5_SHIFT)
#define SCT_EVn_STATE_STATEMSK5(x) ((x) << SCT_EVn_STATE_STATEMSK5_SHIFT)

/* -- SCT_EVn_STATE_STATEMSK6: If bit m is one, event n (n= 0 to 15) happens in state m of the counter selected by the HEVENT bit (m = state number; state 0 = bit 0, state 1= bit 1,..., state 31 = bit 31). */
#define SCT_EVn_STATE_STATEMSK6_SHIFT (6)
#define SCT_EVn_STATE_STATEMSK6_MASK (0x01 << SCT_EVn_STATE_STATEMSK6_SHIFT)
#define SCT_EVn_STATE_STATEMSK6(x) ((x) << SCT_EVn_STATE_STATEMSK6_SHIFT)

/* -- SCT_EVn_STATE_STATEMSK7: If bit m is one, event n (n= 0 to 15) happens in state m of the counter selected by the HEVENT bit (m = state number; state 0 = bit 0, state 1= bit 1,..., state 31 = bit 31). */
#define SCT_EVn_STATE_STATEMSK7_SHIFT (7)
#define SCT_EVn_STATE_STATEMSK7_MASK (0x01 << SCT_EVn_STATE_STATEMSK7_SHIFT)
#define SCT_EVn_STATE_STATEMSK7(x) ((x) << SCT_EVn_STATE_STATEMSK7_SHIFT)

/* -- SCT_EVn_STATE_STATEMSK8: If bit m is one, event n (n= 0 to 15) happens in state m of the counter selected by the HEVENT bit (m = state number; state 0 = bit 0, state 1= bit 1,..., state 31 = bit 31). */
#define SCT_EVn_STATE_STATEMSK8_SHIFT (8)
#define SCT_EVn_STATE_STATEMSK8_MASK (0x01 << SCT_EVn_STATE_STATEMSK8_SHIFT)
#define SCT_EVn_STATE_STATEMSK8(x) ((x) << SCT_EVn_STATE_STATEMSK8_SHIFT)

/* -- SCT_EVn_STATE_STATEMSK9: If bit m is one, event n (n= 0 to 15) happens in state m of the counter selected by the HEVENT bit (m = state number; state 0 = bit 0, state 1= bit 1,..., state 31 = bit 31). */
#define SCT_EVn_STATE_STATEMSK9_SHIFT (9)
#define SCT_EVn_STATE_STATEMSK9_MASK (0x01 << SCT_EVn_STATE_STATEMSK9_SHIFT)
#define SCT_EVn_STATE_STATEMSK9(x) ((x) << SCT_EVn_STATE_STATEMSK9_SHIFT)

/* -- SCT_EVn_STATE_STATEMSK10: If bit m is one, event n (n= 0 to 15) happens in state m of the counter selected by the HEVENT bit (m = state number; state 0 = bit 0, state 1= bit 1,..., state 31 = bit 31). */
#define SCT_EVn_STATE_STATEMSK10_SHIFT (10)
#define SCT_EVn_STATE_STATEMSK10_MASK (0x01 << SCT_EVn_STATE_STATEMSK10_SHIFT)
#define SCT_EVn_STATE_STATEMSK10(x) ((x) << SCT_EVn_STATE_STATEMSK10_SHIFT)

/* -- SCT_EVn_STATE_STATEMSK11: If bit m is one, event n (n= 0 to 15) happens in state m of the counter selected by the HEVENT bit (m = state number; state 0 = bit 0, state 1= bit 1,..., state 31 = bit 31). */
#define SCT_EVn_STATE_STATEMSK11_SHIFT (11)
#define SCT_EVn_STATE_STATEMSK11_MASK (0x01 << SCT_EVn_STATE_STATEMSK11_SHIFT)
#define SCT_EVn_STATE_STATEMSK11(x) ((x) << SCT_EVn_STATE_STATEMSK11_SHIFT)

/* -- SCT_EVn_STATE_STATEMSK12: If bit m is one, event n (n= 0 to 15) happens in state m of the counter selected by the HEVENT bit (m = state number; state 0 = bit 0, state 1= bit 1,..., state 31 = bit 31). */
#define SCT_EVn_STATE_STATEMSK12_SHIFT (12)
#define SCT_EVn_STATE_STATEMSK12_MASK (0x01 << SCT_EVn_STATE_STATEMSK12_SHIFT)
#define SCT_EVn_STATE_STATEMSK12(x) ((x) << SCT_EVn_STATE_STATEMSK12_SHIFT)

/* -- SCT_EVn_STATE_STATEMSK13: If bit m is one, event n (n= 0 to 15) happens in state m of the counter selected by the HEVENT bit (m = state number; state 0 = bit 0, state 1= bit 1,..., state 31 = bit 31). */
#define SCT_EVn_STATE_STATEMSK13_SHIFT (13)
#define SCT_EVn_STATE_STATEMSK13_MASK (0x01 << SCT_EVn_STATE_STATEMSK13_SHIFT)
#define SCT_EVn_STATE_STATEMSK13(x) ((x) << SCT_EVn_STATE_STATEMSK13_SHIFT)

/* -- SCT_EVn_STATE_STATEMSK14: If bit m is one, event n (n= 0 to 15) happens in state m of the counter selected by the HEVENT bit (m = state number; state 0 = bit 0, state 1= bit 1,..., state 31 = bit 31). */
#define SCT_EVn_STATE_STATEMSK14_SHIFT (14)
#define SCT_EVn_STATE_STATEMSK14_MASK (0x01 << SCT_EVn_STATE_STATEMSK14_SHIFT)
#define SCT_EVn_STATE_STATEMSK14(x) ((x) << SCT_EVn_STATE_STATEMSK14_SHIFT)

/* -- SCT_EVn_STATE_STATEMSK15: If bit m is one, event n (n= 0 to 15) happens in state m of the counter selected by the HEVENT bit (m = state number; state 0 = bit 0, state 1= bit 1,..., state 31 = bit 31). */
#define SCT_EVn_STATE_STATEMSK15_SHIFT (15)
#define SCT_EVn_STATE_STATEMSK15_MASK (0x01 << SCT_EVn_STATE_STATEMSK15_SHIFT)
#define SCT_EVn_STATE_STATEMSK15(x) ((x) << SCT_EVn_STATE_STATEMSK15_SHIFT)

/* -- SCT_EVn_STATE_STATEMSK16: If bit m is one, event n (n= 0 to 15) happens in state m of the counter selected by the HEVENT bit (m = state number; state 0 = bit 0, state 1= bit 1,..., state 31 = bit 31). */
#define SCT_EVn_STATE_STATEMSK16_SHIFT (16)
#define SCT_EVn_STATE_STATEMSK16_MASK (0x01 << SCT_EVn_STATE_STATEMSK16_SHIFT)
#define SCT_EVn_STATE_STATEMSK16(x) ((x) << SCT_EVn_STATE_STATEMSK16_SHIFT)

/* -- SCT_EVn_STATE_STATEMSK17: If bit m is one, event n (n= 0 to 15) happens in state m of the counter selected by the HEVENT bit (m = state number; state 0 = bit 0, state 1= bit 1,..., state 31 = bit 31). */
#define SCT_EVn_STATE_STATEMSK17_SHIFT (17)
#define SCT_EVn_STATE_STATEMSK17_MASK (0x01 << SCT_EVn_STATE_STATEMSK17_SHIFT)
#define SCT_EVn_STATE_STATEMSK17(x) ((x) << SCT_EVn_STATE_STATEMSK17_SHIFT)

/* -- SCT_EVn_STATE_STATEMSK18: If bit m is one, event n (n= 0 to 15) happens in state m of the counter selected by the HEVENT bit (m = state number; state 0 = bit 0, state 1= bit 1,..., state 31 = bit 31). */
#define SCT_EVn_STATE_STATEMSK18_SHIFT (18)
#define SCT_EVn_STATE_STATEMSK18_MASK (0x01 << SCT_EVn_STATE_STATEMSK18_SHIFT)
#define SCT_EVn_STATE_STATEMSK18(x) ((x) << SCT_EVn_STATE_STATEMSK18_SHIFT)

/* -- SCT_EVn_STATE_STATEMSK19: If bit m is one, event n (n= 0 to 15) happens in state m of the counter selected by the HEVENT bit (m = state number; state 0 = bit 0, state 1= bit 1,..., state 31 = bit 31). */
#define SCT_EVn_STATE_STATEMSK19_SHIFT (19)
#define SCT_EVn_STATE_STATEMSK19_MASK (0x01 << SCT_EVn_STATE_STATEMSK19_SHIFT)
#define SCT_EVn_STATE_STATEMSK19(x) ((x) << SCT_EVn_STATE_STATEMSK19_SHIFT)

/* -- SCT_EVn_STATE_STATEMSK20: If bit m is one, event n (n= 0 to 15) happens in state m of the counter selected by the HEVENT bit (m = state number; state 0 = bit 0, state 1= bit 1,..., state 31 = bit 31). */
#define SCT_EVn_STATE_STATEMSK20_SHIFT (20)
#define SCT_EVn_STATE_STATEMSK20_MASK (0x01 << SCT_EVn_STATE_STATEMSK20_SHIFT)
#define SCT_EVn_STATE_STATEMSK20(x) ((x) << SCT_EVn_STATE_STATEMSK20_SHIFT)

/* -- SCT_EVn_STATE_STATEMSK21: If bit m is one, event n (n= 0 to 15) happens in state m of the counter selected by the HEVENT bit (m = state number; state 0 = bit 0, state 1= bit 1,..., state 31 = bit 31). */
#define SCT_EVn_STATE_STATEMSK21_SHIFT (21)
#define SCT_EVn_STATE_STATEMSK21_MASK (0x01 << SCT_EVn_STATE_STATEMSK21_SHIFT)
#define SCT_EVn_STATE_STATEMSK21(x) ((x) << SCT_EVn_STATE_STATEMSK21_SHIFT)

/* -- SCT_EVn_STATE_STATEMSK22: If bit m is one, event n (n= 0 to 15) happens in state m of the counter selected by the HEVENT bit (m = state number; state 0 = bit 0, state 1= bit 1,..., state 31 = bit 31). */
#define SCT_EVn_STATE_STATEMSK22_SHIFT (22)
#define SCT_EVn_STATE_STATEMSK22_MASK (0x01 << SCT_EVn_STATE_STATEMSK22_SHIFT)
#define SCT_EVn_STATE_STATEMSK22(x) ((x) << SCT_EVn_STATE_STATEMSK22_SHIFT)

/* -- SCT_EVn_STATE_STATEMSK23: If bit m is one, event n (n= 0 to 15) happens in state m of the counter selected by the HEVENT bit (m = state number; state 0 = bit 0, state 1= bit 1,..., state 31 = bit 31). */
#define SCT_EVn_STATE_STATEMSK23_SHIFT (23)
#define SCT_EVn_STATE_STATEMSK23_MASK (0x01 << SCT_EVn_STATE_STATEMSK23_SHIFT)
#define SCT_EVn_STATE_STATEMSK23(x) ((x) << SCT_EVn_STATE_STATEMSK23_SHIFT)

/* -- SCT_EVn_STATE_STATEMSK24: If bit m is one, event n (n= 0 to 15) happens in state m of the counter selected by the HEVENT bit (m = state number; state 0 = bit 0, state 1= bit 1,..., state 31 = bit 31). */
#define SCT_EVn_STATE_STATEMSK24_SHIFT (24)
#define SCT_EVn_STATE_STATEMSK24_MASK (0x01 << SCT_EVn_STATE_STATEMSK24_SHIFT)
#define SCT_EVn_STATE_STATEMSK24(x) ((x) << SCT_EVn_STATE_STATEMSK24_SHIFT)

/* -- SCT_EVn_STATE_STATEMSK25: If bit m is one, event n (n= 0 to 15) happens in state m of the counter selected by the HEVENT bit (m = state number; state 0 = bit 0, state 1= bit 1,..., state 31 = bit 31). */
#define SCT_EVn_STATE_STATEMSK25_SHIFT (25)
#define SCT_EVn_STATE_STATEMSK25_MASK (0x01 << SCT_EVn_STATE_STATEMSK25_SHIFT)
#define SCT_EVn_STATE_STATEMSK25(x) ((x) << SCT_EVn_STATE_STATEMSK25_SHIFT)

/* -- SCT_EVn_STATE_STATEMSK26: If bit m is one, event n (n= 0 to 15) happens in state m of the counter selected by the HEVENT bit (m = state number; state 0 = bit 0, state 1= bit 1,..., state 31 = bit 31). */
#define SCT_EVn_STATE_STATEMSK26_SHIFT (26)
#define SCT_EVn_STATE_STATEMSK26_MASK (0x01 << SCT_EVn_STATE_STATEMSK26_SHIFT)
#define SCT_EVn_STATE_STATEMSK26(x) ((x) << SCT_EVn_STATE_STATEMSK26_SHIFT)

/* -- SCT_EVn_STATE_STATEMSK27: If bit m is one, event n (n= 0 to 15) happens in state m of the counter selected by the HEVENT bit (m = state number; state 0 = bit 0, state 1= bit 1,..., state 31 = bit 31). */
#define SCT_EVn_STATE_STATEMSK27_SHIFT (27)
#define SCT_EVn_STATE_STATEMSK27_MASK (0x01 << SCT_EVn_STATE_STATEMSK27_SHIFT)
#define SCT_EVn_STATE_STATEMSK27(x) ((x) << SCT_EVn_STATE_STATEMSK27_SHIFT)

/* -- SCT_EVn_STATE_STATEMSK28: If bit m is one, event n (n= 0 to 15) happens in state m of the counter selected by the HEVENT bit (m = state number; state 0 = bit 0, state 1= bit 1,..., state 31 = bit 31). */
#define SCT_EVn_STATE_STATEMSK28_SHIFT (28)
#define SCT_EVn_STATE_STATEMSK28_MASK (0x01 << SCT_EVn_STATE_STATEMSK28_SHIFT)
#define SCT_EVn_STATE_STATEMSK28(x) ((x) << SCT_EVn_STATE_STATEMSK28_SHIFT)

/* -- SCT_EVn_STATE_STATEMSK29: If bit m is one, event n (n= 0 to 15) happens in state m of the counter selected by the HEVENT bit (m = state number; state 0 = bit 0, state 1= bit 1,..., state 31 = bit 31). */
#define SCT_EVn_STATE_STATEMSK29_SHIFT (29)
#define SCT_EVn_STATE_STATEMSK29_MASK (0x01 << SCT_EVn_STATE_STATEMSK29_SHIFT)
#define SCT_EVn_STATE_STATEMSK29(x) ((x) << SCT_EVn_STATE_STATEMSK29_SHIFT)

/* -- SCT_EVn_STATE_STATEMSK30: If bit m is one, event n (n= 0 to 15) happens in state m of the counter selected by the HEVENT bit (m = state number; state 0 = bit 0, state 1= bit 1,..., state 31 = bit 31). */
#define SCT_EVn_STATE_STATEMSK30_SHIFT (30)
#define SCT_EVn_STATE_STATEMSK30_MASK (0x01 << SCT_EVn_STATE_STATEMSK30_SHIFT)
#define SCT_EVn_STATE_STATEMSK30(x) ((x) << SCT_EVn_STATE_STATEMSK30_SHIFT)

/* -- SCT_EVn_STATE_STATEMSK31: If bit m is one, event n (n= 0 to 15) happens in state m of the counter selected by the HEVENT bit (m = state number; state 0 = bit 0, state 1= bit 1,..., state 31 = bit 31). */
#define SCT_EVn_STATE_STATEMSK31_SHIFT (31)
#define SCT_EVn_STATE_STATEMSK31_MASK (0x01 << SCT_EVn_STATE_STATEMSK31_SHIFT)
#define SCT_EVn_STATE_STATEMSK31(x) ((x) << SCT_EVn_STATE_STATEMSK31_SHIFT)

/* --- SCT_EVn_CTRL -------------------------------------- */
#define SCT_EVn_CTRL(n) MMIO32(SCT_BASE + 0x304 + (n*8))

/* -- SCT_EVn_CTRL_MATCHSEL: Selects the Match register associated with this event (if any). A  match can occur only when the counter selected by the HEVENT  bit is running. */
#define SCT_EVn_CTRL_MATCHSEL_SHIFT (0)
#define SCT_EVn_CTRL_MATCHSEL_MASK (0x0F << SCT_EVn_CTRL_MATCHSEL_SHIFT)
#define SCT_EVn_CTRL_MATCHSEL(x) ((x) << SCT_EVn_CTRL_MATCHSEL_SHIFT)

/* -- SCT_EVn_CTRL_HEVENT: Select L/H counter. Do not set this bit if UNIFY = 1. */
#define SCT_EVn_CTRL_HEVENT_SHIFT (4)
#define SCT_EVn_CTRL_HEVENT_MASK (0x01 << SCT_EVn_CTRL_HEVENT_SHIFT)
#define SCT_EVn_CTRL_HEVENT(x) ((x) << SCT_EVn_CTRL_HEVENT_SHIFT)

/* SCT_EVn_CTRL_HEVENT_HEVENT values */
#define SCT_EVn_CTRL_HEVENT_L_STATE SCT_EVn_CTRL_HEVENT(0x00)  /* L state. Selects the L state and the L match register selected by MATCHSEL. */
#define SCT_EVn_CTRL_HEVENT_H_STATE SCT_EVn_CTRL_HEVENT(0x01)  /* H state. Selects the H state and the H match register selected by MATCHSEL. */

/* -- SCT_EVn_CTRL_OUTSEL: Input/output select */
#define SCT_EVn_CTRL_OUTSEL_SHIFT (5)
#define SCT_EVn_CTRL_OUTSEL_MASK (0x01 << SCT_EVn_CTRL_OUTSEL_SHIFT)
#define SCT_EVn_CTRL_OUTSEL(x) ((x) << SCT_EVn_CTRL_OUTSEL_SHIFT)

/* SCT_EVn_CTRL_OUTSEL_OUTSEL values */
#define SCT_EVn_CTRL_OUTSEL_INPUT SCT_EVn_CTRL_OUTSEL(0x00)  /* Input. Selects the input selected by IOSEL. */
#define SCT_EVn_CTRL_OUTSEL_OUTPUT SCT_EVn_CTRL_OUTSEL(0x01)  /* Output. Selects the output selected by IOSEL. */

/* -- SCT_EVn_CTRL_IOSEL: Selects the input or output signal associated with this event (if  any). Do not select an input in this register, if CKMODE is 1x. In this case the clock input is an implicit ingredient of  every event. */
#define SCT_EVn_CTRL_IOSEL_SHIFT (6)
#define SCT_EVn_CTRL_IOSEL_MASK (0x0F << SCT_EVn_CTRL_IOSEL_SHIFT)
#define SCT_EVn_CTRL_IOSEL(x) ((x) << SCT_EVn_CTRL_IOSEL_SHIFT)

/* -- SCT_EVn_CTRL_IOCOND: Selects the I/O condition for event n. (The detection of edges on outputs lags the conditions that switch the outputs by one SCT clock). In order to guarantee proper edge/state detection, an input must have a minimum pulse width of at least one SCT  clock period . */
#define SCT_EVn_CTRL_IOCOND_SHIFT (10)
#define SCT_EVn_CTRL_IOCOND_MASK (0x03 << SCT_EVn_CTRL_IOCOND_SHIFT)
#define SCT_EVn_CTRL_IOCOND(x) ((x) << SCT_EVn_CTRL_IOCOND_SHIFT)

/* SCT_EVn_CTRL_IOCOND_IOCOND values */
#define SCT_EVn_CTRL_IOCOND_LOW SCT_EVn_CTRL_IOCOND(0x00)  /* LOW */
#define SCT_EVn_CTRL_IOCOND_RISE SCT_EVn_CTRL_IOCOND(0x01)  /* Rise */
#define SCT_EVn_CTRL_IOCOND_FALL SCT_EVn_CTRL_IOCOND(0x02)  /* Fall */
#define SCT_EVn_CTRL_IOCOND_HIGH SCT_EVn_CTRL_IOCOND(0x03)  /* HIGH */

/* -- SCT_EVn_CTRL_COMBMODE: Selects how the specified match and I/O condition are used and combined. */
#define SCT_EVn_CTRL_COMBMODE_SHIFT (12)
#define SCT_EVn_CTRL_COMBMODE_MASK (0x03 << SCT_EVn_CTRL_COMBMODE_SHIFT)
#define SCT_EVn_CTRL_COMBMODE(x) ((x) << SCT_EVn_CTRL_COMBMODE_SHIFT)

/* SCT_EVn_CTRL_COMBMODE_COMBMODE values */
#define SCT_EVn_CTRL_COMBMODE_OR SCT_EVn_CTRL_COMBMODE(0x00)  /* OR. The event occurs when either the specified match or I/O condition occurs. */
#define SCT_EVn_CTRL_COMBMODE_MATCH SCT_EVn_CTRL_COMBMODE(0x01)  /* MATCH. Uses the specified match only. */
#define SCT_EVn_CTRL_COMBMODE_IO SCT_EVn_CTRL_COMBMODE(0x02)  /* IO. Uses the specified I/O condition only. */
#define SCT_EVn_CTRL_COMBMODE_AND SCT_EVn_CTRL_COMBMODE(0x03)  /* AND. The event occurs when the specified match and I/O condition occur simultaneously. */

/* -- SCT_EVn_CTRL_STATELD: This bit controls how the STATEV value modifies the state  selected by HEVENT when this event is the highest-numbered  event occurring for that state. */
#define SCT_EVn_CTRL_STATELD_SHIFT (14)
#define SCT_EVn_CTRL_STATELD_MASK (0x01 << SCT_EVn_CTRL_STATELD_SHIFT)
#define SCT_EVn_CTRL_STATELD(x) ((x) << SCT_EVn_CTRL_STATELD_SHIFT)

/* SCT_EVn_CTRL_STATELD_STATELD values */
#define SCT_EVn_CTRL_STATELD_STATEV_VALUE_IS_ADDE SCT_EVn_CTRL_STATELD(0x00)  /* STATEV value is added into STATE (the carry-out is ignored). */
#define SCT_EVn_CTRL_STATELD_STATEV_VALUE_IS_LOAD SCT_EVn_CTRL_STATELD(0x01)  /* STATEV value is loaded into STATE. */

/* -- SCT_EVn_CTRL_STATEV: This value is loaded into or added to the state selected by  HEVENT, depending on STATELD, when this event is the  highest-numbered event occurring for that state. If STATELD and  STATEV are both zero, there is no change to the STATE value. */
#define SCT_EVn_CTRL_STATEV_SHIFT (15)
#define SCT_EVn_CTRL_STATEV_MASK (0x1F << SCT_EVn_CTRL_STATEV_SHIFT)
#define SCT_EVn_CTRL_STATEV(x) ((x) << SCT_EVn_CTRL_STATEV_SHIFT)

/* -- SCT_EVn_CTRL_MATCHMEM: If this bit is one and the COMBMODE field specifies a match  component to the triggering of this event, then a match is considered to be active whenever the counter value is  GREATER THAN OR EQUAL TO the value specified in the  match register when counting up, LESS THEN OR EQUAL TO  the match value when counting down. If this bit is zero, a match is only be active during the cycle  when the counter is equal to the match value. */
#define SCT_EVn_CTRL_MATCHMEM_SHIFT (20)
#define SCT_EVn_CTRL_MATCHMEM_MASK (0x01 << SCT_EVn_CTRL_MATCHMEM_SHIFT)
#define SCT_EVn_CTRL_MATCHMEM(x) ((x) << SCT_EVn_CTRL_MATCHMEM_SHIFT)

/* -- SCT_EVn_CTRL_DIRECTION: Direction qualifier for event generation. This field only applies when the counters are operating in BIDIR  mode. If BIDIR = 0, the SCT ignores this field. Value 0x3 is reserved. */
#define SCT_EVn_CTRL_DIRECTION_SHIFT (21)
#define SCT_EVn_CTRL_DIRECTION_MASK (0x03 << SCT_EVn_CTRL_DIRECTION_SHIFT)
#define SCT_EVn_CTRL_DIRECTION(x) ((x) << SCT_EVn_CTRL_DIRECTION_SHIFT)

/* SCT_EVn_CTRL_DIRECTION_DIRECTION values */
#define SCT_EVn_CTRL_DIRECTION_DIRECTION_INDEPENDEN SCT_EVn_CTRL_DIRECTION(0x00)  /* Direction independent. This event is triggered regardless of the count direction. */
#define SCT_EVn_CTRL_DIRECTION_COUNTING_UP SCT_EVn_CTRL_DIRECTION(0x01)  /* Counting up. This event is triggered only during up-counting when BIDIR = 1. */
#define SCT_EVn_CTRL_DIRECTION_COUNTING_DOWN SCT_EVn_CTRL_DIRECTION(0x02)  /* Counting down. This event is triggered only during down-counting when BIDIR = 1. */

/* --- SCT_OUT0_SET -------------------------------------- */
#define SCT_OUT0_SET MMIO32(SCT_BASE + 0x500)

/* --- SCT_OUT1_SET -------------------------------------- */
#define SCT_OUT1_SET MMIO32(SCT_BASE + 0x508)

/* --- SCT_OUT2_SET -------------------------------------- */
#define SCT_OUT2_SET MMIO32(SCT_BASE + 0x510)

/* --- SCT_OUT3_SET -------------------------------------- */
#define SCT_OUT3_SET MMIO32(SCT_BASE + 0x518)

/* --- SCT_OUT4_SET -------------------------------------- */
#define SCT_OUT4_SET MMIO32(SCT_BASE + 0x520)

/* --- SCT_OUT5_SET -------------------------------------- */
#define SCT_OUT5_SET MMIO32(SCT_BASE + 0x528)

/* --- SCT_OUT6_SET -------------------------------------- */
#define SCT_OUT6_SET MMIO32(SCT_BASE + 0x530)

/* --- SCT_OUT7_SET -------------------------------------- */
#define SCT_OUT7_SET MMIO32(SCT_BASE + 0x538)

/* --- SCT_OUT8_SET -------------------------------------- */
#define SCT_OUT8_SET MMIO32(SCT_BASE + 0x540)

/* --- SCT_OUT9_SET -------------------------------------- */
#define SCT_OUT9_SET MMIO32(SCT_BASE + 0x548)

/* --- SCT_OUT10_SET ------------------------------------- */
#define SCT_OUT10_SET MMIO32(SCT_BASE + 0x550)

/* --- SCT_OUT11_SET ------------------------------------- */
#define SCT_OUT11_SET MMIO32(SCT_BASE + 0x558)

/* --- SCT_OUT12_SET ------------------------------------- */
#define SCT_OUT12_SET MMIO32(SCT_BASE + 0x560)

/* --- SCT_OUT13_SET ------------------------------------- */
#define SCT_OUT13_SET MMIO32(SCT_BASE + 0x568)

/* --- SCT_OUT14_SET ------------------------------------- */
#define SCT_OUT14_SET MMIO32(SCT_BASE + 0x570)

/* --- SCT_OUT15_SET ------------------------------------- */
#define SCT_OUT15_SET MMIO32(SCT_BASE + 0x578)

/* -- SCT_OUTn_SETm: A 1 in bit m selects event m to set output n (or clear it if SETCLRn = 0x1 or 0x2) event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15. */
#define SCT_OUTn_SETm(n, m) (((n)&1) << (m))

/* --- SCT_OUT0_CLR -------------------------------------- */
#define SCT_OUT0_CLR MMIO32(SCT_BASE + 0x504)

/* --- SCT_OUT1_CLR -------------------------------------- */
#define SCT_OUT1_CLR MMIO32(SCT_BASE + 0x50C)

/* --- SCT_OUT2_CLR -------------------------------------- */
#define SCT_OUT2_CLR MMIO32(SCT_BASE + 0x514)

/* --- SCT_OUT3_CLR -------------------------------------- */
#define SCT_OUT3_CLR MMIO32(SCT_BASE + 0x51C)

/* --- SCT_OUT4_CLR -------------------------------------- */
#define SCT_OUT4_CLR MMIO32(SCT_BASE + 0x524)

/* --- SCT_OUT5_CLR -------------------------------------- */
#define SCT_OUT5_CLR MMIO32(SCT_BASE + 0x52C)

/* --- SCT_OUT6_CLR -------------------------------------- */
#define SCT_OUT6_CLR MMIO32(SCT_BASE + 0x534)

/* --- SCT_OUT7_CLR -------------------------------------- */
#define SCT_OUT7_CLR MMIO32(SCT_BASE + 0x53C)

/* --- SCT_OUT8_CLR -------------------------------------- */
#define SCT_OUT8_CLR MMIO32(SCT_BASE + 0x544)

/* --- SCT_OUT9_CLR -------------------------------------- */
#define SCT_OUT9_CLR MMIO32(SCT_BASE + 0x54C)

/* --- SCT_OUT10_CLR ------------------------------------- */
#define SCT_OUT10_CLR MMIO32(SCT_BASE + 0x554)

/* --- SCT_OUT11_CLR ------------------------------------- */
#define SCT_OUT11_CLR MMIO32(SCT_BASE + 0x55C)

/* --- SCT_OUT12_CLR ------------------------------------- */
#define SCT_OUT12_CLR MMIO32(SCT_BASE + 0x564)

/* --- SCT_OUT13_CLR ------------------------------------- */
#define SCT_OUT13_CLR MMIO32(SCT_BASE + 0x56C)

/* --- SCT_OUT14_CLR ------------------------------------- */
#define SCT_OUT14_CLR MMIO32(SCT_BASE + 0x574)

/* --- SCT_OUT15_CLR ------------------------------------- */
#define SCT_OUT15_CLR MMIO32(SCT_BASE + 0x57C)

/* -- SCT_OUTn_CLR_CLRm: A 1 in bit m selects event m to clear output n (or set it if SETCLRn = 0x1 or 0x2) event 0 = bit 0, event 1 = bit 1,..., event 15 = bit 15. */
#define SCT_OUTn_CLRm(n, m) (((n)&1) << (m))

