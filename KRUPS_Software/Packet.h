#ifndef PACKET_H
#define PACKET_H


#define PACKET_SIZE (1960) //size of a packets to send
#define HEADER_SIZE (2)  //number of bytes taken up in the packet by buffer

#include<compress.h>

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

    length = len;
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
