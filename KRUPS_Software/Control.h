#ifndef CONTROL_H
#define CONTROL_H
/*
 * Basic control flow functions for main code
 * Functions in this header complete a simple task that happens 
 * repeatedley throughout the code base
 */

 #include "Packet.h"
 #include "Config.h"

//Appends a 16 bit interger to a buffer, also increments
//the provided location in the buffer
void append(uint8_t *buf, size_t &loc, int16_t input) {
  buf[loc++] = input;
  buf[loc++] = input >> 8;
}

/*
 * Prints a packet to the serial port, so that it may be copied 
 * onto a computer directly, avoiding sending it through comms
 */
void printPacket(Packet packet)
{
  for(uint16_t i = 0; i < packet.getLenght(); i++)
  {
    Serial.print(packet[i]);
    Serial.print(" ");
  }
  Serial.println();
  Serial.println();
}

/*
 * Reads the pwr_pin and checks if a power off signal has been sent
 */
void checkPowerOffSignal()
{
  if(analogRead(PWR_PIN) == 4095 && millis() > 20 * 1000)
  {
    while(analogRead(PWR_PIN) == 4095);
    //digitalWrite(13, LOW);
    digitalWrite(PWR_PIN, LOW);
  }
}

#endif
