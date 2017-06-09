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

#include"Packet.h"
#include<compress.h>

extern int16_t measure_reads;
extern int16_t num_packets;
int mux = 0;

//Fake read funcitons for testing 
void Read_gyro(uint8_t *buf, size_t &loc)
{
    int t = int(millis()/1000);
    append(buf, loc, t);
    append(buf, loc, 2*t);
    append(buf, loc, 3*t);
}

void Read_loaccel(uint8_t *buf, size_t &loc)
{
    int t = int(millis()/1000);
    append(buf, loc, t*t);
    append(buf, loc, 2*t*t);
    append(buf, loc, 3*t*t);
}

void Read_hiaccel(uint8_t *buf, size_t &loc)
{
    int t = int(millis()/1000);
    append(buf, loc, t+5);
    append(buf, loc, t+6);
    append(buf, loc, t+7);
}

void Read_mag(uint8_t *buf, size_t &loc)
{
    append(buf, loc, 7);
    append(buf, loc, 5);
    append(buf, loc, 3);
}

void Read_temp(uint8_t *buf, size_t &loc)
{
    static int timesRead = 0;
    append(buf, loc, timesRead);
    timesRead++;
}

void setMUX(int j)
{
  mux = j;
}

void Read_TC_at_MUX(uint8_t *buf, size_t &loc)
{
  append(buf, loc, 240 + 3*mux + 0);
  append(buf, loc, 240 + 3*mux + 1);
  append(buf, loc, 240 + 3*mux + 2);
}

void printPacket(Packet packet, int32_t len)
{
  for(int i = 0; i < len; i++)
  {
    Serial.print(packet[i]);
    Serial.print(" ");
  }
  Serial.println();
  Serial.println();
}

//empty funtions to allow compilation, do not effect testing
//and do not need to execute
void init_Sensors() {}
void init_TC() {}
void init_accel_interrupt(float a, float b, int c) {}
void init_gyro_interrupt(int a, int b, int c) {}

#endif
