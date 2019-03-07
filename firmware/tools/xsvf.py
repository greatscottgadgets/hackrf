
import struct

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

	def tdomask(self):
		return self._xtdomask
		
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
