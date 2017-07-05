#ifndef PACKET_H
#define PACKET_H

#include "compress.h"

#define PACKET_SIZE (1960) //size of a packets to send
#define HEADER_SIZE (13)  //number of bytes taken up in the packet by buffer

class Packet
{
public:

  Packet()
  {
    for(int i = 0; i < PACKET_SIZE; i++)
    {
      data[i] = 0;
    }
  }
  
	//creates a packet and loads data
	Packet(int16_t number, uint8_t a[], int16_t len)
	{
		size_t loc = 0;
		append(data, loc, number);

		for(int i = loc; i < len; i++)
		{
			data[i] = a[i - loc];
		}
		this->len = len;
	}

	//creates a packet and loads data while converting char data
	Packet(char a[], int16_t length)
	{
		for(int i = 0; i < length; i++)
		{
			data[i] = charToUint8(a[i]);
		}
		len = length;
	}

	//overides the access operator to get data
	uint8_t operator[](int i)
	{
		return data[i];
	}

	int16_t getLen()
	{
		return len;
	}

	uint8_t* getArrayAt(int i)
	{
		return &data[i];
	}
	
private:
	uint8_t data[PACKET_SIZE];
	int16_t len;

	union ctouint
	{
		char c;
		uint8_t u;
	};

	uint8_t charToUint8(char c)
	{
		ctouint a;
		a.c = c;
		return a.u;
	}
};


#endif
