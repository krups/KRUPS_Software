/*
 * Config.h holds all top level macros for on board settings
 * this includes flight time, buffer sizes, and onboard sensors
 * 
 * Author: Collin Dietz
 * Email: c4dietz@gmail.com
 *
 * Co-Author: Josh Loredo
 * Email: josh.loredo@live.com
 */

#ifndef CONFIG_H
#define CONFIG_H

/*
 * Debugging Control
 */
#define DEBUG_SENSORS (false) //decides to use real or fake functions to allow testing
#define OUTPUT_MESSAGES (true) //decides if debug messages should be sent over serial
#define OUTPUT_PACKETS (true) //deciides if packets should be output on succesful iridium transfer 
#define DEBUG_IRIDIUM (true) //decides if status messages from iridium should be printed
#define USE_LED (false) //decides if the LED on pin 13 should be controlable for visual debugging 
#define USE_MODEM (true) //turns on/off modem

/*
 * Device pins
 */
#define PWR_PIN (23) //pin to maintain power
#define FLASH_PIN (0) //select pin for the flash chip
#define PARACHUTE_PIN (5) //pin to trigger parachute deploy

/*
 * Flight control data
 */
 #define TIME_TO_DEPLOY (590900) //Time until parachute deploy routine begins in ms
#define TIME_TO_SPLASH_DOWN (TIME_TO_DEPLOY + 569700) //Time until splash down routine begins in ms


#define MEASURE_READ (35) //size of all the bytes coming in form the sensors
#define GPS_MODE_FREQ (10) //number of seconds to delay betweeen messages in GPS mode for testing at base

/*
 * Buffers
 */
#define BUFFER_SIZE   (8000) //size of the measurement buffer about the max ammount of data that can be compressed to packet size
#define COMPRESS_BUFF_SIZE (2100) // size of buffer to hold compression data
#define WORKSPACESIZE ((1UL << 8)*8) //extra space for compression work

/*
 * Packet Data
 */
#define PACKET_SIZE (512) //size of a packets to send
#define HEADER_SIZE (13)  //number of bytes taken up in the packet by buffer
#define PACKET_MAX (PACKET_SIZE - HEADER_SIZE)

/*
 * Sensor Control
 */
 #define SENSOR_PACKAGE (MPU9)
 #define USE_HI_ACCEL (false)
 #define USE_GYRO (false)
 #define USE_LO_ACCEL (false)
 #define USE_MAG (false)
 #define USE_TIME (true)

 /*
  * GPS Control
  */
#define GPS_WIPE_ON_START (true) //controls if the GPS should wipe all previous logging on power on
#define USE_GPS_LOGGING (true) //controls if the GPS should turn on logging on initiation
#define GPS_START_TIME_MAX (1*60*1000) //time in ms to wait for logging to start
#define PHONE_TO_TEXT ("6065341006") //Phone number of person to receive GPS texts

#endif
