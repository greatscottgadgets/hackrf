#ifndef __picoprintf_h_INCLUDED__
#define __picoprintf_h_INCLUDED__

#include <stddef.h>  // size_t
#include <stdarg.h>  // va_*


int pico_snprintf(char *pDest, size_t cbDest, const char *pFormat, ...);
int pico_vsnprintf(char *pDest, size_t cbDest, const char *pFormat, va_list vl);

// PLEASE use `pico_snprintf()` instead!!!  This function is vulnerable to buffer overflows
inline int pico_sprintf(char *pDest, const char *pFormat, ...) {
    va_list vl;
    va_start(vl, pFormat);
    int result = pico_vsnprintf(pDest, -1, pFormat, vl);
    va_end(vl);
    return result;
}


//
// IMPORTANT!!!
//  comment/uncomment the following lines corresponding to the features you need
//
// #define PICOFORMAT_HANDLE_FILL          // uncomment this line to handle "%6i" and "%04d" -- the fill with zeroes or spaces
// #define PICOFORMAT_HANDLE_FORCEDSIGN    // uncomment this line to handle "%+d" -- the forced sign placement
// #define PICOFORMAT_HANDLE_BIN           // uncomment this line to handle "%b" -- binary representation
// #define PICOFORMAT_HANDLE_OCT           // uncomment this line to handle "%o"
// #define PICOFORMAT_HANDLE_HEX           // uncomment this line to handle "%x" and "%X"
// #define PICOFORMAT_HANDLE_FLOATS        // uncomment this line to handle the "%f"
// #define PICOFORMAT_CLANG_QUIRK          // uncomment this line to match clang's non-standard "%010s" behavior (zero-pad strings when both '0' flag and width are set)


// by default, the debug builds (determined by `#define _DEBUG`) will real-time print errors when a feature is used that is not enabled above
#ifndef FORMAT_ERROR_DELEGATE
    #ifdef _DEBUG
        #define FORMAT_ERROR_DELEGATE(__message, __arg) printf(__message, __arg); printf("\n");
    #else // _DEBUG
        #define FORMAT_ERROR_DELEGATE(__message, __arg)
    #endif // _DEBUG
#endif // FORMAT_ERROR_DELEGATE

#endif // __picoprintf_h_INCLUDED__
