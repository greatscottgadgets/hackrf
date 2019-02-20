import os.path
import struct
import sys

with open("_header.bin", "wb") as f:
    x = struct.pack('<H', os.path.getsize(sys.argv[1] + '_dfu.bin') // 512 + 1)
    y = [0xda, 0xff, x[0], x[1]] + [0xff] * 12
    f.write(bytearray(y))
