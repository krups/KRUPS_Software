/*
 * Author: Collin Dietz
 * Date: 5/7/19
 * Control funtions for Fona GPS module
 */

#ifndef KRUPS_GPS_H
#define KRUPS_GPS_H

#include <Adafruit_FONA.h>

//GPS Initialization 
#define FONA_RST 11

// Buffer for GPS replies
char replybuffer[255];

//GPS is on Serial3
HardwareSerial *fonaSerial = &Serial3;
Adafruit_FONA_3G fona = Adafruit_FONA_3G(FONA_RST);

uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout = 0);

bool GPS_Setup()
{
  bool GPSFail;
  //starting baud rate, later set to 4800 per FONA requirements
  Serial.println("Turing on GPS");
  fonaSerial->begin(4800);
  
  //Connect to FONA
  if (!fona.begin(*fonaSerial)) 
  {
    Serial.println("Couldn't connect to GPs");
    GPSFail = true;
  }
  else
  {
    Serial.println("Connected to GPS");
    // Print module IMEI number.
    char imei[16] = {0}; // MUST use a 16 character buffer for IMEI!
    uint8_t imeiLen = fona.getIMEI(imei);
    if (imeiLen > 0) 
    {
      Serial.print("Module IMEI: "); Serial.println(imei);
    }
    GPSFail = false;
  }
  return GPSFail;
}

void SendGPS(char* gpsdata)
{
  // read the network/cellular status
  uint8_t n = fona.getNetworkStatus();
  Serial.print("Network status ");
  Serial.print(n);
  Serial.print(": ");
  switch(n)
  {
    case 0:
      Serial.println("Not registered");
      break;
    case 1:
      Serial.println("Registered (home)");
      break;
    case 2:
      Serial.println("Not registered (searching)");
      break;
    case 3:
      Serial.println("Denied");
      break;
    case 4:
      Serial.println("Unknown");
      break;
    case 5:
      Serial.println("Registered roaming");
      break;
    default:
      Serial.println("Unkown Case");
  }
        
  Serial.println(F("Activating GPS"));
  // turn GPS on
  if (!fona.enableGPS(true))
  Serial.println("Failed to turn on");
   
  Serial.println("Query GPS location");
  // check for GPS location
  fona.getGPS(0, gpsdata, 120);
  Serial.println("Reply in format: mode,fixstatus,utctime(yyyymmddHHMMSS),latitude,longitude,altitude,speed,course,fixmode,reserved1,HDOP,PDOP,VDOP,reserved2,view_satellites,used_satellites,reserved3,C/N0max,HPA,VPA");
  Serial.println(gpsdata);
      
  Serial.println("Send SMS with location");
  // send an SMS!
  char sendto[11]= PHONE_TO_TEXT;
  Serial.print("Send to #");
  Serial.println(sendto);
  Serial.print("Sending location....");
  Serial.println(gpsdata);
  if (!fona.sendSMS(sendto, gpsdata))
  {
    Serial.println("Failed");
  } 
  else 
  {
    Serial.println("Sent!");
  }
}

#endif
