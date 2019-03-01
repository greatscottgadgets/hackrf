#!/usr/bin/env python3

# Xilinx CoolRunner II XC2C64A characteristics

bits_of_address = 7
bits_of_data = 274
bytes_of_data = (bits_of_data + 7) // 8
bits_in_program_row = bits_of_address + bits_of_data

def values_list_line_wrap(values):
	line_length = 16
	return [' '.join(values[n:n+line_length]) for n in range(0, len(values), line_length)]

def dec_lines(bytes):
	return values_list_line_wrap(['%d,' % n for n in bytes])

def hex_lines(bytes):
	return values_list_line_wrap(['0x%02x,' % n for n in bytes])

def reverse_bits(n, bit_count):
	byte_count = (bit_count + 7) >> 3
	# n = int(bytes.hex(), 16)
	n_bits = bin(n)[2:].zfill(bit_count)
	n_bits_reversed = n_bits[::-1]
	n_reversed = int(n_bits_reversed, 2)
	return n_reversed.to_bytes(byte_count, byteorder='little')

def extract_addresses(block):
	return tuple([row['address'] for row in block])

def extract_data(block):
	return tuple([row['data'] for row in block])

def extract_mask(block):
	return tuple([row['mask'] for row in block])

def equal_blocks(block1, block2, mask):
	block1_data = extract_data(block1)
	block2_data = extract_data(block2)
	assert(len(block1_data) == len(block2_data))
	assert(len(block1_data) == len(mask))
	for row1, row2, mask in zip(block1_data, block2_data, mask):
		differences = (row1 ^ row2) & mask
		if differences != 0:
			return False
	return True

def dump_block(rows, endian='little'):
	data_bytes = (bits_of_data + 7) >> 3
	for row in rows:
		print('%02x %s' % (row['address'], row['data'].to_bytes(data_bytes, byteorder=endian).hex()))

def extract_programming_data(commands):
	ir_map = {
		0x01: 'idcode',
		0xc0: 'conld',
		0xe8: 'enable',
		0xea: 'program',
		0xed: 'erase',
		0xee: 'verify',
		0xf0: 'init',
		0xff: 'bypass',
		# Other instructions unimplemented and if encountered, will cause tool to crash.
	}

	ir = None
	program = []
	verify = []
	for command in commands:
		if command['type'] == 'xsir':
			ir = ir_map[command['tdi']['data'][0]]
			if ir == 'program':
				program.append([])
			if ir == 'verify':
				verify.append([])
		elif ir == 'verify' and command['type'] == 'xsdrtdo':
			tdi_length = command['tdi']['length']
			end_state = command['end_state']
			if tdi_length == bits_of_address and end_state == 1:
				address = int(command['tdi']['data'].hex(), 16)
				verify[-1].append({'address': address})
			elif tdi_length == bits_of_data and end_state == 0:
				mask = int(command['tdo_mask']['data'].hex(), 16)
				expected = int(command['tdo_expected']['data'].hex(), 16)
				verify[-1][-1]['data'] = expected
				verify[-1][-1]['mask'] = mask
		elif ir == 'program' and command['type'] == 'xsdrtdo':
			tdi_length = command['tdi']['length']
			end_state = command['end_state']
			if tdi_length == bits_in_program_row and end_state == 0:
				tdi = int(command['tdi']['data'].hex(), 16)
				address = (tdi >> bits_of_data) & ((1 << bits_of_address) - 1)
				data = tdi & ((1 << bits_of_data) - 1)
				program[-1].append({
					'address': address,
					'data': data
				})

	return {
		'program': program,
		'verify': verify,
	}

