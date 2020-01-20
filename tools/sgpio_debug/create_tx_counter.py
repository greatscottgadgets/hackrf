with open('tx_counter.bin', 'wb') as f:
	f.write(bytes(range(256))*1000)
