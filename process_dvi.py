import struct

control_tokens = [0b1101010100, 0b0010101011, 0b0101010100, 0b1010101011]

def find_control_position(raw_words):
	final_control_position = -1
	control_position = -1
	control_counter = 0
	for w2, w1 in zip(raw_words, raw_words[1:]):
		lw = w2 | (w1 << 10)
		found_control = False
		for i in range(10):
			e = (lw >> i) & (2**10 - 1)
			if e in control_tokens:
				found_control = True
				if control_position == i:
					control_counter += 1
				else:
					control_position = i
					control_counter = 1
		if not found_control:
			control_position = -1
			control_counter = 0
		if control_counter > 127 and control_position != final_control_position:
			print("control: {0} -> {1}".format(final_control_position, control_position))
			final_control_position = control_position
	return final_control_position

def char_align(raw_words, control_position):
	r = []
	for w2, w1 in zip(raw_words, raw_words[1:]):
		lw = w2 | (w1 << 10)
		e = (lw >> control_position) & (2**10 - 1)
		r.append(e)
	return r

def bit(i, n):
	return (i >> n) & 1

def decode_8b10b(b):
	try:
		c = control_tokens.index(b)
		de = False
	except ValueError:
		c = 0
		de = True
	vsync = bool(c & 2)
	hsync = bool(c & 1)

	value = bit(b, 0) ^ bit(b, 9)
	for i in range(1, 8):
		value |= (bit(b, i) ^ bit(b, i-1) ^ (~bit(b, 8) & 1)) << i

	return de, hsync, vsync, value

def write_image(filename, img):
	f = open(filename, "wb")
	f.write(bytes("P6\n{0} {1}\n255\n".format(len(img[0]), len(img)), "ASCII"))
	for row in img:
		for col in row:
			v = struct.pack("B", col)
			f.write(v*3)
	f.close()

def dma_workaround(l):
	r = []
	it = iter(l)
	while True:
		try:
			r += reversed([next(it) for i in range(8)])
		except StopIteration:
			return r

def main():
	raw_words = []
	with open("raw_dvi", "rb") as f:
		word = f.read(2)
		while word:
			raw_words.append(struct.unpack(">H", word)[0])
			word = f.read(2)
	raw_words = dma_workaround(raw_words)

	control_position = find_control_position(raw_words)
	print("Syncing characters at {0}".format(control_position))
	chars = char_align(raw_words, control_position)
	print("8b10b decoding")
	chars_decoded = [decode_8b10b(char) for char in chars]

	print("Generating output image")
	img = []
	x = []
	prev_de = False
	for de, hsync, vsync, value in chars_decoded:
		if de:
			x.append(value)
		else:
			if prev_de:
				img.append(x)
			x = []
		prev_de = de
	for row in img:
		l = len(row)
		if l != 640:
			print("Row of unexpected length {0}".format(l))
	img = [row for row in img if len(row) == 640]
	write_image("tst.ppm", img)

main()
