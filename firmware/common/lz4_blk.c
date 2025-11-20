/*
 * Copyright 2024 Great Scott Gadgets <info@greatscottgadgets.com>
 *
 * This file is part of HackRF.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "lz4_blk.h"

// Decompress raw LZ4 block.
int lz4_blk_decompress(const uint8_t* src, uint8_t* dst, size_t length)
{
	const uint8_t* src_end = src + length; // Point to the end of the current block.
	const uint8_t* dst_0 = dst; // Store original dst pointer to compute output size.

	while (src < src_end) {
		uint8_t token = *src++;

		// Get the literal length from the high nibble of the token.
		uint32_t literal_length = token >> 4;

		// If literal length is 15 or more, we need to read additional length bytes.
		if (literal_length == 0x0F) {
			uint8_t len;
			while ((len = *src++) == 0xFF) {
				literal_length += 0xFF;
			}
			literal_length += len;
		}

		// Copy the literals, if any.
		if (literal_length > 0) {
			memcpy(dst, src, literal_length);
			src += literal_length;
			dst += literal_length;
		}

		// If we're at the end, break (no match data to process).
		if (src >= src_end) {
			break;
		}

		// Get the match offset (2 bytes).
		uint16_t offset = src[0] | (src[1] << 8);
		src += 2;

		// Match length (low nibble of token + 4).
		uint32_t match_length = (token & 0x0F) + 4;

		// If match length is 19 or more, we need to read additional length bytes.
		if ((token & 0x0F) == 0x0F) {
			uint8_t len;
			while ((len = *src++) == 0xFF) {
				match_length += 0xFF;
			}
			match_length += len;
		}

		// Copy the match data.
		const uint8_t* match_ptr = dst - offset;
		for (uint32_t i = 0; i < match_length; i++) {
			*dst++ = *match_ptr++;
		}
	}

	// Return the size of the output.
	return dst - dst_0;
}