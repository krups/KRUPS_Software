#include <SerialFlash.h>
#include <SPI.h>
#include"KRUPS_SerialFlash.h"

#include"Packet.h"

uint8_t numPriority, numRegular;
uint8_t buff[1960];


void setup() {
  pinMode(23, OUTPUT);
  digitalWrite(23, HIGH);
  numPriority = 0;
  numRegular = 0;
  Serial.begin(9600);
  while(!Serial);
  Serial.println("Starting");
  init_flash();
  //readAllPackets();
}

void loop() {
  if(numRegular < 10)
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
    numRegular++;
  
    Serial.println("Saved");
  }
  else
  {
    digitalWrite(23, LOW);
  }

}
