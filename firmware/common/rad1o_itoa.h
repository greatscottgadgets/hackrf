#ifndef _ITOA_H
#define _ITOA_H 1

#define F_ZEROS  (1<<0) // Print with leading zeros
#define F_LONG   (1<<1) // Print with leading spaces
#define F_SPLUS  (1<<2) // Prefix '+' on positive number
#define F_SSPACE (1<<3) // Prefix ' ' on positive numbers
#define F_HEX    (1<<4) // Print as (fixed-width) hex number
const char* IntToStr(int num, unsigned int mxlen, char flag);

#endif /* _ITOA_H */
