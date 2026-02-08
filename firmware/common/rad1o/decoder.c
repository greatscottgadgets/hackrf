#include "fonts.h"
#include "render.h"

#include <stdint.h>

// Local function: Get next nibble.
static int ctr = 0;  // offset for next nibble
static int hilo = 0; // 0= high nibble next, 1=low nibble next
static const uint8_t* data;

#define MAXCHR (30 * 20)
static uint8_t charBuf[MAXCHR];

// Get next nibble
static uint8_t gnn(void)
{
	static uint8_t byte;
	uint8_t val;
	if (hilo == 1)
		ctr++;
	hilo = 1 - hilo;
	if (hilo == 1) {
		byte = data[ctr];
		val = byte >> 4;
	} else {
		val = byte & 0x0f;
	};
	return val;
}

// Local function: Unpack "long run".
static int upl(int off)
{
	int retval;

	while ((retval = gnn()) == 0) {
		off++;
	};
	while (off-- > 0) {
		retval = retval << 4;
		retval += gnn();
	};
	return retval;
}

uint8_t* rad1o_pk_decode(const uint8_t* ldata, int* len)
{
	ctr = 0;
	hilo = 0;
	data = ldata;
	int length = *len;         // Length of character bytestream
	int height;                // Height of character in bytes
	int hoff;                  // bit position for non-integer heights
	uint8_t* bufptr = charBuf; // Output buffer for decoded character

	height = (rad1o_getFontHeight() - 1) / 8 + 1;
	hoff = rad1o_getFontHeight() % 8;

#define DYN (12)        // Decoder parameter: Fixed value for now.
	int repeat = 0; // Decoder internal: repeat colum?
	int curbit = 0; // Decoder internal: current bit (1 or 0)
	int pos = 0;    // Decoder internal: current bit position (0..7)
	int nyb;        // Decoder internal: current nibble / value

	if (data[ctr] >> 4 == 14) { // Char starts with 1-bits.
		gnn();
		curbit = 1;
	};

	while (ctr < length) { /* Iterate the whole input stream */

		/* Get next encoded nibble and decode */
		nyb = gnn();

		if (nyb == 15) {
			repeat++;
			continue;
		} else if (nyb == 14) {
			nyb = upl(0);
			nyb += 1;
			repeat += nyb;
			continue;
		} else if (nyb > DYN) {
			nyb = (16 * (nyb - DYN - 1)) + gnn() + DYN + 1;
		} else if (nyb == 0) {
			nyb = upl(1);
			nyb += (16 * (13 - DYN) + DYN) - 16;
		};

		/* Generate & output bits */

		while (nyb-- > 0) {
			if (pos == 0) // Clear each byte before we start.
				*bufptr = 0;
			if (curbit == 1) {
				*bufptr |= 1 << (7 - pos);
			};
			pos++;
			if (((bufptr - charBuf) % height) == (height - 1) &&
			    (pos == hoff)) {
				// Finish incomplete last byte per column
				pos = 8;
			};

			if (pos == 8) {
				bufptr++;
				if ((bufptr - charBuf) % height == 0) { // End of column?
					while (repeat > 0) {
						for (int y = 0; y < height; y++) {
							bufptr[0] = bufptr[-height];
							bufptr++;
						};
						repeat--;
					};
				};
				pos = 0;
			};
		};
		curbit = 1 - curbit;
	};

	*len = (bufptr - charBuf) / height; // return size of output buffer.
	return charBuf;
}
