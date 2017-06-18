// Solenoid control requires 500ms to fully extend
#define DEBUG   (false)

#ifdef DEBUG
    #define door    (15)
    #define eject   (14)
#else
    #define door    (9) // Red LED
    #define eject   (8) // Green LED
#endif
#define cam1    (11)
#define cam2    (12)
#define timer   (13)
#define sensor  (10)

void cameraTrigger(void);

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
    while(digitalRead(timer) != HIGH);

    // delay for safety
    delay(700);
 
    // cameras on
    cameraTrigger();
    #ifdef Serial.println("cams on"); #endif
    
    // sensor board on
    digitalWrite(sensor, HIGH);
    delay(100);
    digitalWrite(sensor, LOW);

    delay(3500); //
    // door open
    digitalWrite(door, HIGH);
    delay(500);
    digitalWrite(door, LOW);
    
    // sensor board on (redundant)
    delay(2000);
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
    cameraTrigger();

    // cycle cameras on
    delay(100);
    cameraTrigger();
}

void cameraTrigger(void) {
    digitalWrite(cam1, HIGH);
    digitalWrite(cam2, HIGH);
    delay(100);
    digitalWrite(cam1, LOW);
    digitalWrite(cam2, LOW);
}

