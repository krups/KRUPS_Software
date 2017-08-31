#ifndef CONFIG_H
#define CONFIG_H
/*
 * Config.h holds all top level macros for settings on board
 * this includes flight time, buffer sizes, and onboard sensors
 */

/*
 * Device pins
 */
#define PWR_PIN (23) //pin to maintain power
#define FLASH_PIN (0) //select pin for the flash chip

/*
 * Flight data
 */
#define TIME_TO_SPLASH_DOWN (15*60*1000) //Time until splash down routine begins in ms

#define MEASURE_READ (33) //size of all the bytes coming in form the sensors
/*
 * Buffers
 */
#define BUFFER_SIZE   (8000) //size of the measurement buffer about the max ammount of data that can be compressed to packet size
#define COMPRESS_BUFF_SIZE (2100) // size of buffer to hold compression data
#define WORKSPACESIZE ((1UL << 8)*8) //extra space for compression work
/*
 * Packet Data
 */
#define PACKET_SIZE (1960) //size of a packets to send
#define HEADER_SIZE (13)  //number of bytes taken up in the packet by buffer
#define PACKET_MAX (PACKET_SIZE - HEADER_SIZE)


#endif

