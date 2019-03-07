
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
