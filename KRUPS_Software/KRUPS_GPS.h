/*
 * Author: Collin Dietz
 * Date: 12/21/17
 * Control funtions for an Adafruit ultimate GPS module
 */
 
#include<Adafruit_GPS.h>
#include <elapsedMillis.h>
#include"Config.h"
#include"Control.h"

#define GPS_Serial (Serial2) //Hardware serial connection for the module
#define GPS_Enable (8) //GPS enable pin

Adafruit_GPS GPS(&GPS_Serial); //global GPS object


/*
 * Struct to hold the most recent correct GPS data
 */
struct GPS_Data
{
  uint8_t hour, minute, seconds, year, month, day;
  uint16_t milliseconds;
  // Floating point latitude and longitude value in degrees.
  float latitude, longitude;
  // Fixed point latitude and longitude value with degrees stored in units of 1/100000 degrees,
  // and minutes stored in units of 1/100000 degrees.  See pull #13 for more details:
  //   https://github.com/adafruit/Adafruit-GPS-Library/pull/13
  int32_t latitude_fixed, longitude_fixed;
  float latitudeDegrees, longitudeDegrees;
  float geoidheight, altitude;
  float speed, angle, magvariation, HDOP;
  char lat, lon, mag;
  uint8_t fixquality, satellites;
  boolean transmitted;
};

GPS_Data lastValid;

/*
 * Control functions for lastValid GPS_Data
 */

//checks if the struct has a valid GPS posisiton saved in it
boolean haveValidPos()
{
  return lastValid.fixquality != 0;
}

//sets transmitted flag to true in valid to make sure it isnt double sent
void GPS_transmissionComplete()
{
  lastValid.transmitted = true;
}

// checks if current gps data has been transmitted yet
boolean haveTransmitted()
{
  return lastValid.transmitted;
}


/*
 * Initilizes the GPS module, sets data output, freq
 * begins logging, and wipes the onboard flash memory
 * GPS_WIPE_ON_START - sets if flash should be wipped before turinng on loggig
 * USE_GPS_LOGGING - deterimines if logging should be used
 * GPS_START_TIME_MAX - max time to wait for GPS logging to begin
 */
void init_GPS()
{
  pinMode(GPS_Enable, OUTPUT);
  digitalWrite(GPS_Enable, HIGH); //hold gps enable up
  GPS.begin(9600); //start gps and default baud
  
  //if we desire wipe the flash on power on
  #if(GPS_WIPE_ON_START)
      GPS.sendCommand(PMTK_LOCUS_ERASE_FLASH);
      printMessageln("GPS log erased");
  #endif
  
  //set output for RMC+GGA data (what the parser cares about)
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA); 
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);  //set update rate to 1 Hz (suggested)

  #if(USE_GPS_LOGGING)
  //start up logging, try as long as GPS_START_TIME_MAX
  elapsedMillis gpsTimeout = 0;

  printMessageln("Starting GPS logging");
  while (gpsTimeout < GPS_START_TIME_MAX)
  {
    if(GPS.LOCUS_StartLogger())
    {
      //if succesful break the loop
      printMessageln("Logging Started");
      break;
    }
  }
  #endif

  lastValid.fixquality = 0; //functions as flag that a valid pos is found, have to initilize
}

/*
 * pulls in data from the GPS, if data is GPS valid is parsed
 * if GPS has a fix stores the most recent pos in lastValid
 */
void poll_GPS()
{
  char c = GPS.read(); //keep pulling in data
  if(GPS.newNMEAreceived()) //if a full sentence is through
  {
    if(!GPS.parse(GPS.lastNMEA()))
    {
      return; //parsing failed, keep pulling in data
    }

    //if good data save as most recent pos
    if(GPS.fix)
    {
      //printMessageln("GPS pos updated");
      lastValid.hour = GPS.hour; lastValid.minute = GPS.minute; lastValid.seconds = GPS.seconds;
      lastValid.milliseconds = GPS.milliseconds;
      lastValid.latitude = GPS.latitude; lastValid.longitude = GPS.longitude;
      lastValid.latitude_fixed = GPS.latitude_fixed; lastValid.longitude_fixed = GPS.longitude_fixed;
      lastValid.latitudeDegrees = GPS.latitudeDegrees; lastValid.longitudeDegrees = GPS.longitudeDegrees;
      lastValid.geoidheight = GPS.geoidheight; lastValid.altitude = GPS.altitude;
      lastValid.speed = GPS.speed; lastValid.angle = GPS.angle; 
      lastValid.magvariation = GPS.magvariation; lastValid.HDOP = GPS.HDOP;
      lastValid.lat = GPS.lat; lastValid.lon = GPS.lon; lastValid.mag = GPS.mag;
      lastValid.fixquality = GPS.fixquality; lastValid.satellites = GPS.satellites;
      lastValid.transmitted = false;
    }
  }
}

/*
 * reports relevant gps pos data as a string off of lastValid. 
 * Check haveValidPos() to make sure that data is valid 
 */
String currGPSPos()
{
  String pos = String(lastValid.hour) + ":" + String(lastValid.minute) + "." + String(lastValid.seconds) + '\n'; //add time info to string 
  pos += String(lastValid.latitudeDegrees) + '\t' + String(lastValid.longitudeDegrees) + '\n'; //posistion info
  pos += String(lastValid.speed) + "kn @" +  String(lastValid.angle) +'\n';
  pos += String(lastValid.altitude) + "m " + String(lastValid.fixquality);
  return pos;
}

