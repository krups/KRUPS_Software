#include <SerialFlash.h>
#include <SPI.h>

void spaces(int num) {
  for (int i=0; i < num; i++) {
    Serial.print(" ");
  }
}

String makeFileName(int flightNum, int PacketNum)
{
  return "F" + (String)flightNum + "P" + (String)PacketNum;
}

void readAllPackets(bool shouldDelete)
{
  SerialFlashFile logFile = SerialFlash.open("log");
  if(logFile)
  {
    Serial.print("log");
    if(shouldDelete)
    {
      SerialFlash.remove("log");
      Serial.println("Removed");
    }
  }
  for(int flightNum = 0; flightNum < 255; flightNum++)
  {
    for(int packetNum = 0; packetNum < 255; packetNum++)
    {
      String fileName = makeFileName(flightNum, packetNum);
      SerialFlashFile file = SerialFlash.open(fileName.c_str());
      if(file)
      {
        Serial.print(fileName);
        spaces(20 - fileName.length());
        if(shouldDelete)
        {
          SerialFlash.remove(fileName.c_str());
          Serial.println("Removed");
        }
        else
        {
          Serial.println();
        }
        
      }
    }
  }
}

void setup() {
  pinMode(23, OUTPUT);
  digitalWrite(23, HIGH);
  Serial.begin(9600);
  Serial.println("Starting");
  bool active = SerialFlash.begin(14);
  Serial.print("Flash chip: ");
  Serial.println(active);
  readAllPackets(true);
//  SerialFlash.eraseAll();
//  while(!SerialFlash.ready());
  Serial.println("Done");
}

void loop() {
}
