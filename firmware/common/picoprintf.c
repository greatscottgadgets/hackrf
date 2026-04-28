/* MIT License
 *
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

#include "picoprintf.h"

#include <math.h>    // fabs()
#include <stdlib.h>  // llabs()


static void flip(char *pLeft, char *pRight) {
    pRight--;
    while (pLeft < pRight) {
        char tmp = *pLeft;
        *pLeft = *pRight;
        *pRight = tmp;
        pLeft++;
        pRight--;
    }
}

// using #define over `inline` for enabling porting to old C
#if !defined(MIN)
#define MIN(left, right) (((left) < (right)) ? (left) : (right))
#endif
#if !defined(MAX)
#define MAX(left, right) (((left) > (right)) ? (left) : (right))
#endif


// returns the pointer to the null-terminating character of the filled string
int pico_vsnprintf(char *pDest, size_t cbDest, const char *pFormat, va_list vl) {
    const char *pLowercaseNumberDigits = "0123456789abcdef";
    const char *pUppercaseNumberDigits = "0123456789ABCDEF";
    const char *pOctalDigits = pLowercaseNumberDigits;
    const char *pBinaryDigits = pLowercaseNumberDigits;
    char *pStart = pDest;
    for (char *pEnd = pDest + cbDest - 1; *pFormat && pDest < pEnd; ) {
        if (*pFormat != '%') {
            *pDest++ = *pFormat++;
        } else {                            // format starts here
            pFormat++;                      // skipping the '%'
            if (*pFormat == '%') {          // unless it's indeed a '%'
                *pDest++ = *pFormat++;
            } else {                        // first, collect the format
                char format = '\0';
                int bits_per_digit = 0;     // valid in 'b', 'o', 'p' and 'x'/'X' modes only
                int whole_chars = 0;        // number of chars in the whole part of the number
                int decimal_chars = -1;     // if not specified, %f are rendered with 6, while %g are rendered with 0
            #ifdef __aarch64__              // these platforms benefit from packing flags into a single variable
                struct {
                    unsigned force_sign:1;
                    unsigned fill_zeros:1;
                    unsigned left_align:1;
                    unsigned seen_period:1;
                    unsigned seen_numbers:1;
                    unsigned treat_as_unsigned:1;
                    unsigned treat_as_long:1;
                    unsigned render_in_lowercase:1;
                } flags = {0};
                #define FLAGS flags.

            #else                           // other platforms do not benefit from struct packing
                unsigned force_sign = 0;
                unsigned fill_zeros = 0;
                unsigned left_align = 0;
                unsigned seen_period = 0;
                unsigned seen_numbers = 0;
                unsigned treat_as_unsigned = 0;
                unsigned treat_as_long = 0;
                unsigned render_in_lowercase = 0;
                #define FLAGS
            #endif                          // struct packing platforms
                for (; *pFormat && '\0' == format; pFormat++) {
                    switch (*pFormat) {
                #ifdef PICOFORMAT_HANDLE_FORCEDSIGN
                    case '+':
                        FLAGS force_sign = 1;
                        break;
                #endif // PICOFORMAT_HANDLE_FORCEDSIGN
                #ifdef PICOFORMAT_HANDLE_FILL
                    case '-':    // left-align flag
                        FLAGS left_align = 1;
                        break;
                #endif // PICOFORMAT_HANDLE_FILL
                    case '0':
                        if (!FLAGS seen_numbers) {
                            FLAGS fill_zeros = 1;
                            break;
                        }
                        // fall through
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                        *(FLAGS seen_period ? &decimal_chars : &whole_chars ) *= 10;
                        *(FLAGS seen_period ? &decimal_chars : &whole_chars) += *pFormat - '0';
                        FLAGS seen_numbers = 1;
                        break;
                    case '.':
                        FLAGS seen_numbers = FLAGS seen_period = 1;
                        decimal_chars = 0;
                        break;
                    case '*':                        // dynamic width/precision: read value from arg list
                        *(FLAGS seen_period ? &decimal_chars : &whole_chars) = va_arg(vl, int);
                        FLAGS seen_numbers = 1;
                        break;
                    case 'l':    // long modifier
                        FLAGS treat_as_long = 1;
                        break;
                    case 'u':    // unsigned decimal integer
                        FLAGS treat_as_unsigned = 1;
                        format = 'd';
                        break;
                #ifdef PICOFORMAT_HANDLE_BIN
                    case 'b':
                        bits_per_digit = 0;
                        FLAGS treat_as_unsigned = 1;
                        format = 'b';
                        break;
                #endif // PICOFORMAT_HANDLE_BIN
                #ifdef PICOFORMAT_HANDLE_OCT
                    case 'o':    // octal integer
                        bits_per_digit = 3;
                        FLAGS treat_as_unsigned = 1;
                        format = 'b';
                        break;
                #endif // PICOFORMAT_HANDLE_OCT
                #ifdef PICOFORMAT_HANDLE_HEX
                    case 'x':    // hexadecimal integer
                    case 'p':    // pointer
                        bits_per_digit = 4;
                        FLAGS treat_as_unsigned = 1;
                        FLAGS render_in_lowercase = 1;
                        format = 'b';
                        break;
                    case 'X':    // hexadecimal integer, uppercase
                        bits_per_digit = 4;
                        FLAGS treat_as_unsigned = 1;
                        format = 'b';
                        break;
                #endif // PICOFORMAT_HANDLE_HEX
                #ifdef PICOFORMAT_HANDLE_FLOATS
                    case 'a':
                #ifdef PICOFORMAT_HANDLE_EXPONENTS
                    case 'e':   // floating point, exponent format
                #endif // PICOFORMAT_HANDLE_EXPONENTS
                    case 'f':
                        FLAGS render_in_lowercase = 1;
                        // fall through
                    case 'F':
                #endif // PICOFORMAT_HANDLE_FLOATS
                    case 'c':  // single char: always supported
                    case 'd':  // integer: always supported
                    case 'i':  // integer: always supported
                    case 's':  // string of native `char`s: always supported
                        format = *pFormat;
                        break;
                    default:
                        FORMAT_ERROR_DELEGATE("detected unhandled format specifier: %c", *pFormat);
                        break;
                    }
                }

                // format is parsed, now render the value
                char *pParamStarts = pDest;
                switch (format) {
                case 'c':           // single char
                    *pDest++ = va_arg(vl, int) & 0xff;
                    break;
                case 's': {         // null-terminated string
                #ifdef PICOFORMAT_HANDLE_FILL
                        int len = 0;                                // effective length, bounded by precision if set
                        const char *pStr = va_arg(vl, const char*);
                        for (; pStr[len] && (decimal_chars < 0 || len < decimal_chars); len++);
                        if (!FLAGS left_align) {                    // right-align: pad on the left
                #ifdef PICOFORMAT_CLANG_QUIRK                       // clang's non-standard: '0' flag zero-pads strings
                            char chFill = FLAGS fill_zeros ? '0' : ' ';
                #else  // PICOFORMAT_CLANG_QUIRK                     // standard C: '0' flag is undefined for %s, use spaces
                            char chFill = ' ';
                #endif // PICOFORMAT_CLANG_QUIRK
                            for (; pDest < pEnd && whole_chars > len; whole_chars--) {
                                *pDest++ = chFill;
                            }
                        }
                        for (int ii = 0; ii < len && pDest < pEnd; ii++) {
                            *pDest++ = pStr[ii];
                        }
                        if (FLAGS left_align) {                     // left-align: pad on the right (always spaces; '-' flag overrides '0')
                            for (; pDest < pEnd && whole_chars > len; whole_chars--) {
                                *pDest++ = ' ';
                            }
                        }
                #else  // PICOFORMAT_HANDLE_FILL
                        for (const char *pStr = va_arg(vl, const char*); *pStr && pDest < pEnd; ) {
                            *pDest++ = *pStr++;
                        }
                #endif // PICOFORMAT_HANDLE_FILL
                    }
                    break;
            #if defined(PICOFORMAT_HANDLE_HEX) || defined(PICOFORMAT_HANDLE_OCT) || defined(PICOFORMAT_HANDLE_BIN)
                case 'b': {          // binary, oct, or hex integer, always unsigned
                        long long int val = 0;
                        if (FLAGS treat_as_long) {
                            val = va_arg(vl, unsigned long long int);
                        } else {
                            val = va_arg(vl, unsigned);
                        }
                        unsigned mask = bits_per_digit == 4 ? 0x0f : bits_per_digit == 3 ? 0x07 : 0x01;
                #if defined(PICOFORMAT_HANDLE_HEX)
                        const char* chars = FLAGS render_in_lowercase ? pLowercaseNumberDigits : pUppercaseNumberDigits;
                #elif defined(PICOFORMAT_HANDLE_OCT)
                        const char* chars = pOctalDigits;
                #else
                        const char* chars = pBinaryDigits;
                #endif // individual non-decimal formats
                        while (pDest < pEnd
                            && (pDest == pParamStarts
                             || val
                             || pDest - pParamStarts < whole_chars)) {
                            char ch;
                            if (pDest == pParamStarts || 0 != val) { // if first char or there's still meaningful digits
                                ch = chars[val & mask];
                                val >>= bits_per_digit;
                            } else {
                                ch = FLAGS fill_zeros ? '0' : ' ';
                            }
                            *pDest++ = ch;
                        }
                        flip(pParamStarts, pDest);
                    }
                    break;
            #endif // defined(PICOFORMAT_HANDLE_BIN) || defined(PICOFORMAT_HANDLE_OCT) || defined(PICOFORMAT_HANDLE_HEX)
                case 'd':           // decimal integer
                case 'i': {
                        long long int val = 0;
                        if (FLAGS treat_as_long) {
                            if (FLAGS treat_as_unsigned) {
                                val = va_arg(vl, unsigned long long int);
                            } else {
                                val = va_arg(vl, long long int);
                            }
                        } else {
                            if (FLAGS treat_as_unsigned) {
                                val = va_arg(vl, unsigned);
                            } else {
                                val = va_arg(vl, int);
                            }
                        }
                        char chSign = '\0';
                        if ((FLAGS force_sign || val < 0) && pDest < pEnd) {
                            chSign = val < 0 ? '-' : '+';
                            val = llabs(val);
                            whole_chars--;
                        }
                        while (pDest < pEnd
                            && (pDest == pParamStarts
                             || val
                             || pDest - pParamStarts < whole_chars)) {
                            char ch;
                            if (pDest == pParamStarts || 0 != val) {        // if first digit (i.e. zero) or there's still non-zero digits to write
                                ch = val % 10 + '0';
                                val /= 10;
                            } else {
                                if ('\0' != chSign && !FLAGS fill_zeros) {  // write a sign if filling with spaces, not with zeros
                                    *pDest++ = chSign;
                                    whole_chars++;
                                    chSign = '\0';                          // prevent the sign from being written twice
                                }
                        #ifdef PICOFORMAT_HANDLE_FILL
                                ch = FLAGS fill_zeros ? '0' : ' ';
                        #else // PICOFORMAT_HANDLE_FILL
                                    break;
                        #endif // PICOFORMAT_HANDLE_FILL
                            }
                            *pDest++ = ch;
                        }
                        if ('\0' != chSign) {                               // write a sign if it wasn't written before
                            *pDest++ = chSign;
                        }
                        flip(pParamStarts, pDest);
                    }
                    break;
            #ifdef PICOFORMAT_HANDLE_FLOATS
                case 'f': case 'F': {
                        double val = va_arg(vl, double);
                        if (FLAGS force_sign || val < 0.f) {
                            *pDest++ = val < 0.f ? '-' : '+';
                            val = fabs(val);
                            pParamStarts = pDest;
                        }
                        if (val == INFINITY || val == -INFINITY || isnan(val)) {
                            for (const char *pszVal = isnan(val) ? FLAGS render_in_lowercase ? "nan" : "NAN": FLAGS render_in_lowercase ? "inf" : "INF"
                               ; pDest < pEnd && *pszVal
                               ; *pDest++ = *pszVal++);
                        } else {
                            if (decimal_chars == -1) {
                                if (format == 'g') {
                                    decimal_chars = 0;
                                } else {
                                    decimal_chars = 6;
                                }
                            }
                            whole_chars = MAX(0, whole_chars - decimal_chars);
                            whole_chars = MAX(whole_chars, pDest - pParamStarts + 1); // at least sign (if present) and first char
                            for (float digit = 1.f
                               ; pDest < pEnd && (val >= digit || pDest - pParamStarts < whole_chars)
                               ; digit *= 10.f) {
                                char ch = (int)(val / digit) % 10 + '0';
                                *pDest++ = ch;
                            }
                            flip(pParamStarts, pDest);
                            if (decimal_chars && pDest < pEnd) {
                                *pDest++ = '.';
                                val += .5f * pow(10.f, -decimal_chars); // compensating for rounding error
                                for (size_t digit = 0; pDest < pEnd && digit < decimal_chars; digit++) {
                                    val = (val - (int)(val)) * 10.f;
                                    *pDest++ = (int)(val) + '0';
                                }
                            }
                        }
                    }
                    break;
            #endif // PICOFORMAT_HANDLE_FLOATS
                }
            }
        }
    }
    *pDest = '\0';
    return pDest - pStart;
}


int pico_snprintf(char *pDest, size_t cbDest, const char *pFormat, ...) {
    va_list vl;
    va_start(vl, pFormat);
    int result = pico_vsnprintf(pDest, cbDest, pFormat, vl);
    va_end(vl);
    return result;
}
