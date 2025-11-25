#!/usr/bin/env python3
#
# This file is part of HackRF.
#
# Copyright (c) 2025 Great Scott Gadgets <info@greatscottgadgets.com>
# SPDX-License-Identifier: BSD-3-Clause

import lz4.block
import struct
import os

from board                  import PralinePlatform
from top.standard           import Top as standard
from top.half_precision     import Top as half_precision
from top.ext_precision_rx   import Top as ext_precision_rx
from top.ext_precision_tx   import Top as ext_precision_tx


BLOCK_SIZE = 4096  # 4 KiB blocks
OUTPUT_FILE = "praline_fpga.bin"

def compress_blockwise(input_stream, output_stream):
    # For every block...
    while block := f_in.read(BLOCK_SIZE):

        # Compress the block using raw LZ4 block compression.
        compressed_block = lz4.block.compress(block, store_size=False, 
            mode="high_compression", compression=12)

        # Write the compressed block length and its contents to the output file.
        f_out.write(struct.pack("<H", len(compressed_block)))
        f_out.write(compressed_block)
    
    # Write end marker (block of size 0)
    f_out.write(struct.pack("<H", 0))


if __name__ == "__main__":

    fpga_images = {
        "0_standard":   standard(),
        "1_halfprec":   half_precision(),
        "2_extprec_rx": ext_precision_rx(),
        "3_extprec_tx": ext_precision_tx(),
    }

    # Build bitstreams first.
    for name, image in fpga_images.items():
        PralinePlatform().build(image, name=name)

    # Pack all the bitstreams.
    with open(f"build/{OUTPUT_FILE}", "wb") as f_out:
        f_out.write(struct.pack("<I", len(fpga_images)))  # number of bitstreams
        f_out.seek(4 * len(fpga_images), os.SEEK_CUR)     # reserve 4-byte slots for offsets
        offsets = []

        # Write compressed bitstreams in our custom raw LZ4 block format.
        for name, image in fpga_images.items():
            img_path = f"build/{name}.bin"
            offsets.append(f_out.tell())
            with open(img_path, 'rb') as f_in:
                compress_blockwise(f_in, f_out)

        # Write offsets table, right after the number of bitstreams.
        f_out.seek(4, os.SEEK_SET)
        for offset in offsets:
            f_out.write(struct.pack("<I", offset))

