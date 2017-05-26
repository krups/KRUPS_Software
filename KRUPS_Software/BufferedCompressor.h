#ifndef BUFFEREDCOMPRESSOR_H
#define BUFFEREDCOMPRESSOR_H

#include <brieflzCompress.h>
#include <cstdint>

#define WORKSPACESIZE ((1UL << 8) * 8)
#define CHUNK_SIZE (100)
#define DATA_SIZE (1958)

using namespace std;

class BufferedCompressor
{
private:
	static uint8_t workSpace[WORKSPACESIZE]; //generic workspace used by brieflz
	uint8_t buffer[CHUNK_SIZE]; //holds data read in to be compressed
	uint8_t data[DATA_SIZE]; //compressed data
	size_t bufferLoc, dataLoc; //pos in the two arrays
	bool isReady; //flag to let users know data is filled above threshold
	void checkIfFull(); //sees if the full bit should be set
	void emptyBuffer(); //finshes up compression by empyting the rest of the buffer
	unsigned long decompressedLength; //size when decompressed

public:
	BufferedCompressor();
	bool isFull(); //checks if data is ready to be read out
	void sink(const uint8_t* data, size_t len); //fills the buffer with the provided data
	unsigned long getLen(); //returns the len of the data array
	//reads the data from the data array into a, sets len to the len of data stored, and returns the decompressed length
	unsigned long readOut(uint8_t* a, size_t& len);
	Packet readIntoPacket(); //reads the data directly into a packet;
};

#endif
