
#ifndef DEBUG_H
#define DEBUG_H

//Pinout definitions for sensors
#define PINMUX0		(16)
#define PINMUX1		(17)
#define PINCS1		(20)
#define PINCS2		(21)
#define PINCS3		(9)
#define PINCS4		(10)
#define PINSO		(12)
#define CLK			(13)

#include"Packet.h"
#include"Control.h"

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
  append(buf, loc, 20 + 3*mux + 0);
  append(buf, loc, 20 + 3*mux + 1);
  append(buf, loc, 20 + 3*mux + 2);
  append(buf, loc, 20 + 3*mux + 3);
}

//empty funtions to allow compilation, do not effect testing
//and do not need to execute
void init_Sensors() {}
void init_TC() {


  pinMode(PINMUX0, OUTPUT);
  pinMode(PINMUX1, OUTPUT);
  pinMode(PINCS1, OUTPUT);
  pinMode(PINCS2, OUTPUT);
  pinMode(PINCS3, OUTPUT);
  pinMode(PINCS4, OUTPUT);
  
  digitalWrite(PINMUX0, LOW);       // low, low = channel 1
  digitalWrite(PINMUX1, LOW);
  //SPI.begin();
  }
void init_accel_interrupt(float a, float b, int c) {}
void init_gyro_interrupt(int a, int b, int c) {}

void initSensors() {}

void readGyro(uint8_t *buf, size_t &loc)
{
    int t = int(millis()/1000);
    append(buf, loc, t);
    append(buf, loc, 2*t);
    append(buf, loc, 3*t);
}

void readAccel(uint8_t *buf, size_t &loc)
{
    int t = int(millis()/1000);
    append(buf, loc, t*t);
    append(buf, loc, 2*t*t);
    append(buf, loc, 3*t*t);
}

void readMag(uint8_t *buf, size_t &loc)
{
    append(buf, loc, 7);
    append(buf, loc, 5);
    append(buf, loc, 3);
}

/*
Grabs current time in millis and saves it as a three byte number
in the buffer
*/
void save_time(uint8_t* buff, size_t& loc)
{
  long time = millis();
  buff[loc++] = time & 0xFF; //low byte
  buff[loc++] = time >> 8; //mid byte
  buff[loc++] = time >> 16; //top byte
}


#endif	