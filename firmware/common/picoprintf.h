/*
 * Copyright (c) 2023-2026 rom-p
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

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
