// Solenoid control requires 500ms to fully extend
#define cam1    (11)
#define cam2    (12)
#define door    (15)
#define eject   (14)
#define timer   (13)
#define sensor  (10)

void setup() {
    Serial.begin(9600);
    while(!Serial);
  // put your setup code here, to run once:
    pinMode(cam1, OUTPUT);
    pinMode(cam2, OUTPUT);
    pinMode(door, OUTPUT);
    pinMode(eject, OUTPUT);
    pinMode(sensor, OUTPUT);
    pinMode(timer, INPUT);

    // wait until timer event triggers
    Serial.println("untriggered");
    while(digitalRead(timer) != HIGH);
    Serial.println("triggered");

    // delay for safety
    delay(700);
    Serial.println("done delaying");
 
    // cameras on
    digitalWrite(cam1, HIGH);
    digitalWrite(cam2, HIGH);
    delay(100);
    digitalWrite(cam1, LOW);
    digitalWrite(cam2, LOW);
    Serial.println("cams on");
    
    // sensor board on
    digitalWrite(sensor, HIGH);
    delay(100);
    digitalWrite(sensor, LOW);
    Serial.println("sensor on");

    delay(3500); //
    // door open
    digitalWrite(door, HIGH);
    delay(500);
    digitalWrite(door, LOW);
    Serial.println("door open");
    
    // sensor board on (redundant)
    delay(2000);
    digitalWrite(sensor, HIGH);
    delay(100);
    digitalWrite(sensor, LOW);
    
    // eject solenoid
    digitalWrite(eject, HIGH);
    delay(500);
    digitalWrite(eject, LOW);
    Serial.println("ejected");
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
