#ifndef CONTROL_H
#define CONTROL_H


#include "Packet.h"
#include "Config.h"

/*
	Functions and time delay for activating parachute Solenoids

*/
extern volatile bool chuteDeployed;

const int time_out = 75; 

void sealChuteLid()
{
 analogWrite(PARACHUTE_PIN, 128);
 delay(time_out);
 analogWrite(PARACHUTE_PIN, 0);
 chuteDeployed = false;
}

void deployChute()
{
 analogWrite(PARACHUTE_PIN, 250);
 delay(time_out);
 analogWrite(PARACHUTE_PIN, 0);
 chuteDeployed = true;
}
/*
 * Decides sorting of packets in the priority queue
 * Method: Checks the priority flag of the packet 
 * if same priority sorts by time stap of first sensor read
 */
bool packetPriorityCalc(Packet a, Packet b)
{
  if(a.getPriority() && !b.getPriority())
  {
    return true; //a is priority, b is not, a has higher priority
  }
  else if(!a.getPriority()&& b.getPriority())
  {
    return false; //a is not priority, b is, b has higher priority
  }
  else //have same priority flag compare time
  {
    //check time stamps and check if already packed for transmission
    //grab time stamps from a and b
    int timeA = a.getPackTime();
    int timeB = b.getPackTime();
    return timeA <= timeB; //priority goes to which ever was made first
  }
}


//Appends a 16 bit interger to a buffer, also increments
//the provided location in the buffer
void append(uint8_t *buf, size_t &loc, int16_t input) {
  buf[loc++] = input;
  buf[loc++] = input >> 8;
}

/*
 * Prints a message over Serial if the settings allow it w/o newline
 */
template<typename T>
void printMessage(T s)
{
    #if OUTPUT_MESSAGES
      Serial.print(s);
    #endif
}

/*
 * Prints a message over Serial if the settings allow it w newline
 */
template<typename T>
void printMessageln(T s)
{
    #if OUTPUT_MESSAGES
      Serial.println(s);
    #endif
}

/*
 * Prints a newline over Serial if the settings allow it w newline
 */
void printMessageln()
{
    #if OUTPUT_MESSAGES
      Serial.println();
    #endif
}
/*
 * Prints a packet to the serial port, so that it may be copied 
 * onto a computer directly, avoiding sending it through comms
 */
void printPacket(Packet packet)
{
  Serial.println(packet.getLength());
  for(uint16_t i = 0; i < packet.getLength(); i++)
  {
    Serial.print(packet[i]);
    Serial.print(" ");
  }
  Serial.println();
}

/*
 * Reads the pwr_pin and checks if a power off signal has been sent
 */
void checkPowerOffSignal()
{
	if(analogRead(PWR_PIN) == 4095 && millis() > 20 * 1000)
    {
		while(analogRead(PWR_PIN) == 4095);
		//digitalWrite(13, LOW);
		digitalWrite(PWR_PIN, LOW);
	}
}


/*
 * blinks the LED on pin 13 numTime times with a pauseTime ms delay between
 */
void blinkLed(int numTimes, int pauseTime)
{
  #if USE_LED
    for(int i = 0; i < numTimes; i++)
    {
      delay(pauseTime);
      digitalWrite(13, LOW);
      delay(pauseTime);
      digitalWrite(13,HIGH);
    }
  #endif
}

//external variables (from KRUPS_Software) for use with packStats() and checkSerialIn()
//done as extern instead of parameters to lower overhead of function call
extern uint8_t numRegular, numPriority, sendAttemptErrors;
extern uint16_t bytesMade;
extern double avgCompressR, avgCompressP, avgTimeSinceR, avgTimeSinceP;
extern volatile bool GPS_Mode, splash_down, chuteDeployed;
extern PriorityQueue<Packet> message_queue;
extern size_t rloc, ploc;
extern elapsedMillis timeSinceR, timeSinceP;
extern IridiumSBD isbd;

/*
 * Fills a packet from loc 2 to 12 with stats from current program run
 */
void packStats(Packet& p)
{
  size_t loc = 2; //start  after the decompress len
  //timeStamp
  uint16_t timeSeconds  = millis()/1000;
  append(p.getArrayBase(), loc, timeSeconds);
  
  //num Regular packets
  p.getArrayBase()[int(loc)] = numRegular;
  loc++;
  
  //num p 
  p.getArrayBase()[int(loc)] = numPriority;
  loc++;
  
  // r compress avg
  p.getArrayBase()[int(loc)]  = uint8_t(avgCompressR/numRegular);
  loc++;
  
  // p compress avg
  p.getArrayBase()[int(loc)]  = uint8_t(avgCompressP/numPriority);
  loc++;
  
  //r build time 
  p.getArrayBase()[int(loc)]  = uint8_t(avgTimeSinceR/numRegular);
  loc++;
  
  //p build time
  p.getArrayBase()[int(loc)]  = uint8_t(avgTimeSinceP/numPriority);
  loc++;
  
  //failed sends
  p.getArrayBase()[int(loc)] = sendAttemptErrors;
  loc++;
  
  //bytes made 
  append(p.getArrayBase(), loc, bytesMade);
}


/*
 *Checks the Serial port for inbound commands from KICC 
 */
void checkSerialIn()
{
  if(Serial.available())
  {
    String command = Serial.readString();
    if(command == "OFF\n")
    {
      blinkLed(2, 1000);
      digitalWrite(PWR_PIN, LOW);
    }
    else if(command == "STATUS\n")
    {
      Serial.println("ON");
      if(GPS_Mode)
      {
        printMessageln("GPS Mode");
      }
      else
      {
        printMessage("Total Packets: ");
        printMessageln(numRegular + numPriority);
        printMessage("In queue: ");
        printMessageln(message_queue.count());
        printMessage("Regular Location: ");
        printMessageln(rloc);
        printMessage("Priority Location: ");
        printMessageln(ploc);
        printMessage("Average R Compression: ");
        printMessageln(uint8_t(avgCompressR/numRegular));
        printMessage("Average P Compression: ");
        printMessageln(uint8_t(avgCompressP/numPriority));
        printMessage("Time Since Last R: ");
        printMessageln(timeSinceR);
        printMessage("Time Since Last P: ");
        printMessageln(timeSinceP);
        if(splash_down)
        {
          printMessageln("Splash Down has occured");
        }
        else
        {
          printMessageln("Splash Down has not occured");
        }

        if(chuteDeployed)
        {
         printMessageln("Chute deploy has occured");
        }
        else
        {
         printMessageln("Chute deploy has not occured");
        }
        
       } 
     }
	 else if(command == "DEPLOY\n")
	 {
		Serial.println("DEPLOY");
    deployChute();
	 }
	 else if(command == "SEAL\n")
	 {
    Serial.println("SEALING LID");
    sealChuteLid();
	 }
   else if(command == "TIME\n")
   {
    Serial.print("Time:");
    Serial.println(millis());
    Serial.print("Time till chute deploy:");
    Serial.println((TIME_TO_DEPLOY - millis())/1000);
    Serial.print("Time till splash:");
    Serial.println((TIME_TO_SPLASH_DOWN - millis())/1000);
   }
   else if(command == "CSQ\n")
   {
    int signalQuality;
    isbd.getSignalQuality(signalQuality);
    Serial.println();
    Serial.println();
    Serial.println();
    Serial.println();

    Serial.print("On a scale of 0 to 5, signal quality is currently ");
    Serial.print(signalQuality);
    Serial.println(".");

    Serial.println();
    Serial.println();
    Serial.println();
    Serial.println();
   }
   }
}

#endif
