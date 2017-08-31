#ifndef PACKET_H
#define PACKET_H

#include"Control.h"

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
		append(data, loc, number); //append decompress length to the front of the packet
    for(loc; loc < HEADER_SIZE; loc++) //init the debug data slots to 0
    {
      data[loc] = 0;
    }
		for(int i = 0; i < len; i++) //copy over the data
		{
			data[loc+i] = a[i];
		}
   loc += len;

    length = loc;
	}

  uint8_t* getArrayBase()
  {
    return data;
  }

  uint8_t* getArrayAt(int i)
  {
    return &data[i];
  }

  uint16_t getLength()
  {
    return length;
  }

	//overides the access operator to get data
	uint8_t operator[](int i)
	{
		return data[i];
	}

private:
	uint8_t data[PACKET_SIZE];
  uint16_t length;
};


#endif
