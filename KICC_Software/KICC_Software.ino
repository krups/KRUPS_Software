#define CAPSULE_SERIAL (Serial2)
#define KICC_SERIAL (Serial)
#define BAUD (19200)
void setup()
{
  CAPSULE_SERIAL.begin(BAUD);
  KICC_SERIAL.begin(BAUD);
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);
}

void loop()
{
  if(CAPSULE_SERIAL.available())
  {
    KICC_SERIAL.write(CAPSULE_SERIAL.read());
  }

  if(KICC_SERIAL.available())
  {
    CAPSULE_SERIAL.write(KICC_SERIAL.read());
  }
  
}

