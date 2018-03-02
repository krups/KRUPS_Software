#include<SerialFlash.h>
#include<SPI.h>
#include"Packet.h"
#include"Config.h"
#include"Control.h"

#define FLASH_PIN (14)

uint8_t flight_counter = 10;

//reads in the currrent flight number
void init_flight_counter()
{
  Serial.println("starting");
  uint8_t buff[] = {0};
  bool doesExist = SerialFlash.exists("log");
  Serial.println("Does exist: ");
  Serial.println(doesExist);
  SerialFlashFile file = SerialFlash.open("log");
  if(!file)
  {
    Serial.println("making log");
    //log does not exist
    bool made = SerialFlash.create("log", 1);
    Serial.print("Log Made: ");
    Serial.println(made);
    file = SerialFlash.open("log");
    file.write(&buff, 11);
    flight_counter = 10;
    file.close();
  }
  else
  {
    file.read(buff, 1);
    buff[0] = buff[0] + 1;
    flight_counter = buff[0];
    file.close();
    SerialFlash.remove("log");
    SerialFlash.create("log", 1);
    file = SerialFlash.open("log");
    file.write(&buff, 1);
    file.close();
  }
}

String makeFileName(int flightNum, int PacketNum)
{
  return "F" + (String)flightNum + "P" + (String)PacketNum;
}

//inits the flash chip
void init_flash()
{
  bool active = SerialFlash.begin(FLASH_PIN);
  init_flight_counter();
  printMessage("Flash chip: ");
  printMessageln(active);
}


extern uint8_t numPriority, numRegular;
//Writes the packet to the onboard flash chip
//prefers 256 byte chunks as the library says this amount is the most efficent
void save_packet(Packet packet)
{
  uint8_t num_packets = numRegular + numPriority;
  String filename = makeFileName(flight_counter, num_packets);
  Serial.println(filename);
  bool result = SerialFlash.create(filename.c_str(), packet.getLength());
  Serial.print("made: ");
  Serial.println(result);
  SerialFlashFile file = SerialFlash.open(filename.c_str());
  for(int i = 0; i < packet.getLength(); i += 256)
  {
    if(packet.getLength() - i < 256) //if we don't have enough space for a full chunk to be written
    {
      uint32_t written = file.write(packet.getArrayAt(i), packet.getLength() - i); //write to the end of the packet
      Serial.println(written);
    }
    else
    {
      uint32_t written = file.write(packet.getArrayAt(i), 256); // else write a 256 chunk
      Serial.println(written);
    }
  }
  file.close();
}

void readAllPackets()
{
  int packetNum = 0;
  int flightNum = 0;
  String filename = makeFileName(flightNum, packetNum);
  SerialFlashFile file = SerialFlash.open(filename.c_str());
  Serial.println(filename);
  uint8_t buff[256];
  uint32_t readIn = 0;
  while(file)
  {
    int readSize;
    int fileSize = file.size();
    Serial.println(fileSize);
    while(fileSize > 0)
    {
      if(fileSize < 256)
      {
        uint32_t readed = file.read(buff, fileSize);
        readSize = fileSize;
        fileSize -= readSize;
        readIn += readed;
      }
      else
      {
        uint32_t readed = file.read(buff, 256);
        readSize = 256;
        fileSize -= readSize;
        readIn += readed;
      }

      for(int i = 0; i < readSize; i++)
      {
        Serial.print(buff[i]);
        Serial.print(", ");
      }
    }
    Serial.println();

    packetNum++;
    String filename = makeFileName(flightNum, packetNum);
    Serial.println(filename);
    readIn = 0;
    file = SerialFlash.open(filename.c_str());
    if(!file)
    {
      packetNum = 0;
      flightNum++;
      String filename = makeFileName(flightNum, packetNum);
      Serial.println(filename);
      file = SerialFlash.open(filename.c_str());
    }
  }
  Serial.println("Done");
}
