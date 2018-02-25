#include<SerialFlash.h>
#include<SPI.h>

#define MOSI              11
#define MISO              12
#define SCK               13

void setup() {
  Serial.begin(9600);
  Serial.println("Begining");

  SPI.setMOSI(MOSI);
  SPI.setMISO(MISO);
  SPI.setSCK(SCK);

  pinMode(13, OUTPUT); 
  pinMode(23, OUTPUT);
  digitalWrite(23, HIGH);
  bool active = SerialFlash.begin(14);
  Serial.println(active);
  Serial.println(SerialFlash.ready());
  
  int packetNum = 0;
  String prefix = "Packet_";
  String filename = prefix + (String)packetNum;
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
    Serial.println(readIn);
    packetNum++;
    filename = prefix + (String)packetNum;
    Serial.println(filename);
    readIn = 0;
    file = SerialFlash.open(filename.c_str());
  }
  Serial.println("Done");
}

void loop() {
  // put your main code here, to run repeatedly:

}
