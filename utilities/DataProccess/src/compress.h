// Wrapper function for Heatshrink encoder and decoder
// 96 byte input buffer in heatshrink header suggested

#ifndef COMPRESS_H
#define COMPRESS_H

#include <cstdint>

using namespace std;

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

// used to translate half float to to full float
float half_float(uint8_t *buf) {
	ftoh temp; temp.b[2] = buf[0]; temp.b[3] = buf[1];
	temp.b[0] = 0; temp.b[1] = 0;
	return temp.f;
}

// used to translate buffered uint8_t to int16_t
int16_t buf_int(uint8_t *buf) {
	stoe temp; temp.e[0] = buf[0]; temp.e[1] = buf[1];
	return temp.s;
}
#endif
