#!/usr/bin/env python2
# vim: set ts=4 sw=4 tw=0 et pm=:
import struct
import sys
import os.path
import getopt
import zlib

options, remainder = getopt.getopt(sys.argv[1:], 'p:v:d:S:v', ['pid=',
														'vid=',
														'did=',
														'spec=',
														'verbose',
														])

pid = 0x000c
vid = 0x1fc9
did = 0
spec = 0x0100
verbose = False

for opt, arg in options:
	if opt in ('-p', '--pid'):
		pid = int(arg)
	if opt in ('-v', '--vid'):
		vid = int(arg)
	if opt in ('-d', '--did'):
		did = int(arg)
	if opt in ('-S', '--spec'):
		spec = int(arg)
	elif opt in ('-v', '--verbose'):
		verbose = True

if len(remainder)<1:
	in_file = "/dev/stdin"
else:
	in_file = remainder[0]

if len(remainder)<2:
	out = open("/dev/stdout","wb")
else:
	out = open(remainder[1],"wb")

# ref. NXP UM10503 Table 24 (Boot image header description)
header = ""
header += struct.pack ('<B',int("11"+"011010",2)) # AES enc not active + No hash active
header += struct.pack ('<B',int("11"+"111111",2)) # RESERVED + AES_CONTROL
size=os.path.getsize(in_file)
size=(size+511)/512 # 512 byte blocks, rounded up
header += struct.pack('<H',size)                  # (badly named) HASH_SIZE
header += struct.pack('8B',*[0xff] *8)            # HASH_VALUE (unused)
header += struct.pack('4B',*[0xff] *4)            # RESERVED

out.write( header )

infile=open(in_file,"rb").read()
out.write( infile )

suffix= ""
suffix+= struct.pack('<H', did)  # bcdDevice
suffix+= struct.pack('<H', pid)  # idProduct
suffix+= struct.pack('<H', vid)  # idVendor
suffix+= struct.pack('<H', spec) # bcdDFU
suffix+= b'DFU'[::-1]            # (reverse DFU)
suffix+= struct.pack('<B', 16)   # suffix length

out.write( suffix )

checksum=zlib.crc32(header+infile+suffix) & 0xffffffff ^ 0xffffffff
out.write( struct.pack('I', checksum) ) # crc32
