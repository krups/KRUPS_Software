#include<SerialFlash.h>
#define FLASH_PIN (0)

void setup() {
  //open serial and wait for it to complete
  Serial.begin(9600);
  while(!Serial)
  {
    delay(100);
  }

  //try and access the chip
  bool serialOpen = SerialFlash.begin(FLASH_PIN);
  if(serialOpen)
  {
    //erase the chip
    Serial.println("Erasing the chip...");
    SerialFlash.eraseAll();
    while(SerialFlash.ready() == false)
    {
      //wait for chip to be ready (30s to 2 mins)
    }
    Serial.println("Chip erased");
  }
  else
  {
    //error message
    Serial.println("Unable to access Chip");
  }
}

void loop() {
  // put your main code here, to run repeatedly:

}
