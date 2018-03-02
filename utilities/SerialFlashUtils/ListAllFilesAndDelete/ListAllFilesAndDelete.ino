#include<SerialFlash.h>
#include<SPI.h>

#define MOSI              11
#define MISO              12
#define SCK               13

bool deleteAll = true;

void spaces(int num) {
  for (int i=0; i < num; i++) {
    Serial.print(" ");
  }
}

void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);
   SPI.setMOSI(MOSI);
  SPI.setMISO(MISO);
  SPI.setSCK(SCK);

  pinMode(13, OUTPUT); 
  pinMode(23, OUTPUT);
  digitalWrite(23, HIGH);
  bool active = SerialFlash.begin(14);
  Serial.println(active);
  Serial.println(SerialFlash.ready());

  SerialFlash.opendir();
  while (1) {
    char filename[64];
    uint32_t filesize;

    if (SerialFlash.readdir(filename, sizeof(filename), filesize)) {
      Serial.print("  ");
      Serial.print(filename);
      spaces(20 - strlen(filename));
      Serial.print("  ");
      Serial.print(filesize);
      Serial.print(" bytes");
      

      if(deleteAll)
      {
        bool removed = SerialFlash.remove(filename);
        if(removed)
        {
          Serial.print("    removed   ");
        }
      }

      Serial.println();
    } else {
      break; // no more files
    }
  }
  Serial.println("Done");
}

void loop() {
  // put your main code here, to run repeatedly:

}
