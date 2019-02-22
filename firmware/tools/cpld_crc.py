#!/usr/bin/env python3

#######################################################################
# Command line argument parsing.
#######################################################################

import argparse

parser = argparse.ArgumentParser()
parser.add_argument('--crcmod', action='store_true', help='Use Python crcmod library instead of built-in CRC32 code')
parser.add_argument('--debug', action='store_true', help='Enable debug output')
parser.add_argument('hackrf_xc2c_cpld_xsvf', type=str, help='HackRF Xilinx XC2C64A CPLD XSVF file containing erase/program/verify phases')
args = parser.parse_args()

#######################################################################
# Generic XSVF parsing phase, produces a tree of commands performed
# against the CPLD.
#######################################################################

with open(args.hackrf_xc2c_cpld_xsvf, "rb") as f:
	from xsvf import XSVFParser
	commands = XSVFParser().parse(f, debug=args.debug)

#######################################################################
# Extraction of verify row addresses and data/masks.
#######################################################################

ir_map = {
	0x01: 'idcode',
	0xc0: 'conld',
	0xe8: 'enable',
	0xea: 'program',
	0xed: 'erase',
	0xee: 'verify',
	0xf0: 'init',
	0xff: 'bypass',
}

ir = None
data = []
for command in commands:
	if command['type'] == 'xsir':
		ir = ir_map[command['tdi']['data'][0]]
		if ir == 'verify':
			data.append([])
	elif ir == 'verify' and command['type'] == 'xsdrtdo':
		tdi_length = command['tdi']['length']
		end_state = command['end_state']
		if tdi_length == 7 and end_state == 1:
			address = int(command['tdi']['data'].hex(), 16)
			data[-1].append([address])
		elif tdi_length == 274 and end_state == 0:
			mask = int(command['tdo_mask']['data'].hex(), 16)
			expected = int(command['tdo_expected']['data'].hex(), 16)
			data[-1][-1].extend([expected, mask])

#######################################################################
# Check that extracted data conforms to expectations.
#######################################################################

# There should two extracted verify blocks.
assert(len(data) == 2)

# Check the row address order of the second verify block.
address_sequence = tuple([row[0] for row in data[1]])
expected_address_sequence = (0x00, 0x40, 0x60, 0x20, 0x30, 0x70, 0x50, 0x10, 0x18, 0x58, 0x78, 0x38, 0x28, 0x68, 0x48, 0x08, 0x0c, 0x4c, 0x6c, 0x2c, 0x3c, 0x7c, 0x5c, 0x1c, 0x14, 0x54, 0x74, 0x34, 0x24, 0x64, 0x44, 0x04, 0x06, 0x46, 0x66, 0x26, 0x36, 0x76, 0x56, 0x16, 0x1e, 0x5e, 0x7e, 0x3e, 0x2e, 0x6e, 0x4e, 0x0e, 0x0a, 0x4a, 0x6a, 0x2a, 0x3a, 0x7a, 0x5a, 0x1a, 0x12, 0x52, 0x72, 0x32, 0x22, 0x62, 0x42, 0x02, 0x03, 0x43, 0x63, 0x23, 0x33, 0x73, 0x53, 0x13, 0x1b, 0x5b, 0x7b, 0x3b, 0x2b, 0x6b, 0x4b, 0x0b, 0x0f, 0x4f, 0x6f, 0x2f, 0x3f, 0x7f, 0x5f, 0x1f, 0x17, 0x57, 0x77, 0x37, 0x27, 0x67, 0x47, 0x07, 0x05, 0x45,)
assert(address_sequence == expected_address_sequence)

#######################################################################
# Calculate CRC of data read from CPLD during the second verification
# pass, which is after the "done" bit is set. Mask off insignificant
# bits (turning them to zero) and extending rows to the next full byte.
#######################################################################

data = data[1]
byte_count = (274 + 7) // 8

if args.crcmod:
	# Use a proper CRC library
	import crcmod
	crc = crcmod.predefined.Crc('crc-32')
else:
	# Use my home-grown, simple, slow CRC32 object to avoid additional
	# Python dependencies.
	from dumb_crc32 import DumbCRC32
	crc = DumbCRC32()

for address, data, mask in data:
	valid_data = data & mask
	bytes = valid_data.to_bytes(byte_count, byteorder='little')
	crc.update(bytes)

print('0x%s' % crc.hexdigest().lower())
