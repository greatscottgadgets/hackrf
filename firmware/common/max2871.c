#include <stdint.h>

/*
 * - The input is fixed to 50 MHz
 * f_REF = 50 MHz
 *
 * - The VCO must operate between 3 GHz and 6 GHz
 * f_VCO range: 3 GHz - 6 GHz
 *
 * Phase Detector Clock:
 *      - We configure the phase detector to not change the
 *        input signal
 *      f_PFD = f_REF x [(1 + DBR)/(R x (1 + RDIV2))]
 *      R = 1
 *      DBR = 0
 *      RDIV2 = 0
 *
 *      => f_PFD = 50 MHz
 *
 *
 * f_RFOUT < 3 GHz:
 *  - To go to < 3 GHz, f_VCO has to be divided.
 *  f_RFOUT = f_VCO / DIVA
 *
 *  - We just divide by 2 for now
 *  DIVA = 2
 *  => f_RFOUT = f_VCO / 2
 *  
 *  - Relationship between f_PFD (fixed) and f_VCO (variable)
 *  N + (F/M) = f_VCO/ f_PFD
 *
 *  - Insert relationship between f_RFOUT and f_VCO:
 *  N + (F/M) = (f_RFOUT * 2) / f_PFD
 *  f_RFOUT = (N + F/M) * f_PFD / 2
 *
 *  - Limits for N, M and F from the datasheet:
 *  19 <= N <= 4091
 *  2  <= M <= 4095
 *  F < M
 *
 *  - Plug in constants:
 *  f_RFOUT = (N + F/M) / 2 * 50 MHz
 *
 *  - Given the range of N, we can go to:
 *  f_RFOUT range: 475 MHz - 3 GHz
 *
 *  - N steps in 25 MHz increments:
 *  N = floor(f_RFOUT / 50 MHz * 2)
 *      => N = (f_RFOUT * 2) / 50 MHz (uses integer math)
 *
 *  - Calculate the error:
 *  f_int_ERROR = f_RFOUT - (N * 50 MHz) / 2
 *  f_int_ERROR range: 0 MHz - 24 MHz
 *
 *  - Use the fraction to get to the correct frequency:
 *  (F/M) / 2 * 50 MHz = f_int_ERROR
 *
 *  - Fix M:
 *  M = 25
 * 
 *  - Calculate F:
 *  (F/25) / 2 * 50 MHz = f_int_ERROR
 *  F = f_int_ERROR / 1 MHz
 *
 *  - Calculate the new error:
 *
 *  f_ERROR = f_RFOUT - (N + F/M) / 2 * 50 MHz
 *  f_ERROR range: 0 MHz - 1 MHz
 *
 *
 * f_RFOUT > 3 GHz:
 *  - Do not divide the VCO output
 *  DIVA = 1
 *  => f_RFOUT = f_VCO
 *
 *  - Relationship between f_PFD (fixed) and f_VCO (variable)
 *  N + (F/M) = f_VCO/ f_PFD
 *
 *  - Insert relationship between f_RFOUT and f_VCO:
 *  N + (F/M) = f_RFOUT / f_PFD
 *  f_RFOUT = (N + F/M) * f_PFD
 *
 *  - Limits for N, M and F from the datasheet:
 *  19 <= N <= 4091
 *  2  <= M <= 4095
 *  F < M
 *
 *  - Plug in constants:
 *  f_RFOUT = (N + F/M) * 50 MHz
 *
 *  - Given the range of N and the VCO limits, we can go to:
 *  f_RFOUT range: 3000 MHz - 6000 MHz
 *
 *  - N steps in 50 MHz increments:
 *  N = floor(f_RFOUT / 50 MHz)
 *      => N = f_RFOUT / 50 MHz (uses integer math)
 *
 *  - Calculate the error:
 *  f_int_ERROR = f_RFOUT - N * 50 MHz
 *  f_int_ERROR range: 0 MHz - 49 MHz
 *
 *  - Use the fraction to get to the correct frequency:
 *  (F/M) * 50 MHz = f_int_ERROR
 *
 *  - Fix M:
 *  M = 50
 * 
 *  - Calculate F:
 *  (F/50) * 50 MHz = f_int_ERROR
 *  F = f_int_ERROR / 1 MHz
 *
 *  - Calculate the new error:
 *
 *  f_ERROR = f_RFOUT - (N + F/M) * 50 MHz
 *  f_ERROR range: 0 MHz - 1 MHz
 *
 */

void mixer_init(void)
{}
void mixer_setup(void)
{}

/* Set frequency (MHz). */
uint64_t mixer_set_frequency(uint16_t mhz)
{
    (void) mhz;
    return mhz;
}

void mixer_tx(void)
{}
void mixer_rx(void)
{}
void mixer_rxtx(void)
{}
void mixer_enable(void)
{}
void mixer_disable(void)
{}
void mixer_set_gpo(uint8_t gpo)
{
    (void) gpo;
}


