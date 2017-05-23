// Wrapper function for Heatshrink encoder and decoder
// 96 byte input buffer in heatshrink header suggested

#ifndef COMPRESS_H
#define COMPRESS_H

#include <heatshrink_encoder.h>
#include <heatshrink_decoder.h>

heatshrink_encoder heatshrink;
heatshrink_decoder heatexpand;

union ftoh {
	float f;
	uint8_t b[4];
};

union stoe {
	int16_t s;
	int8_t e[2];
};

// Takes an input array of bits, and the length of the array and outputs an array that
// has been compressed using the heatshrink compression library, and it's length
void compress(uint8_t *input_buffer, size_t len, uint8_t *output_buffer, size_t &outlen) {
    // Reset a global heatshrink encoder object
    heatshrink_encoder_reset(&heatshrink);

    size_t input_size, output_size, accum=0;
    outlen = 0;

    do { // Push the input into Heatshrink encoder state machine
		heatshrink_encoder_sink(&heatshrink, input_buffer+accum, len-accum, &input_size);
		heatshrink_encoder_poll(&heatshrink, output_buffer+outlen, 52, &output_size);
        accum += input_size; outlen += output_size;
    } while(accum < len);

    // Ensure all output has been retrieved
    while(HSER_FINISH_MORE == heatshrink_encoder_finish(&heatshrink) ) {
        heatshrink_encoder_poll(&heatshrink, output_buffer+outlen, 52, &output_size);
        outlen += output_size;
    }
}

// first half of compression function, use to sink measurements as they are taken
void compressSensor(uint8_t *input_buffer, size_t len, uint8_t *output_buffer, size_t &outlen) {
	size_t input_size, output_size, accum = 0;

	do {
	heatshrink_encoder_sink(&heatshrink, input_buffer+accum, len-accum, &input_size);
	heatshrink_encoder_poll(&heatshrink, output_buffer+outlen, 52, &output_size);
	accum += intput_size; outlen += output_size;
	} while(accum < len);
}

// second half of compression function, polls all data from the state machine and
// resets after finishing the compression
void compressFinish(uint8_t *output_buffer, size_t &outlen) {
	size_t output_size;

	while(HSER_FINISH_MORE == heatshrink_encoder_finish(&heatshrink) ) {
		heatshrink_encoder_poll(&heatshrink, output_buffer+outlen, 52, &output_size);
		outlen += output_size;
	}
	heatshrink_encoder_reset(&heatshrink);
}

// Takes a previously compressed array of bits, and the array's length and decompresses the array
// using the heatshrink compression library and outputs the array and it's length
void decompress(uint8_t *input_buffer, size_t len, uint8_t *output_buffer, size_t &outlen) {
    // Reset a global heatshrink decoder object
    heatshrink_decoder_reset(&heatexpand);
    size_t input_size, output_size, accum=0;
    outlen = 0;

    do { // Push the input into Heatshrink encoder state machine
        heatshrink_decoder_sink(&heatexpand, input_buffer+accum, len-accum, &input_size);
        heatshrink_decoder_poll(&heatexpand, output_buffer+outlen, 52, &output_size);
        accum += input_size; outlen += output_size;
    } while(accum < len);

    // Ensure all output has been retrieved
    while(HSDR_FINISH_MORE == heatshrink_decoder_finish(&heatexpand) ) {
        heatshrink_decoder_poll(&heatexpand, output_buffer+outlen, 52, &output_size);
        outlen += output_size;
    }
}


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
