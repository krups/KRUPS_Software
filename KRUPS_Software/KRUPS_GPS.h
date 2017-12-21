/*
 * Author: Collin Dietz
 * Date: 12/21/17
 * Control funtions for an Adafruit ultimate GPS module
 */
 
#include<Adafruit_GPS.h>
#include <elapsedMillis.h>

#define GPS_Serial (Serial1) //Hardware serial connection for the module

Adafruit_GPS GPS(&GPS_Serial); //global GPS object

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
};

GPS_Data lastValid;


/*
 * Initilizes the GPS module, sets data output, freq
 * begins logging, and wipes the onboard flash memory
 * GPS_WIPE_ON_START - sets if flash should be wipped before turinng on loggig
 * USE_GPS_LOGGING - deterimines if logging should be used
 * GPS_START_TIME_MAX - max time to wait for GPS logging to begin
 */
void init_GPS()
{
  GPS.begin(9600); //start gps and default baud
  
  //if we desire wipe the flash on power on
  #if(GPS_WIPE_ON_START)
      GPS.sendCommand(PMTK_LOCUS_ERASE_FLASH);
  #endif
  
  //set output for RMC+GGA data (what the parser cares about)
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA); 
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);  //set update rate to 1 Hz (suggested)

  #if(USE_GPS_LOGGING)
  //start up logging, try as long as GPS_START_TIME_MAX
  elapsedMillis gpsTimeout = 0;

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
}


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
      lastValid.hour = GPS.hour; lastValid.minute = GPS.minute; lastValid.seconds = GPS.seconds;
      lastValid.milliseconds = GPS.milliseconds;
      lastValid.latitude = GPS.latitude; lastValid.longitude = GPS.longitude;
      lastValid.latitude_fixed = GPS.latitude_fixed; lastValid.longitude_fixed = lastValid.longitude_fixed;
      lastValid.latitudeDegrees = GPS.latitudeDegrees; lastValid.longitudeDegrees = lastValid.longitudeDegrees;
      lastValid.geoidheight = GPS.geoidheight; lastValid.altitude = lastValid.altitude;
      lastValid.speed = GPS.speed; lastValid.angle = lastValid.angle; 
      lastValid.magvariation = GPS.magvariation; lastValid.HDOP = lastValid.HDOP;
      lastValid.lat = GPS.lat; lastValid.lon = lastValid.lon; lastValid.mag = GPS.mag;
      lastValid.fixquality = GPS.fixquality; lastValid.satellites = lastValid.satellites;
    }
  }
}

