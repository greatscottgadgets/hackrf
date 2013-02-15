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
/*#include "prgispx.h"*/

//#include "stdio.h"
#include "hackrf_core.h"

extern FILE *in;
//static int  g_iTCK = 0; /* For xapp058_example .exe */
//static int  g_iTMS = 0; /* For xapp058_example .exe */
//static int  g_iTDI = 0; /* For xapp058_example .exe */

#ifdef WIN95PP
#include "conio.h"

#define DATA_OFFSET    (unsigned short) 0
#define STATUS_OFFSET  (unsigned short) 1
#define CONTROL_OFFSET (unsigned short) 2

typedef union outPortUnion {
    unsigned char value;
    struct opBitsStr {
        unsigned char tdi:1;
        unsigned char tck:1;
        unsigned char tms:1;
        unsigned char zero:1;
        unsigned char one:1;
        unsigned char bit5:1;
        unsigned char bit6:1;
        unsigned char bit7:1;
    } bits;
} outPortType;

typedef union inPortUnion {
    unsigned char value;
    struct ipBitsStr {
        unsigned char bit0:1;
        unsigned char bit1:1;
        unsigned char bit2:1;
        unsigned char bit3:1;
        unsigned char tdo:1;
        unsigned char bit5:1;
        unsigned char bit6:1;
        unsigned char bit7:1;
    } bits;
} inPortType;

static inPortType in_word;
static outPortType out_word;
static unsigned short base_port = 0x378;
static int once = 0;
#endif


/*BYTE *xsvf_data=0;*/


/* setPort:  Implement to set the named JTAG signal (p) to the new value (v).*/
/* if in debugging mode, then just set the variables */
void setPort(short p,short val)
{
#ifdef WIN95PP
    /* Old Win95 example that is similar to a GPIO register implementation.
       The old Win95 example maps individual bits of the 
       8-bit register (out_word) to the JTAG signals: TCK, TMS, TDI. 
       */

    /* Initialize static out_word register bits just once */
    if (once == 0) {
        out_word.bits.one = 1;
        out_word.bits.zero = 0;
        once = 1;
    }

    /* Update the local out_word copy of the JTAG signal to the new value. */
    if (p==TMS)
        out_word.bits.tms = (unsigned char) val;
    if (p==TDI)
        out_word.bits.tdi = (unsigned char) val;
    if (p==TCK) {
        out_word.bits.tck = (unsigned char) val;
        (void) _outp( (unsigned short) (base_port + 0), out_word.value );
        /* To save HW write cycles, this example only writes the local copy
           of the JTAG signal values to the HW register when TCK changes. */
    }
#endif
    /* Printing code for the xapp058_example.exe.  You must set the specified
       JTAG signal (p) to the new value (v).  See the above, old Win95 code
       as an implementation example. */
/*
    if (p==TMS)
        g_iTMS = val;
    if (p==TDI)
        g_iTDI = val;
    if (p==TCK) {
        g_iTCK = val;
        printf( "TCK = %d;  TMS = %d;  TDI = %d\n", g_iTCK, g_iTMS, g_iTDI );
    }
*/
	if (p==TMS)
		if (val)
			gpio_set(PORT_CPLD_TMS, PIN_CPLD_TMS);
		else
			gpio_clear(PORT_CPLD_TMS, PIN_CPLD_TMS);
	if (p==TDI)
		if (val)
			gpio_set(PORT_CPLD_TDI, PIN_CPLD_TDI);
		else
			gpio_clear(PORT_CPLD_TDI, PIN_CPLD_TDI);
	if (p==TCK)
		if (val)
			gpio_set(PORT_CPLD_TCK, PIN_CPLD_TCK);
		else
			gpio_clear(PORT_CPLD_TCK, PIN_CPLD_TCK);

	/* conservative delay */
	delay(20000);
}


/* toggle tck LH.  No need to modify this code.  It is output via setPort. */
void pulseClock()
{
    setPort(TCK,0);  /* set the TCK port to low  */
    setPort(TCK,1);  /* set the TCK port to high */
}


/* readByte:  Implement to source the next byte from your XSVF file location */
/* read in a byte of data from the prom */
void readByte(unsigned char *data)
{
    /* pretend reading using a file */
    *data   = (unsigned char)fgetc( in );
    /**data=*xsvf_data++;*/
}

/* readTDOBit:  Implement to return the current value of the JTAG TDO signal.*/
/* read the TDO bit from port */
unsigned char readTDOBit()
{
#ifdef WIN95PP
    /* Old Win95 example that is similar to a GPIO register implementation.
       The old Win95 reads the hardware input register and extracts the TDO
       value from the bit within the register that is assigned to the
       physical JTAG TDO signal. 
       */
    in_word.value = (unsigned char) _inp( (unsigned short) (base_port + STATUS_OFFSET) );
    if (in_word.bits.tdo == 0x1) {
        return( (unsigned char) 1 );
    }
#endif
    /* You must return the current value of the JTAG TDO signal. */
    //return( (unsigned char) 0 );

	delay(2000);
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

#if 0
    /* Alternate implementation */
    /* For systems with TCK rates << 1 MHz;  Consider this implementation. */
    /* This implementation does not work with Spartan-3AN or indirect flash
       programming. */
    if ( microsec >= 50L )
    {
        /* Make sure TCK is low during wait for XC18V00/XCFxxS */
        /* Or, a running TCK implementation as shown above is an OK alternate */
        setPort( TCK, 0 );

        /* Use Windows Sleep().  Round up to the nearest millisec */
        _sleep( ( microsec + 999L ) / 1000L );
    }
    else    /* Satisfy FPGA JTAG configuration, startup TCK cycles */
    {
        for ( i = 0; i < microsec;  ++i )
        {
            pulseClock();
        }
    }
#endif

#if 0
    /* Alternate implementation */
    /* This implementation is valid for only XC9500/XL/XV, CoolRunner/II CPLDs, 
       XC18V00 PROMs, or Platform Flash XCFxxS/XCFxxP PROMs.  
       This implementation does not work with FPGAs JTAG configuration. */
    /* Make sure TCK is low during wait for XC18V00/XCFxxS PROMs */
    /* Or, a running TCK implementation as shown above is an OK alternate */
    setPort( TCK, 0 );
    /* Use Windows Sleep().  Round up to the nearest millisec */
    _sleep( ( microsec + 999L ) / 1000L );
#endif
}
