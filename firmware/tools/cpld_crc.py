#!/usr/bin/env python3

import struct

class DumbCRC32(object):
	def __init__(self):
		self._remainder = 0xffffffff
		self._reversed_polynomial = 0xedb88320
		self._final_xor = 0xffffffff

	def update(self, data):
		bit_count = len(data) * 8
		for bit_n in range(bit_count):
			bit_in = data[bit_n >> 3] & (1 << (bit_n & 7))
			self._remainder ^= 1 if bit_in != 0 else 0
			bit_out = (self._remainder & 1)
			self._remainder >>= 1;
			if bit_out != 0:
				self._remainder ^= self._reversed_polynomial;

	def digest(self):
		return self._remainder ^ self._final_xor

	def hexdigest(self):
		return '%08x' % self.digest()

class XSVFParser(object):
	def __init__(self):
		self._handlers = {
			0x00: self.XCOMPLETE   ,
			0x01: self.XTDOMASK    ,
			0x02: self.XSIR        ,
			0x03: self.XSDR        ,
			0x04: self.XRUNTEST    ,
			0x07: self.XREPEAT     ,
			0x08: self.XSDRSIZE    ,
			0x09: self.XSDRTDO     ,
			0x0a: self.XSETSDRMASKS,
			0x0b: self.XSDRINC     ,
			0x0c: self.XSDRB       ,
			0x0d: self.XSDRC       ,
			0x0e: self.XSDRE       ,
			0x0f: self.XSDRTDOB    ,
			0x10: self.XSDRTDOC    ,
			0x11: self.XSDRTDOE    ,
			0x12: self.XSTATE      ,
			0x13: self.XENDIR      ,
			0x14: self.XENDDR      ,
			0x15: self.XSIR2       ,
			0x16: self.XCOMMENT    ,
			0x17: self.XWAIT       ,
		}

	def read_byte(self):
		return self.read_bytes(1)[0]

	def read_bytes(self, n):
		c = self._f.read(n)
		if len(c) == n:
			return c
		else:
			raise RuntimeError('unexpected end of file')

	def read_bits(self, n):
		length_bytes = (n + 7) >> 3
		return self.read_bytes(length_bytes)

	def read_u32(self):
		return struct.unpack('>I', self.read_bytes(4))[0]

	def parse(self, f, debug=False):
		self._f = f
		self._debug = debug
		self._xcomplete = False
		self._xenddr = None
		self._xendir = None
		self._xruntest = 0
		self._xsdrsize = None
		self._xtdomask = None
		self._commands = []

		while self._xcomplete == False:
			self.read_instruction()

		self._f = None

		return self._commands

	def read_instruction(self):
		instruction_id = self.read_byte()
		if instruction_id in self._handlers:
			instruction_handler = self._handlers[instruction_id]
			result = instruction_handler()
			if result is not None:
				self._commands.append(result)
		else:
			raise RuntimeError('unexpected instruction 0x%02x' % instruction_id)

	def XCOMPLETE(self):
		self._xcomplete = True

	def XTDOMASK(self):
		length_bits = self._xsdrsize
		self._xtdomask = self.read_bits(length_bits)

	def XSIR(self):
		length_bits = self.read_byte()
		tdi = self.read_bits(length_bits)
		if self._debug:
			print('XSIR tdi=%d:%s' % (length_bits, tdi.hex()))
		return {
			'type': 'xsir',
			'tdi': {
				'length': length_bits,
				'data': tdi
			},
		}

	def XSDR(self):
		length_bits = self._xsdrsize
		tdi = self.read_bits(length_bits)
		if self._debug:
			print('XSDR tdi=%d:%s' % (length_bits, tdi.hex()))
		return {
			'type': 'xsdr',
			'tdi': {
				'length': length_bits,
				'data': tdi,
			},
		}

	def XRUNTEST(self):
		self._xruntest = self.read_u32()
		if self._debug:
			print('XRUNTEST number=%d' % self._xruntest)

	def XREPEAT(self):
		repeat = self.read_byte()
		# print('XREPEAT times=%d' % repeat)

	def XSDRSIZE(self):
		self._xsdrsize = self.read_u32()

	def XSDRTDO(self):
		length_bits = self._xsdrsize
		tdi = self.read_bits(length_bits)
		tdo_mask = self._xtdomask
		self._tdo_expected = (length_bits, self.read_bits(length_bits))
		wait = self._xruntest
		if wait == 0:
			end_state = self._xenddr
		else:
			end_state = 1 # Run-Test/Idle
		if self._debug:
			print('XSDRTDO tdi=%d:%s tdo_mask=%d:%s tdo_expected=%d:%s end_state=%u wait=%u' % (
				length_bits, tdi.hex(),
				length_bits, tdo_mask.hex(),
				self._tdo_expected[0], self._tdo_expected[1].hex(),
				end_state,
				wait,
			))
		return {
			'type': 'xsdrtdo',
			'tdi': {
				'length': length_bits,
				'data': tdi
			},
			'tdo_mask': {
				'length': length_bits,
				'data': tdo_mask,
			},
			'tdo_expected': {
				'length': self._tdo_expected[0],
				'data': self._tdo_expected[1],
			},
			'end_state': end_state,
			'wait': wait,
		}

	def XSETSDRMASKS(self):
		raise RuntimeError('unimplemented')

	def XSDRINC(self):
		raise RuntimeError('unimplemented')

	def XSDRB(self):
		raise RuntimeError('unimplemented')

	def XSDRC(self):
		raise RuntimeError('unimplemented')

	def XSDRE(self):
		raise RuntimeError('unimplemented')

	def XSDRTDOB(self):
		raise RuntimeError('unimplemented')

	def XSDRTDOC(self):
		raise RuntimeError('unimplemented')

	def XSDRTDOE(self):
		raise RuntimeError('unimplemented')

	def XSTATE(self):
		state = self.read_byte()
		if self._debug:
			print('XSTATE %u' % state)
		return {
			'type': 'xstate',
			'state': state,
		}

	def XENDIR(self):
		self._xendir = self.read_byte()

	def XENDDR(self):
		self._xenddr = self.read_byte()

	def XSIR2(self):
		raise RuntimeError('unimplemented')

	def XCOMMENT(self):
		raise RuntimeError('unimplemented')

	def XWAIT(self):
		wait_state = self.read_byte()
		end_state = self.read_byte()
		wait_time = self.read_u32()

