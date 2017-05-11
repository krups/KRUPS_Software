/*
Header to redefine critical funcitons to provide dummy data
to allow debugging and testing on the main software

Author: Collin Dietz
Email: c4dietz@gmail.com
Date: 5/11/17
*/


#ifndef DEBUG_H
#define DEBUG_H

#define TC_NUM (12)

extern volatile int16_t num_packets;

//Fake read funcitons for testing 
void Read_gyro(uint8_t *buf, size_t &loc)
{
    append(buf, loc, 0+num_packets);
    append(buf, loc, 1+num_packets);
    append(buf, loc, 2+num_packets);
}

void Read_loaccel(uint8_t *buf, size_t &loc)
{
    append(buf, loc, 10+num_packets);
    append(buf, loc, 11+num_packets);
    append(buf, loc, 12+num_packets);
}

void Read_mag(uint8_t *buf, size_t &loc)
{
    append(buf, loc, 20+num_packets);
    append(buf, loc, 21+num_packets);
    append(buf, loc, 22+num_packets);
}

void Read_TC(uint8_t *buf, size_t &loc)
{
  for(int i = 0; i < TC_NUM; i++)
  {
    append(buf, loc, i*7 + num_packets);
  }
  delay(TC_NUM/3 * 100);
}

//empty funtions to allow compilation, do not effect testing
//and do not need to execute
void init_Sensors() {}
void init_TC() {}
void init_accel_interrupt(float a, float b, int c) {}
void init_gyro_interrupt(int a, int b, int c) {}

#endif