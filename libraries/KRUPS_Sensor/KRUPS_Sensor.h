// Wrapper functions for all sensors on the KRUPS breakout board
// all functions both retrieve measurements, and append to the
// buffer to be compressed
#ifndef KRUPS_SENSOR_H
#define KRUPS_SENSOR_H

#include <Adafruit_BMP280.h>
#include <Adafruit_L3GD20_U.h>
#include <Adafruit_LSM303_U.h>
#include <Adafruit_LSM9DS1.h>
#include <compress.h>

// Set correction values appropriately to zero sensors when in a
// known situation e.g. board laying on a level, unmoving surface
#define gyroXcorr 		(0)
#define gyroYcorr  		(0)
#define gyroZcorr  		(0)
#define accelXcorr  	(0)
#define accelYcorr 		(0)
#define accelZcorr 		(0)
#define hiXcorr			(0)
#define hiYcorr			(0)
#define hiZcorr			(0)
#define magXcorr  		(0)
#define magYcorr 		(0)
#define magZcorr  		(0)

#define ADXL377x		(17)
#define ADXL377y		(16)
#define ADXL377z		(14)

// define interrupt pins
#define GYRO_INTERRUPT          (0)
#define ACCEL_INTERRUPT         (0)
extern volatile bool ejected, launched;

// function prototypes
void accel_int(void);
void gyro_int(void);

Adafruit_L3GD20_Unified gyro;
Adafruit_LSM303_Accel_Unified accel;
Adafruit_LSM303_Mag_Unified mag;
Adafruit_LSM9DS1 sense;
sensors_event_t gyro_event, accel_event, mag_event;

void write8(byte address, byte reg, byte value)
{
    Wire.beginTransmission(address);
    Wire.write((uint8_t)reg);
    Wire.write((uint8_t)value);
    Wire.endTransmission();
}

byte read8(byte address, byte reg)
{
    byte value;

    Wire.beginTransmission(address);
    Wire.write((uint8_t)reg);
    Wire.endTransmission();
    Wire.requestFrom(address, (byte)1);
    value = Wire.read();
    Wire.endTransmission();

    return value;
}

// initialize the sensors using adafruit unified driver
void init_Sensors(void) {
	gyro.begin(GYRO_RANGE_2000DPS);
	accel.begin();
	mag.begin();
}

//LSM Version
// initialize the sensors using adafruit unified driver
void initSensors(void)
{
    sense.begin();
    sense.setupAccel(sense.LSM9DS1_ACCELRANGE_16G);
    sense.setupMag(sense.LSM9DS1_MAGGAIN_16GAUSS);
    sense.setupGyro(sense.LSM9DS1_GYROSCALE_2000DPS);
}

// initialize interrupts for LSM303 on interrupt1 with given threshold in G
// and duration in seconds (saturates at maximum), final parameter is
// the interrupts to activate 0,0,ZH,ZL,YH,YL,XH,XL where 1=active, 0=not active
// and _H means higher than threshold and _L means lower than threshold
void init_accel_interrupt(float threshold, float duration, uint8_t interrupt) {
	float thr = threshold / .01575; 		// hardcoded for 2g scale, must change for other scales
	float dur = duration / .01;			// hardcoded for 100hz sampling time
	write8(LSM303_ADDRESS_ACCEL, LSM303_REGISTER_ACCEL_CTRL_REG3_A, 0x40); 				// activates ability to use interrupts
	write8(LSM303_ADDRESS_ACCEL, LSM303_REGISTER_ACCEL_INT1_CFG_A, (byte)interrupt);
	write8(LSM303_ADDRESS_ACCEL, LSM303_REGISTER_ACCEL_INT1_THS_A, (byte)thr); 			// changes interrupt threshold
	write8(LSM303_ADDRESS_ACCEL, LSM303_REGISTER_ACCEL_INT1_DURATION_A, (byte)dur); 	// changes duration for interrupt
	attachInterrupt(digitalPinToInterrupt(ACCEL_INTERRUPT), accel_int, RISING);
}

