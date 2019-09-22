#ifndef KRUPS_SD_H
#define KRUPS_SD_H

#include <SD.h>
#include "Packet.h"

int FlightNumber = 0;

int GetFlightNumber()
{
  int flight = 1;
  String dir = "/" + (String)flight + "/";
  while(SD.exists(dir.c_str()))
  {
    flight++;
    dir = "/" + (String)flight + "/";
  }
  Serial.println("Flight ID is:");
  Serial.println(flight);
  SD.mkdir(((String)flight).c_str());
  return flight;
}

bool InitSDCard()
{
  //Connects to SD card
  if(SD.begin(BUILTIN_SDCARD))
  {
    Serial.println("Connection to SD slot established.");
    FlightNumber = GetFlightNumber();
    return true;
  }
  else
  {
    Serial.println("It failed to connect to  the SD");
    return false;
  }
}

void StorePacket(int Pnumber, Packet p)
{
  String Flight = "";
  Flight += FlightNumber;
  Flight += "/";
  Flight += "PT_";
  Flight += Pnumber;
  Flight += ".bin";
  Serial.println(Flight);
  
  File storedData;
  storedData = SD.open(Flight.c_str(),FILE_WRITE);
  storedData.write(p.getArrayBase(), p.getLength());
  storedData.close();
  Serial.println("Stored");
}
#endif