def validate_programming_data(programming_data):
	expected_address_sequence = (0x00, 0x40, 0x60, 0x20, 0x30, 0x70, 0x50, 0x10, 0x18, 0x58, 0x78, 0x38, 0x28, 0x68, 0x48, 0x08, 0x0c, 0x4c, 0x6c, 0x2c, 0x3c, 0x7c, 0x5c, 0x1c, 0x14, 0x54, 0x74, 0x34, 0x24, 0x64, 0x44, 0x04, 0x06, 0x46, 0x66, 0x26, 0x36, 0x76, 0x56, 0x16, 0x1e, 0x5e, 0x7e, 0x3e, 0x2e, 0x6e, 0x4e, 0x0e, 0x0a, 0x4a, 0x6a, 0x2a, 0x3a, 0x7a, 0x5a, 0x1a, 0x12, 0x52, 0x72, 0x32, 0x22, 0x62, 0x42, 0x02, 0x03, 0x43, 0x63, 0x23, 0x33, 0x73, 0x53, 0x13, 0x1b, 0x5b, 0x7b, 0x3b, 0x2b, 0x6b, 0x4b, 0x0b, 0x0f, 0x4f, 0x6f, 0x2f, 0x3f, 0x7f, 0x5f, 0x1f, 0x17, 0x57, 0x77, 0x37, 0x27, 0x67, 0x47, 0x07, 0x05, 0x45,)

	# Validate program blocks:

	# There should be two extracted program blocks. The first contains the
	# the bitstream with done bit(s) not asserted. The second updates the
	# "done" bit(s) to finish the process.
	assert(len(programming_data['program']) == 2)

	# First program phase writes the bitstream to flash (or SRAM) with
	# special bit(s) not asserted, so the bitstream is not yet valid.
	assert(extract_addresses(programming_data['program'][0]) == expected_address_sequence)

	# Second program phase updates a single row to finish the programming
	# process.
	assert(len(programming_data['program'][1]) == 1)
	assert(programming_data['program'][1][0]['address'] == 0x05)

	# Validate verify blocks:

	# There should be two extracted verify blocks.
	assert(len(programming_data['verify']) == 2)

	# The two verify blocks should match.
	assert(programming_data['verify'][0] == programming_data['verify'][1])

	# Check the row address order of the second verify block.
	assert(extract_addresses(programming_data['verify'][0]) == expected_address_sequence)
	assert(extract_addresses(programming_data['verify'][1]) == expected_address_sequence)

	# Checks across programming and verification:

	# Check that program data matches data expected during verification.
	assert(equal_blocks(programming_data['program'][0], programming_data['verify'][0], extract_mask(programming_data['verify'][0])))
	assert(equal_blocks(programming_data['program'][0], programming_data['verify'][1], extract_mask(programming_data['verify'][1])))

def make_sram_program(program_blocks):
	program_sram = list(program_blocks[0])
	program_sram[-2] = program_blocks[1][0]
	return program_sram

#######################################################################
# Command line argument parsing.
#######################################################################

import argparse

parser = argparse.ArgumentParser()
action_group = parser.add_mutually_exclusive_group(required=True)
action_group.add_argument('--checksum', action='store_true', help='Calculate bitstream read-back CRC32 value')
action_group.add_argument('--code', action='store_true', help='Generate C code for bitstream loading/programming/verification')
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

programming_data = extract_programming_data(commands)
validate_programming_data(programming_data)

#######################################################################
# Patch the second programming phase into the first for SRAM
# programming.
#######################################################################

verify_blocks = programming_data['verify']
program_blocks = programming_data['program']

#######################################################################
# Calculate CRC of data read from CPLD during the second verification
# pass, which is after the "done" bit is set. Mask off insignificant
# bits (turning them to zero) and extending rows to the next full byte.
#######################################################################

if args.checksum:
	if args.crcmod:
		# Use a proper CRC library
		import crcmod
		crc = crcmod.predefined.Crc('crc-32')
	else:
		# Use my home-grown, simple, slow CRC32 object to avoid additional
		# Python dependencies.
		from dumb_crc32 import DumbCRC32
		crc = DumbCRC32()

	verify_block = verify_blocks[1]
	for address, data, mask in verify_block:
		valid_data = data & mask
		bytes = valid_data.to_bytes(bytes_of_data, byteorder='little')
		crc.update(bytes)

	print('0x%s' % crc.hexdigest().lower())

if args.code:
	program_sram = make_sram_program(program_blocks)
	verify_block = verify_blocks[1]
	verify_masks = tuple(frozenset(extract_mask(verify_block)))
	verify_mask_index = dict([(k, v) for v, k in enumerate(verify_masks)])
	verify_mask_row_index = [verify_mask_index[row['mask']] for row in verify_block]

	result = []
	result.extend((
		'/* WARNING: Auto-generated file. Do not edit. */',
		'',
		'#include <cpld_xc2c.h>',
		'',
		'const cpld_xc2c64a_program_t cpld_hackrf_program_sram = { {',
	))
	data_lines = [', '.join(['0x%02x' % n for n in row['data'].to_bytes(bytes_of_data, byteorder='little')]) for row in program_sram]
	result.extend(['\t{ { %s } },' % line for line in data_lines])
	result.extend((
		'} };',
		'',
		'const cpld_xc2c64a_verify_t cpld_hackrf_verify = {',
		'\t.mask = {',
	))
	verify_mask_lines = [', '.join(['0x%02x' % n for n in mask.to_bytes(bytes_of_data, byteorder='little')]) for mask in verify_masks]
	result.extend(['\t\t{ { %s } },' % line for line in verify_mask_lines])
	result.extend((
		'\t},'
		'\t.mask_index = {',
	))
	result.extend(['\t\t%s' % line for line in dec_lines(verify_mask_row_index)])
	result.extend((
		'\t}',
		'};',
		'',
	))
	print('\n'.join(result))
