/*******************************************************/
/* file: lenval.h                                      */
/* abstract:  This file contains a description of the  */
/*            data structure "lenval".                 */
/*******************************************************/

#ifndef lenval_dot_h
#define lenval_dot_h

/* the lenVal structure is a byte oriented type used to store an */
/* arbitrary length binary value. As an example, the hex value   */
/* 0x0e3d is represented as a lenVal with len=2 (since 2 bytes   */
/* and val[0]=0e and val[1]=3d.  val[2-MAX_LEN] are undefined    */

/* maximum length (in bytes) of value to read in        */
/* this needs to be at least 4, and longer than the     */
/* length of the longest SDR instruction.  If there is, */
/* only 1 device in the chain, MAX_LEN must be at least */
/* ceil(27/8) == 4.  For 6 devices in a chain, MAX_LEN  */
/* must be 5, for 14 devices MAX_LEN must be 6, for 20  */
/* devices MAX_LEN must be 7, etc..                     */
/* You can safely set MAX_LEN to a smaller number if you*/
/* know how many devices will be in your chain.         */
/*  #define MAX_LEN (Actual #define is below this comment block)
      This #define defines the maximum length (in bytes) of predefined
      buffers in which the XSVF player stores the current shift data.
      This length must be greater than the longest shift length (in bytes)
      in the XSVF files that will be processed.  7000 is a very conservative
      number.  The buffers are stored on the stack and if you have limited
      stack space, you may decrease the MAX_LEN value.

      How to find the "shift length" in bits?
      Look at the ASCII version of the XSVF (generated with the -a option
      for the SVF2XSVF translator) and search for the XSDRSIZE command
      with the biggest parameter.  XSDRSIZE is equivalent to the SVF's
      SDR length plus the lengths of applicable HDR and TDR commands.
      Remember that the MAX_LEN is defined in bytes.  Therefore, the
      minimum MAX_LEN = ceil( max( XSDRSIZE ) / 8 );

      The following MAX_LEN values have been tested and provide relatively
      good margin for the corresponding devices:

        DEVICE       MAX_LEN   Resulting Shift Length Max (in bits)
        ---------    -------   ----------------------------------------------
        XC9500/XL/XV 32        256

        CoolRunner/II 256      2048   - actual max 1 device = 1035 bits

        FPGA         128       1024    - svf2xsvf -rlen 1024

        XC18V00/XCF00
                     1100      8800   - no blank check performed (default)
                                      - actual max 1 device = 8192 bits verify
                                      - max 1 device = 4096 bits program-only

        XC18V00/XCF00 when using the optional Blank Check operation
                     2500      20000  - required for blank check
                                      - blank check max 1 device = 16384 bits
*/
#define MAX_LEN 7000


typedef struct var_len_byte
{
    short len;   /* number of chars in this value */
    unsigned char val[MAX_LEN+1];  /* bytes of data */
} lenVal;


/* return the long representation of a lenVal */
extern long value(lenVal *x);

/* set lenVal equal to value */
extern void initLenVal(lenVal *x, long value);

/* check if expected equals actual (taking the mask into account) */
extern short EqualLenVal(lenVal *expected, lenVal *actual, lenVal *mask);

/* add val1+val2 and put the result in resVal */
extern void addVal(lenVal *resVal, lenVal *val1, lenVal *val2);

/* return the (byte, bit) of lv (reading from left to right) */
extern short RetBit(lenVal *lv, int byte, int bit);

/* set the (byte, bit) of lv equal to val (e.g. SetBit("00000000",byte, 1)
   equals "01000000" */
extern void SetBit(lenVal *lv, int byte, int bit, short val);

/* read from XSVF numBytes bytes of data into x */
extern void  readVal(lenVal *x, short numBytes);

#endif

