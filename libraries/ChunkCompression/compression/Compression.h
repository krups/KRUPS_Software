#ifndef COMPRESSION_H
#define COMPRESSION_H

#include <cstdint>
#include "brieflzCompress.h"


using namespace std;

unsigned long compressChunks(uint8_t* src, size_t len, uint8_t* dest, size_t chunkSize, uint8_t* buff);


#endif