#######################################################################
# Command line argument parsing.
#######################################################################

import sys

if len(sys.argv) != 2:
	print("Usage: cpld_crc.py <HackRF CPLD XSVF file)")
	sys.exit(-1)

path_xsvf = sys.argv[1]

#######################################################################
# Generic XSVF parsing phase, produces a tree of commands performed
# against the CPLD.
#######################################################################

parser = XSVFParser()
with open(path_xsvf, "rb") as f:
	commands = parser.parse(f) #, debug=True)

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
			# print('address: %02x' % address)
		elif tdi_length == 274 and end_state == 0:
			mask = int(command['tdo_mask']['data'].hex(), 16)
			expected = int(command['tdo_expected']['data'].hex(), 16)
			data[-1][-1].extend([expected, mask])
			# print('mask:%x tdo:%x' % (mask, expected))

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

if False:
	# Use a proper CRC library
	import crcmod
	crc = crcmod.predefined.Crc('crc-32')
else:
	# Use my home-grown, simple, slow CRC32 object to avoid additional
	# Python dependencies.
	crc = DumbCRC32()

for address, data, mask in data:
	valid_data = data & mask
	bytes = valid_data.to_bytes(byte_count, byteorder='little')
	crc.update(bytes)

print('0x%s' % crc.hexdigest().lower())
