#include "brieflzDecompress.h"
#include "ChunkCompression.h"


unsigned long depack_count(const void *src, void *dst, unsigned long depacked_size)
{
    const unsigned long GIVEN = 999999999;
    unsigned long unused = blz_depack_used_count(src, GIVEN, dst, depacked_size);
    return GIVEN - unused;
}



unsigned long decompressChunks(uint8_t* src, size_t len, uint8_t* dest, size_t chunkSize, unsigned long decompressedLen)
{
	int accum = 0;
    int loc = 0;
	do
    {
        if(decompressedLen - accum < chunkSize)
        {
            unsigned long response = depack_count(src+loc, dest+accum, decompressedLen-accum);
            loc += response;
            accum += decompressedLen - accum;
        }
        else
        {
            unsigned long response = depack_count(src+loc, dest+accum, chunkSize);
            loc += response;
            accum += chunkSize;
        }
    }while(loc < len);
    if(accum != decompressedLen)
    	return -1;
    return loc;
}