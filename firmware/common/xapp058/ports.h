/*******************************************************/
/* file: ports.h                                       */
/* abstract:  This file contains extern declarations   */
/*            for providing stimulus to the JTAG ports.*/
/*******************************************************/

#ifndef ports_dot_h
#define ports_dot_h

/* these constants are used to send the appropriate ports to setPort */
/* they should be enumerated types, but some of the microcontroller  */
/* compilers don't like enumerated types */
#define TCK (short) 0
#define TMS (short) 1
#define TDI (short) 2

/* set the port "p" (TCK, TMS, or TDI) to val (0 or 1) */
extern void setPort(short p, short val);

/* read the TDO bit and store it in val */
extern unsigned char readTDOBit();

/* make clock go down->up->down*/
extern void pulseClock();

/* read the next byte of data from the xsvf file */
extern void readByte(unsigned char *data);

extern void waitTime(long microsec);

#endif
