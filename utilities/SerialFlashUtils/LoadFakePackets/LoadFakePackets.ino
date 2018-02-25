#include <SerialFlash.h>
#include <SPI.h>

#include"Packet.h"

#define MOSI              11
#define MISO              12
#define SCK               13

int num_packets;

uint8_t buff[PACKET_MAX];

//Writes the packet to the onboard flash chip
//prefers 256 byte chunks as the library says this amount is the most efficent
void save_packet(Packet packet)
{
  String filename = "Packet_" + (String)num_packets;
  Serial.println(filename);
  Serial.println(packet.getLength());
  bool result = SerialFlash.createErasable(filename.c_str(), packet.getLength());
  Serial.print("Result: ");
  Serial.println(result);
  SerialFlashFile file = SerialFlash.open(filename.c_str());
  Serial.print("File Open: ");
  Serial.println(file == true);
  Serial.println(file.getFlashAddress());
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
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("Begin");
  num_packets = 0;
  SPI.setMOSI(MOSI);
  SPI.setMISO(MISO);
  SPI.setSCK(SCK);

  pinMode(13, OUTPUT); 
  pinMode(23, OUTPUT);
  digitalWrite(23, HIGH);
  bool active = SerialFlash.begin(14);
  Serial.println(active);

   uint8_t id[5];
  SerialFlash.readID(id);
  for(int i = 0; i <5; i++)
  {
    Serial.print(id[i]);
    Serial.print(",");
  }
  Serial.println();
  SerialFlash.eraseAll();
  Serial.println("Erased");
  
  //Flash LED at 1Hz while formatting
  while (!SerialFlash.ready()) {
    delay(500);
    digitalWrite(13, HIGH);
    delay(500);
    digitalWrite(13, LOW);
  }
  
  Serial.println(SerialFlash.ready());
  Serial.print("Capacity: ");
  Serial.println(SerialFlash.capacity(id));
}

void loop() {
  if(num_packets < 10)
  {
    Serial.println("making packet");
    // put your main code here, to run repeatedly:
    for(int i = 0; i < PACKET_MAX; i++)
    {
      buff[i] =  172; //(uint8_t)((256.00/PACKET_MAX) * i);
      Serial.print(buff[i]);
      Serial.print(", ");
    }
    Serial.println();
  
    Packet p = Packet(180, buff, PACKET_MAX, true);
  
    save_packet(p);
    num_packets++;
  
    Serial.println("Saved");
  }
}
