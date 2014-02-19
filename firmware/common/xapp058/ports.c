/*******************************************************/
/* file: ports.c                                       */
/* abstract:  This file contains the routines to       */
/*            output values on the JTAG ports, to read */
/*            the TDO bit, and to read a byte of data  */
/*            from the prom                            */
/* Revisions:                                          */
/* 12/01/2008:  Same code as before (original v5.01).  */
/*              Updated comments to clarify instructions.*/
/*              Add print in setPort for xapp058_example.exe.*/
/*******************************************************/
#include "ports.h"

#include "hackrf_core.h"
#include "cpld_jtag.h"
#include <libopencm3/lpc43xx/gpio.h>

void delay_jtag(uint32_t duration)
{
	#define DIVISOR	(1024)
	#define MIN_NOP (8)

	uint32_t i;
	uint32_t delay_nop;

	/* @204Mhz duration of about 400ns for delay_nop=20 */
	if(duration < DIVISOR)
	{
		delay_nop = MIN_NOP;
	}else
	{
		delay_nop = (duration / DIVISOR) + MIN_NOP;
	}

	for (i = 0; i < delay_nop; i++)
		__asm__("nop");
}

/* setPort:  Implement to set the named JTAG signal (p) to the new value (v).*/
/* if in debugging mode, then just set the variables */
void setPort(short p,short val)
{
	if (p==TMS) {
		if (val)
			gpio_set(PORT_CPLD_TMS, PIN_CPLD_TMS);
		else
			gpio_clear(PORT_CPLD_TMS, PIN_CPLD_TMS);
	} if (p==TDI) {
		if (val)
			gpio_set(PORT_CPLD_TDI, PIN_CPLD_TDI);
		else
			gpio_clear(PORT_CPLD_TDI, PIN_CPLD_TDI);
	} if (p==TCK) {
		if (val)
			gpio_set(PORT_CPLD_TCK, PIN_CPLD_TCK);
		else
			gpio_clear(PORT_CPLD_TCK, PIN_CPLD_TCK);
	}

	/* conservative delay */
	delay_jtag(20000);
}


/* toggle tck LH.  No need to modify this code.  It is output via setPort. */
void pulseClock()
{
    setPort(TCK,0);  /* set the TCK port to low  */
	delay_jtag(200);
    setPort(TCK,1);  /* set the TCK port to high */
	delay_jtag(200);
}


/* readByte:  Implement to source the next byte from your XSVF file location */
/* read in a byte of data from the prom */
void readByte(unsigned char *data)
{
	*data = cpld_jtag_get_next_byte();
}

/* readTDOBit:  Implement to return the current value of the JTAG TDO signal.*/
/* read the TDO bit from port */
unsigned char readTDOBit()
{
	delay_jtag(2000);
	return CPLD_TDO_STATE;
}

/* waitTime:  Implement as follows: */
/* REQUIRED:  This function must consume/wait at least the specified number  */
/*            of microsec, interpreting microsec as a number of microseconds.*/
/* REQUIRED FOR SPARTAN/VIRTEX FPGAs and indirect flash programming:         */
/*            This function must pulse TCK for at least microsec times,      */
/*            interpreting microsec as an integer value.                     */
/* RECOMMENDED IMPLEMENTATION:  Pulse TCK at least microsec times AND        */
/*                              continue pulsing TCK until the microsec wait */
/*                              requirement is also satisfied.               */
void waitTime(long microsec)
{
    static long tckCyclesPerMicrosec    = 1; /* must be at least 1 */
    long        tckCycles   = microsec * tckCyclesPerMicrosec;
    long        i;

    /* This implementation is highly recommended!!! */
    /* This implementation requires you to tune the tckCyclesPerMicrosec 
       variable (above) to match the performance of your embedded system
       in order to satisfy the microsec wait time requirement. */
    for ( i = 0; i < tckCycles; ++i )
    {
        pulseClock();
    }
}
