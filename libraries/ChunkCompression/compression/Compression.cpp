#include "Compression.h"
#include "brieflzCompress.h"

using namespace std;

unsigned long compressChunks(uint8_t* src, size_t len, uint8_t* dest, size_t chunkSize, uint8_t* buff)
{
    size_t loc = 0;
    unsigned long response, outlen = 0;
    do
    {
        if(len - loc < chunkSize)
        {
            response = blz_pack(src+loc, dest+outlen, len-loc, buff);
            loc += len-loc;
        }
        else
        {
            response = blz_pack(src+loc, dest+outlen, chunkSize, buff);
            loc += chunkSize;
        }
        outlen += response;
    }while(loc < len);

    return outlen;
}