// initialize interrupts for L3GD20 on interrupt1 with given threshold in dps
// and duration in seconds (saturates at maximum), final parameter is
// the interrupts to activate 0,0,ZH,ZL,YH,YL,XH,XL where 1=active, 0=not active
// and _H means higher than threshold and _L means lower than threshold
void init_gyro_interrupt(float threshold, float duration, uint8_t interrupt) {
    float thr = threshold / .061;         // hardcoded for 2000DPS
    float dur = duration / .01;           // hardcoded for 100hz sampling time
    write8(L3GD20_ADDRESS, GYRO_REGISTER_CTRL_REG3, 0x80);                              // activates ability to use interrupts
    write8(L3GD20_ADDRESS, GYRO_REGISTER_INT1_CFG, (byte)interrupt);
    write8(L3GD20_ADDRESS, GYRO_REGISTER_TSH_XL, (byte)thr);                            // set lower and upper byte of interrupt threshold
    write8(L3GD20_ADDRESS, GYRO_REGISTER_TSH_XH, (int)thr>>8);
    write8(L3GD20_ADDRESS, GYRO_REGISTER_INT1_DURATION, (byte)dur);                     // set duration to trigger interrupt
    attachInterrupt(digitalPinToInterrupt(GYRO_INTERRUPT), gyro_int, FALLING);
}

// Reads all axis of the L3GD20 gyroscope and appends the values to
// the end of the input buffer, and moves the location pointer accordingly
void Read_gyro(uint8_t *buf, size_t &loc) { // requires 6 bytes in buffer
	gyro.getEvent(&gyro_event);
	append(buf, loc, gyro_event.gyro.x + gyroXcorr);
	append(buf, loc, gyro_event.gyro.y + gyroYcorr);
	append(buf, loc, gyro_event.gyro.z + gyroZcorr);
}

// Reads all axis of the LSM303 accel and appends the values to
// the end of the input buffer, and moves the location pointer accordingly
void Read_loaccel(uint8_t *buf, size_t &loc) { // requires 6 bytes in buffer
	accel.read();
	append(buf, loc, accel.raw.x + accelXcorr);
	append(buf, loc, accel.raw.y + accelYcorr);
	append(buf, loc, accel.raw.z + accelZcorr);
}

// Reads all axis of the LSM303 magnetometer and appends the values to
// the end of the input buffer, and moves the location pointer accordingly
void Read_mag(uint8_t *buf, size_t &loc) { // requires 6 bytes in buffer
	mag.read();
	append(buf, loc, mag.raw.x + magXcorr);
	append(buf, loc, mag.raw.y + magYcorr);
	append(buf, loc, mag.raw.z + magZcorr);
}

// Reads all axis of the LSM9DS1 gyro and appends values to
// end of the input buffer and moves location pointer accordingly
void readGyro(uint8_t *buf, size_t &loc) { // requires 6 bytes in buffer
    sense.readGyro();
    append(buf, loc, sense.gyroData.x + gyroXcorr);
    append(buf, loc, sense.gyroData.y + gyroYcorr);
    append(buf, loc, sense.gyroData.z + gyroZcorr);
}

// Reads all axis of the LSM9DS1 accel and appends the values to
// end of the input buffer and moves location pointer accordingly
void readAccel(uint8_t*buf, size_t &loc) { // requires 6 bytes in buffer
    sense.readAccel();
    append(buf, loc, sense.accelData.x + accelXcorr);
    append(buf, loc, sense.accelData.y + accelYcorr);
    append(buf, loc, sense.accelData.z + accelZcorr);
}

// Reads all axis of the LSM9DS1 magnetometer and appends the values to
// end of the input buffer and moves location pointer accordingly
void readMag(uint8_t *buf, size_t &loc) { // requires 6 bytes in buffer
    sense.readMag();
    append(buf, loc, sense.magData.x + magXcorr);
    append(buf, loc, sense.magData.y + magYcorr);
    append(buf, loc, sense.magData.z + magZcorr);
}

void readTemp(uint8_t *buf, size_t &loc) {
    sense.readTemp();
    append(buf, loc, sense.temperature);
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

// accel ISR sets launched bool
void accel_int(void) {
    launched = true;
}

// gyro ISR sets launched bool
void gyro_int(void) {
    ejected = true;
}

#endif
