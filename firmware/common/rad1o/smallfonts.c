/* Partially based on original code for the KS0108 by Stephane Rey */

/**************************************************************************/
/*! 
    @file     smallfonts.c
    @author   K. Townsend (microBuilder.eu)
    @date     22 March 2010
    @version  0.10

    @section LICENSE

    Software License Agreement (BSD License)

    Copyright (c) 2010, microBuilder SARL
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
    1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    3. Neither the name of the copyright holders nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/**************************************************************************/
#include "smallfonts.h"

/* System 7x8 */
const uint8_t au8Font7x8[] = {
	0,   0,   0,   0,   0,   0,   0, //' '
	0,   6,   95,  95,  6,   0,   0, //'!'
	0,   7,   7,   0,   7,   7,   0, //'"'
	20,  127, 127, 20,  127, 127, 20, //'#'
	36,  46,  107, 107, 58,  18,  0, //'$'
	70,  102, 48,  24,  12,  102, 98, //'%'
	48,  122, 79,  93,  55,  122, 72, //'&'
	4,   7,   3,   0,   0,   0,   0, //'''
	0,   28,  62,  99,  65,  0,   0, //'('
	0,   65,  99,  62,  28,  0,   0, //')'
	8,   42,  62,  28,  28,  62,  42, //'*'
	8,   8,   62,  62,  8,   8,   0, //'+'
	0,   128, 224, 96,  0,   0,   0, //','
	8,   8,   8,   8,   8,   8,   0, //'-'
	0,   0,   96,  96,  0,   0,   0, //'.'
	96,  48,  24,  12,  6,   3,   1, //'/'
	62,  127, 113, 89,  77,  127, 62, //'0'
	64,  66,  127, 127, 64,  64,  0, //'1'
	98,  115, 89,  73,  111, 102, 0, //'2'
	34,  99,  73,  73,  127, 54,  0, //'3'
	24,  28,  22,  83,  127, 127, 80, //'4'
	39,  103, 69,  69,  125, 57,  0, //'5'
	60,  126, 75,  73,  121, 48,  0, //'6'
	3,   3,   113, 121, 15,  7,   0, //'7'
	54,  127, 73,  73,  127, 54,  0, //'8'
	6,   79,  73,  105, 63,  30,  0, //'9'
	0,   0,   102, 102, 0,   0,   0, //':'
	0,   128, 230, 102, 0,   0,   0, //';'
	8,   28,  54,  99,  65,  0,   0, //'<'
	36,  36,  36,  36,  36,  36,  0, //'='
	0,   65,  99,  54,  28,  8,   0, //'>'
	2,   3,   81,  89,  15,  6,   0, //'?'
	62,  127, 65,  93,  93,  31,  30, //'@'
	124, 126, 19,  19,  126, 124, 0, //'A'
	65,  127, 127, 73,  73,  127, 54, //'B'
	28,  62,  99,  65,  65,  99,  34, //'C'
	65,  127, 127, 65,  99,  62,  28, //'D'
	65,  127, 127, 73,  93,  65,  99, //'E'
	65,  127, 127, 73,  29,  1,   3, //'F'
	28,  62,  99,  65,  81,  115, 114, //'G'
	127, 127, 8,   8,   127, 127, 0, //'H'
	0,   65,  127, 127, 65,  0,   0, //'I'
	48,  112, 64,  65,  127, 63,  1, //'J'
	65,  127, 127, 8,   28,  119, 99, //'K'
	65,  127, 127, 65,  64,  96,  112, //'L'
	127, 127, 14,  28,  14,  127, 127, //'M'
	127, 127, 6,   12,  24,  127, 127, //'N'
	28,  62,  99,  65,  99,  62,  28, //'O'
	65,  127, 127, 73,  9,   15,  6, //'P'
	30,  63,  33,  113, 127, 94,  0, //'Q'
	65,  127, 127, 9,   25,  127, 102, //'R'
	38,  111, 77,  89,  115, 50,  0, //'S'
	3,   65,  127, 127, 65,  3,   0, //'T'
	127, 127, 64,  64,  127, 127, 0, //'U'
	31,  63,  96,  96,  63,  31,  0, //'V'
	127, 127, 48,  24,  48,  127, 127, //'W'
	67,  103, 60,  24,  60,  103, 67, //'X'
	7,   79,  120, 120, 79,  7,   0, //'Y'
	71,  99,  113, 89,  77,  103, 115, //'Z'
	0,   127, 127, 65,  65,  0,   0, //'['
	1,   3,   6,   12,  24,  48,  96, //'\'
	0,   65,  65,  127, 127, 0,   0, //']'
	8,   12,  6,   3,   6,   12,  8, //'^'
	128, 128, 128, 128, 128, 128, 128, //'_'
	0,   0,   3,   7,   4,   0,   0, //'`'
	32,  116, 84,  84,  60,  120, 64, //'a'
	65,  127, 63,  72,  72,  120, 48, //'b'
	56,  124, 68,  68,  108, 40,  0, //'c'
	48,  120, 72,  73,  63,  127, 64, //'d'
	56,  124, 84,  84,  92,  24,  0, //'e'
	72,  126, 127, 73,  3,   2,   0, //'f'
	56,  188, 164, 164, 252, 120, 0, //'g'
	65,  127, 127, 8,   4,   124, 120, //'h'
	0,   68,  125, 125, 64,  0,   0, //'i'
	96,  224, 128, 128, 253, 125, 0, //'j'
	65,  127, 127, 16,  56,  108, 68, //'k'
	0,   65,  127, 127, 64,  0,   0, //'l'
	120, 124, 28,  56,  28,  124, 120, //'m'
	124, 124, 4,   4,   124, 120, 0, //'n'
	56,  124, 68,  68,  124, 56,  0, //'o'
	0,   252, 252, 164, 36,  60,  24, //'p'
	24,  60,  36,  164, 248, 252, 132, //'q'
	68,  124, 120, 76,  4,   28,  24, //'r'
	72,  92,  84,  84,  116, 36,  0, //'s'
	0,   4,   62,  127, 68,  36,  0, //'t'
	60,  124, 64,  64,  60,  124, 64, //'u'
	28,  60,  96,  96,  60,  28,  0, //'v'
	60,  124, 112, 56,  112, 124, 60, //'w'
	68,  108, 56,  16,  56,  108, 68, //'x'
	60,  188, 160, 160, 252, 124, 0, //'y'
	76,  100, 116, 92,  76,  100, 0, //'z'
	8,   8,   62,  119, 65,  65,  0, //'{'
	0,   0,   0,   119, 119, 0,   0, //'|'
	65,  65,  119, 62,  8,   8,   0, //'}'
	2,   3,   1,   3,   2,   3,   1, //'~'
	255, 129, 129, 129, 129, 129, 255, //'^?'
	14,  159, 145, 177, 251, 74,  0 //'?'
};

/* Global variables */
const struct FONT_DEF Font_7x8 = { 7, 8, 32, 128, au8Font7x8, NULL, NULL };
