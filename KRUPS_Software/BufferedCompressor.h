#ifndef BUFFEREDCOMPRESSOR_H
#define BUFFEREDCOMPRESSOR_H

#include <brieflzCompress.h>
#include <stddef.h>
#include "Packet.h"

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
	bool empty; //lets the user know if any info is in the buffer or data arrays

public:
	BufferedCompressor();
	bool isFull(); //checks if data is ready to be read out
	bool isEmpty(); //return value of the isEmpty flag
	void sink(const uint8_t* data, size_t len); //fills the buffer with the provided data
	unsigned long getLen(); //returns the len of the data array
	//reads the data from the data array into a, sets len to the len of data stored, and returns the decompressed length
	unsigned long readOut(uint8_t* a, size_t& len);
	Packet readIntoPacket(); //reads the data directly into a packet;
};

#endif

uint8_t BufferedCompressor::workSpace[WORKSPACESIZE] = {};

BufferedCompressor::BufferedCompressor()
{
  bufferLoc = 0;
  dataLoc = 0;
  decompressedLength = 0;
  isReady = false;
  empty = true;
}

bool BufferedCompressor::isFull()
{
  return isReady;
}

bool BufferedCompressor::isEmpty()
{
  return empty;
}


unsigned long BufferedCompressor::getLen()
{
  return dataLoc;
}

void BufferedCompressor::checkIfFull()
{
  if(dataLoc > DATA_SIZE * .9)
  {
    isReady = true;
  }
}

void BufferedCompressor::emptyBuffer()
{
  if(bufferLoc != 0)
  {
    //compress contents of buffer into the data array
    unsigned long compressedTo = blz_pack(buffer, data+dataLoc, bufferLoc, BufferedCompressor::workSpace);
    //track total bytes compressed
    decompressedLength += bufferLoc;
    //reset buffer
    bufferLoc = 0;
    //scoot data buffer up by how much was added
    dataLoc += compressedTo;
  }
}

void BufferedCompressor::sink(const uint8_t* input, size_t len)
{
  empty = false; //data is in the compressor
  for(int i = 0; i < len; i++)
  {
    if(bufferLoc == CHUNK_SIZE) //if buffer has been filled compress and move to begining
    {
      //compress contents of buffer into the data array
      unsigned long compressedTo = blz_pack(buffer, data+dataLoc, CHUNK_SIZE, BufferedCompressor::workSpace);
      //reset buffer
      bufferLoc = 0;
      //scoot data buffer up by how much was added
      dataLoc += compressedTo;
      //track total bytes compressed
      decompressedLength += CHUNK_SIZE;
      checkIfFull(); // see if full bit should be set
    }
      // move data into the buffer
      buffer[bufferLoc] = input[i];
      bufferLoc++;
  }
}

unsigned long BufferedCompressor::readOut(uint8_t* a, size_t& len)
{
  //if the buffer is not compress the rest
  emptyBuffer();

  for(int i = 0;  i < dataLoc; i++)
  {
    a[i] = data[i];
  }
  len = dataLoc; //set the len of data copied
  unsigned long ans = decompressedLength; // save the decompressedLength
  isReady = false; //reset full bit
  //reset to initial
  bufferLoc = 0;
  dataLoc = 0;
  decompressedLength = 0;
  isReady = false;
  empty = true;

  return ans;
}

Packet BufferedCompressor::readIntoPacket()
{
  emptyBuffer();
  Packet p = Packet(decompressedLength, data, dataLoc);
  Serial.print(decompressedLength);
  Serial.print(" bytes compressed down to ");
  Serial.println(dataLoc);
  Serial.print("Compressed by ");
  Serial.print(100 - (double(decompressedLength)/dataLoc)*100);
  Serial.println("%");
 

  //reset to initial
  bufferLoc = 0;
  dataLoc = 0;
  decompressedLength = 0;
  isReady = false;
  empty = true;

  return p;
}
