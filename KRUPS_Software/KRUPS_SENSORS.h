#ifndef KRUPS_SENSORS_H
#define KRUPS_SENSORS_H

#include <MPU9250.h>
#include "control.h"

#define gyroXcorr     (0)
#define gyroYcorr     (0)
#define gyroZcorr     (0)
#define accelXcorr    (0)
#define accelYcorr    (0)
#define accelZcorr    (0)
#define hiXcorr       (0)
#define hiYcorr       (0)
#define hiZcorr       (0)
#define magXcorr      (0)
#define magYcorr      (0)
#define magZcorr      (0)

//High accel pins
#define ADXL377x    (35)
#define ADXL377y    (36)
#define ADXL377z    (37)

MPU9250 sense(Wire, 0x68);
    
// initialize the sensors (mag, gyro, accel)
void init_Sensors(void)
{
	//Wire.begin(I2C_MASTER, 0x00, I2C_PINS_18_19, I2C_PULLUP_INT, 400000);
  Wire.begin();
    int status = sense.begin();
	if(status < 0){
		 Serial.println("IMU initialization unsuccessful");
		Serial.println("Check IMU wiring or try cycling power");
		Serial.print("Status: ");
		Serial.println(status);
	}
	else{
		Serial.println("MPU initialization successful");
		//sense.calibrateGyro();
		//sense.calibrateAccel();
		//sense.setGyroFSR(2000); //2000 DPS
    }//mag sensor is not calibrateable
}

// Reads all axis of the MPU9250 gyro and appends values to
// end of the input buffer and moves location pointer accordingly
void Read_gyro(uint8_t *buf, size_t &loc) { // requires 6 bytes in buffer
    sense.readSensor();
    //sense.ReadGyro(buf,loc);
}

// Reads all axis of the MPU9250 accel and appends the values to
// end of the input buffer and moves location pointer accordingly
void Read_loaccel(uint8_t*buf, size_t &loc) { // requires 6 bytes in buffer
    sense.readSensor();
    //sense.ReadAccel(buf,loc);
}

// Reads all axis of the MPU9250 magnetometer and appends the values to
// end of the input buffer and moves location pointer accordingly
void Read_mag(uint8_t *buf, size_t &loc) { // requires 6 bytes in buffer
    sense.readSensor();
    //sense.ReadMag(buf,loc);
}
// Reads all axis of the ADXL377 accel and appends the values to
// the end of the input buffer, and moves the location pointer accordingly
void Read_hiaccel(uint8_t *buf, size_t &loc) { // requires 6 bytes in buffer
  int16_t xval, yval, zval;
  xval = analogRead(ADXL377x);
  yval = analogRead(ADXL377y);
  zval = analogRead(ADXL377z);
  append(buf, loc, xval + hiXcorr);
  append(buf, loc, yval + hiYcorr);
  append(buf, loc, zval + hiZcorr);
}

/*
Grabs current time in millis and saves it as a three byte number
in the buffer
*/
void save_time(uint8_t* buff, size_t& loc)
{
  long time = millis();
  buff[loc++] = time & 0xFF; //low byte
  buff[loc++] = time >> 8; //mid byte
  buff[loc++] = time >> 16; //top byte
}


#endif
