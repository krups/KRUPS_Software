// Wrapper function for Heatshrink encoder and decoder
// 96 byte input buffer in heatshrink header suggested

#ifndef COMPRESS_H
#define COMPRESS_H

union ftoh {
	float f;
	uint8_t b[4];
};

union stoe {
	int16_t s;
	int8_t e[2];
};

void append(uint8_t *buf, size_t &loc, int16_t input) {
	buf[loc++] = input;
	buf[loc++] = input >> 8;
}

#endif
