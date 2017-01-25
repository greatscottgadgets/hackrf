import os.path
import struct

with open("_header.bin", "wb") as f:
    x = struct.pack('<H', os.path.getsize('hackrf_usb_dfu.bin') // 512 + 1)
    y = [0xda, 0xff, x[0], x[1], 0xff, 0xff, 0xff, 0xff]
    f.write(bytearray(y))
