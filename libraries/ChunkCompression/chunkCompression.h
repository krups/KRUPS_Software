#ifndef CHUNKCOMPRESSION_H
#define CHUNKCOMPRESSION_H

#include <cstdint>
#include "brieflzCompress.h"
#include "brieflzDecompress.h"

using namespace std;

unsigned long depack_count(const void *src, void *dst, unsigned long depacked_size);
unsigned long decompressChunks(uint8_t* src, size_t len, uint8_t* dest, size_t chunkSize, unsigned long decompressedLen);
unsigned long compressChunks(uint8_t* src, size_t len, uint8_t* dest, size_t chunkSize, uint8_t* buff);

#endif