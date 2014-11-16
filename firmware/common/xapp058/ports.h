/*******************************************************/
/* file: ports.h                                       */
/* abstract:  This file contains extern declarations   */
/*            for providing stimulus to the JTAG ports.*/
/*******************************************************/

#ifndef ports_dot_h
#define ports_dot_h

#include "cpld_jtag.h"

/* these constants are used to send the appropriate ports to setPort */
/* they should be enumerated types, but some of the microcontroller  */
/* compilers don't like enumerated types */
#define TCK (short) 0
#define TMS (short) 1
#define TDI (short) 2

/* set the port "p" (TCK, TMS, or TDI) to val (0 or 1) */
extern void setPort(jtag_gpio_t* const gpio, short p, short val);

/* read the TDO bit and store it in val */
extern unsigned char readTDOBit(jtag_gpio_t* const gpio);

/* make clock go down->up->down*/
extern void pulseClock(jtag_gpio_t* const gpio);

/* read the next byte of data from the xsvf file */
extern void readByte(unsigned char *data);

extern void waitTime(jtag_gpio_t* const gpio, long microsec);

#endif
