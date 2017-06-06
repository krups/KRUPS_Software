// Solenoid control requires 500ms to fully extend
#define cam1    (11)
#define cam2    (12)
#define door    (14)
#define eject   (15)
#define timer   (13)
#define sensor  (10)

void setup() {
  // put your setup code here, to run once:
    pinMode(cam1, OUTPUT);
    pinMode(cam2, OUTPUT);
    pinMode(door, OUTPUT);
    pinMode(eject, OUTPUT);
    pinMode(sensor, OUTPUT);

    // wait until timer event triggers
    while(digitalRead(timer) != HIGH);

    // delay for safety
    delay(5700);
 
    // cameras on
    digitalWrite(cam1, HIGH);
    digitalWrite(cam2, HIGH);
    delay(100);
    digitalWrite(cam1, LOW);
    digitalWrite(cam2, LOW);
    
    // sensor board on
    digitalWrite(sensor, HIGH)
    delay(100);
    digitalWrite(sensor, LOW);
    
    // door open
    digitalWrite(door, HIGH);
    delay(500);
    digitalWrite(door, LOW);
    
    // sensor board on (redundant)
    digitalWrite(sensor, HIGH);
    delay(100);
    digitalWrite(sensor, LOW);
    
    // eject solenoid
    digitalWrite(eject, HIGH);
    delay(500);
    digitalWrite(eject, LOW);
}

void loop() {
    delay(30000);   // run video for 30 seconds
    
    // cycle cameras off
    digitalWrite(cam1, HIGH);
    digitalWrite(cam2, HIGH);
    delay(100);
    digitalWrite(cam1, LOW);
    digitalWrite(cam2, LOW);

    // cycle cameras on
    delay(100);
    digitalWrite(cam1, HIGH);
    digitalWrite(cam2, HIGH);
    delay(100);
    digitalWrite(cam1, LOW);
    digitalWrite(cam2, LOW);
}
