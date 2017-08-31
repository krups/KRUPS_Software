#include <Arduino.h>
#include <IridiumSBD.h>
#include <QueueList.h>
#include <elapsedMillis.h>
#include <brieflzCompress.h>

#include "Packet.h"
#include "Config.h"
#include "Control.h"
//#include <SerialFlash.h>

#define TEST (false) //decides to use real or fake functions to allow testing
#if TEST
#include "Debug.h"
#else
#include "KRUPS_TC.h"
#include "KRUPS_Sensor.h"
#endif




volatile bool launched = false, ejected = true, splash_down = false, GPS_Mode = false;

/*
 * Variables associated with creating and tracking priority packets
 */
uint8_t priority_buf[BUFFER_SIZE]; // holds readings to be stored in priority packets
size_t ploc = 0; //location in the buffer
uint8_t numPriority = 0; //number of packets of this time made
double avgCompressP = 0; //avg compression value for this type of packet
elapsedMillis timeSinceP = 0; //time since this packet has been made last (ms)
double avgTimeSinceP; //avg time to make a priority packet (s)

uint8_t regular_buf[BUFFER_SIZE];
size_t rloc = 0; //location in regular buffer
uint8_t numRegular = 0; //number of packets of this time made
double avgCompressR = 0; //avg compression value for this type of packet
elapsedMillis timeSinceR = 0; //time since this packet has been made last (ms)
double avgTimeSinceR  = 0; //avg time to make a regular packet (s)

uint8_t compressedData[COMPRESS_BUFF_SIZE]; //storage for compressed data
uint8_t workspace[WORKSPACESIZE]; //workspace required by the compression library

uint16_t bytesMade = 0; //total data generated to this point
//uint16_t bytesSent = 0; //bytes sent by the modem
uint8_t sendAttemptErrors = 0; // number of times the iridium modem throws an error while attempting to send

IridiumSBD isbd(Serial1);  //the object for the

int16_t measure_reads = 0; //tells which cycle read we are on (0-3) to determine where to put data

QueueList<Packet> message_queue; //queue of messages left to send, messages pushed into front, pulled out back
QueueList<Packet> priority_queue; //queue of messages to send with higher priority

bool inCallBack = false; //used for debug out put


#if !TEST
void printPacket(Packet packet, int32_t len)
{
  for(int i = 0; i < len; i++)
  {
    Serial.print(packet[i]);
    Serial.print(" ");
  }
  Serial.println();
  Serial.println();
}

int16_t fTemp = 1;

void Read_temp(uint8_t *buf, size_t &loc)
{
  fTemp -= fTemp/5;
  append(buf, loc, fTemp);
}

void Read_gyrof(uint8_t *buf, size_t &loc)
{
    int t = int(millis()/1000);
    append(buf, loc, t);
    append(buf, loc, 2*t);
    append(buf, loc, 3*t);
}
#endif


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
 * GPS testing mode funciton to blast iridium at full pace
 */
 void GPS_Test_Mode()
 {
  GPS_Mode = true; //set flag to disable nonGPS actions in call back
  //digitalWrite(13, LOW);
  delay(250);
  //digitalWrite(13,HIGH);
  delay(250);
  //digitalWrite(13, LOW);
  delay(250);
  //digitalWrite(13,HIGH);
  delay(250);
  isbd.sendSBDText("GPS Mode Activated"); //send an indicator message
  //fill dummy packet
  size_t loc = 0;
  int16_t num = 0;
  while(loc < 1960)
  {
    append(compressedData, loc, num);
  }
  //send until power off (also checks for power off in callback)
  while(true)
  {
    isbd.sendSBDBinary(compressedData, 1960);
    checkPowerOffSignal();
    delay(10*1000);
  }
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
 * Protocol to send a message, with error handling
 */
void startSendingMessage(QueueList<Packet>& queue)
{
  //grab a packet
  Packet packet = queue.peek();
  uint16_t loc  = packet.getLength();
  packStats(packet);
  //attempt to send
  int response =  isbd.sendSBDBinary(packet.getArrayBase(), loc);
  //if it returns 0 message is sent
  if(response == 0)
  {
    queue.pop(); //remove message from the list
    //printPacket(packet, packet.getLength());
  }
  else //if not we have an error
  {
    //print the error
    //Serial.print("Error while sending packet: ");
    //Serial.println(response);

    
    sendAttemptErrors++;
    //don't remove from the queue
    
  }
}

/*
 * Method to handle reading in data from onboard sensors into buff at loc
 * Checking to see how compressed the data is, and if its over the limit pushing
 * a packet into the given queue
 */
void readInData(uint8_t* buff, size_t& loc, QueueList<Packet>& queue )
{
    unsigned long compressLen = 0;
    size_t priorloc;
    priorloc = loc;
    //read in sensor data
    save_time(buff, loc);        // 3 bytes
    //readGyro(buff, loc);        // 6 bytes
    //readAccel(buff, loc);   //lo acccel  // 6 bytes
    Read_hiaccel(buff, loc);     // 6 bytes
    //readMag(buff, loc);         // 6 bytes
    static elapsedMillis timeUsed;
    //loop over the mux posistions and read the TC's
   for(int i = 0; i < 4; i++)
   {
    //set the mux posistion
    setMUX(i);
    //track time used to do extra work
    timeUsed = 0;
    //do extra work
    //use the first 100 ms to do compression
    if(i == 0)
    {
      //if we can fill a packet start compressing data to check length
      if(priorloc > PACKET_MAX)
      {
        //compress the current packet
        compressLen = blz_pack(buff, compressedData, priorloc, workspace);
        
        Serial.print("Compressing: ");
        Serial.println(priorloc);
        Serial.print("To: ");
        Serial.println(compressLen);
      }
      //if the data compresses to more then fits in a packet we need to pacck a packet
      if(compressLen > PACKET_MAX || loc > BUFFER_SIZE*.95 ) //we need to roll back one measurement and make a packet
      {
        compressLen = blz_pack(buff, compressedData, priorloc - MEASURE_READ, workspace); //compress the data
        
        double efficency = 100 - 100 * double(compressLen)/(priorloc - MEASURE_READ);  // measure the efficency of the compression
        Packet p = Packet(priorloc - MEASURE_READ, compressedData, compressLen); //pack the packet
        queue.push(p); //push into the current queue

        //copy the data that wasn't packed into the packet
        for(unsigned int j = 0; j < loc - (priorloc - MEASURE_READ); j++)
        {
          buff[j] = buff[priorloc - MEASURE_READ + j];
        }
        loc -= (priorloc - MEASURE_READ);

        //stat tracking
        if(&queue == &priority_queue)
        {
          avgTimeSinceP += timeSinceP/1000;
          avgCompressP += efficency;
          timeSinceP = 0;
          numPriority++;
        }
        else
        {
          avgTimeSinceR += timeSinceR/1000;
          avgCompressR += efficency;
          timeSinceR = 0;
          numRegular++;
        }
        
        Serial.println("PACKET READY");
        Serial.print(priorloc - MEASURE_READ);
        Serial.print(" compressed to ");
        Serial.println(compressLen);
        Serial.print(100 - 100 * double(compressLen)/(priorloc - MEASURE_READ));
        Serial.println("%");
        Serial.print("Unused Space: ");
        Serial.println(1960 - p.getLength());
        Serial.println("Packet added");
        Serial.println(p.getLength());
        Serial.print("Total Packets: ");
        Serial.println(numPriority + numRegular);
        Serial.print("In  priority_queue: ");
        Serial.println(priority_queue.count());
        Serial.print("In queue: ");
        Serial.println(message_queue.count());
        Serial.println("Copying data to front");
        Serial.print("Time since last packet: ");
        Serial.println(timeSinceP);
        Serial.println(timeSinceR);
      }
    }
    if(timeUsed > 4)
    {
      //Serial.print("Time used: ");
      //Serial.println(timeUsed);
    }


    //if we didn't already use enough time wait the rest
	  while (timeUsed < 100);
    
    //read the data in
    Read_TC_at_MUX(buff, loc); //6 bytes per call
   }
   bytesMade += MEASURE_READ; //track the data generated
}

/*
 * Tasks to be completed at each measurement cycle
 */
void do_tasks()
{
        //pull data into the priority buff
        if(measure_reads == 0)
        { 
          readInData(priority_buf, ploc, priority_queue);
          //Serial.println(ploc);
        }
        //put data into the regular buff
        else
        {
          readInData(regular_buf, rloc, message_queue);
          //Serial.println(rloc);
        }
        
        measure_reads++; //incremeant measure_reads
        if(measure_reads > 3) //handle wrap around
        {
          measure_reads = 0;
        }

        //check splashDown
        if(millis() > TIME_TO_SPLASH_DOWN)
        {
          splashDown();
        }
}

/*
 * routine of tasks to be called once splash down has occured
 */
void splashDown()
{
  //Serial.println("Splashing Down");
  unsigned long compressLen;
  splash_down = true;

  //see if the remainder can be made one packet
  bool inOnePacket = false;
  if(ploc + rloc < COMPRESS_BUFF_SIZE)
  {
    //Serial.println("hybrid Packet");
    //Serial.println("Copying");
    
    //copy the data over
    for(int i = 0; i < rloc; i++)
    {
      priority_buf[ploc + i] = regular_buf[i];
    }
    
    //compresss
    compressLen = blz_pack(priority_buf, compressedData, ploc+rloc, workspace);
    //see if result will fit into one packet
    //Serial.println(compressLen);
    if(compressLen <= PACKET_MAX)
    {
      inOnePacket = true;
      Packet p = Packet(ploc+rloc, compressedData, compressLen);
      priority_queue.push(p);
      numPriority++;
      double efficency = 100 - 100* double(compressLen)/(ploc+rloc);
      avgCompressP += efficency;
      /*
      Serial.println("Packing");
      Serial.print(ploc+rloc);
      Serial.print(" compressed to ");
      Serial.println(compressLen);
      Serial.print(100 - 100* double(compressLen)/(ploc+rloc));
      Serial.println("%");
      */
    }
  }

  //if we havent made it into one packet keep the two split up and pack them
  if(!inOnePacket)
  {
    //Serial.println("Split up");
    //Serial.println("Priority");
    
    //priority packet
    compressLen = blz_pack(priority_buf, compressedData, ploc, workspace);
    Packet p = Packet(ploc, compressedData, compressLen);
    priority_queue.push(p);
    numPriority++;
    double efficency = 100 - 100* double(compressLen)/(ploc);
    avgCompressP += efficency;

    /*
    Serial.print(ploc);
    Serial.print(" compressed to ");
    Serial.println(compressLen);
    Serial.print(100 - 100* double(compressLen)/(ploc));
    Serial.println("%");
    */


    //regular packet
    compressLen = blz_pack(regular_buf, compressedData, rloc, workspace);
    p = Packet(rloc, compressedData, compressLen);
    message_queue.push(p);
    numRegular++;
    efficency = 100 - 100* double(compressLen)/(rloc);
    avgCompressR += efficency;
    /*
    Serial.print(rloc);
    Serial.print(" compressed to ");
    Serial.println(compressLen);
    Serial.print(100 - 100* double(compressLen)/(rloc));
    Serial.println("%");
    */
  }
  /*
  Serial.print("Total Packets: ");
  Serial.println(numRegular + numPriority);
  Serial.print("In queue: ");
  Serial.println(message_queue.count());
  Serial.print("In  priority_queue: ");
  Serial.println(priority_queue.count());
  */
}

///run if packet is not sent on a try
bool ISBDCallback() {
    //reports that we have entered call back
    if(!inCallBack)
    {
      //Serial.println("Call Back");
      inCallBack = true;
    }
    if(!splash_down && !GPS_Mode) //if we haven't splashed down or we arent in GPS Mode
    {
          do_tasks(); //make sure we are still reading in measurements and building packets
    }


    if(Serial.available())
    {
      String command = Serial.readString();
      if(command == "OFF")
      {

      //digitalWrite(13, HIGH);
      delay(1000);
      //digitalWrite(13, LOW);
      delay(1000);
      //digitalWrite(13, HIGH);
      delay(1000);
        
       digitalWrite(PWR_PIN, LOW);
      }
      else if(command == "STATUS")
      {
        if(GPS_Mode)
        {
          Serial.println("GPS Mode");
        }
        else
        {
          Serial.print("Total Packets: ");
          Serial.println(numRegular + numPriority);
          Serial.print("In queue: ");
          Serial.println(message_queue.count());
          Serial.print("In  priority_queue: ");
          Serial.println(priority_queue.count());
          Serial.print("Regular Location: ");
          Serial.println(rloc);
          Serial.print("Priority Location: ");
          Serial.println(ploc);
          Serial.print("Average R Compression: ");
          Serial.println(uint8_t(avgCompressR/numRegular));
          Serial.print("Average P Compression: ");
          Serial.println(uint8_t(avgCompressP/numPriority));
          Serial.print("Time Since Last R: ");
          Serial.println(timeSinceR);
          Serial.print("Time Since Last P: ");
          Serial.println(timeSinceP);
          if(splash_down)
          {
            Serial.println("Splash Down has occured");
          }
          else
          {
            Serial.println("Splash Down has not occured");
          }
        }
      }
    }

    checkPowerOffSignal();
    
    return true;
}

void setup() {
    //latch power
    analogReadRes(12);
    int powerMode = analogRead(PWR_PIN);
    pinMode(13, OUTPUT);
    pinMode(PWR_PIN, OUTPUT);
    digitalWrite(PWR_PIN, HIGH);
    digitalWrite(13, HIGH);
    delay(250);
    digitalWrite(13, LOW);
    delay(250);
    
    Serial.begin(9600);
    Serial1.begin(19200);
    //while(!Serial1);
    //sensors
    //Serial.println("Setting up");
    //initSensors();
    //Serial.println("Setting up");
    init_TC();
    //Serial.println("Setting up");

    //init_accel_interrupt(1.75, .1, 0x00);       // set for launch detection
    //init_gyro_interrupt(180, 0, 0x00);          // set for Ejection detection

    //iridium
    isbd.attachConsole(Serial);
    isbd.attachDiags(Serial);
    isbd.adjustSendReceiveTimeout(45);
    isbd.useMSSTMWorkaround(false);
    isbd.setPowerProfile(0);
   
    timeSinceR = 0;
    timeSinceP = 0;

    //Serial.println("Starting modem");
    int err;
    do
    {
      err = isbd.begin(); //try to start modem
      if(err != 0) //if we have an error report it
      {
        //Serial.print("Error: ");
        //Serial.println(err);
      }
    }while(!(err == 0)); //if err == 0 we successfully started up

    //digitalWrite(13, HIGH);
    Serial.println("CONNECTED");
    
    //report starting up and time to start up
    //Serial.print("Started: ");
    //Serial.print(millis()/1000);
    //Serial.println(" s");

    if(powerMode > 3000 && powerMode < 3250)
    {
      GPS_Test_Mode();
    }

   //digitalWrite(13, HIGH);

}

void loop() {
    //after ejection before splash_down routine
    //digitalWrite(13, HIGH);
    if(!splash_down)
    {
      do_tasks(); //complete needed tasks for measurment
    }
    if(!priority_queue.isEmpty()) //if there is priority data to send, do so
    {
      startSendingMessage(priority_queue); //send a message from the priority list
      inCallBack = false;
    }
    else if(!message_queue.isEmpty()) //if no priority data, check regular data
    {
      startSendingMessage(message_queue); //send a message from the non priority list
      inCallBack = false;
    }

    if(Serial.available())
    {
      String command = Serial.readString();
      if(command == "OFF")
      {

      //digitalWrite(13, HIGH);
      delay(1000);
      //digitalWrite(13, LOW);
      delay(1000);
      //digitalWrite(13, HIGH);
      delay(1000);
        
       digitalWrite(PWR_PIN, LOW);
      }
      else if(command == "STATUS")
      {
          Serial.print("Total Packets: ");
          Serial.println(numRegular + numPriority);
          Serial.print("In queue: ");
          Serial.println(message_queue.count());
          Serial.print("In  priority_queue: ");
          Serial.println(priority_queue.count());
          Serial.print("Regular Location: ");
          Serial.println(rloc);
          Serial.print("Priority Location: ");
          Serial.println(ploc);
          Serial.print("Average R Compression: ");
          Serial.println(uint8_t(avgCompressR/numRegular));
          Serial.print("Average P Compression: ");
          Serial.println(uint8_t(avgCompressP/numPriority));
          Serial.print("Time Since Last R: ");
          Serial.println(timeSinceR);
          Serial.print("Time Since Last P: ");
          Serial.println(timeSinceP);
          if(splash_down)
          {
            Serial.println("Splash Down has occured");
          }
          else
          {
            Serial.println("Splash Down has not occured");
          }
      }
    }

    //If we have gone for long enough turn off the teensy
    if(splash_down && millis() >= (TIME_TO_SPLASH_DOWN + 60*1000) && message_queue.isEmpty() && priority_queue.isEmpty())
    {
      isbd.sendSBDText("Tweezers");
      //digitalWrite(13, LOW);
      digitalWrite(PWR_PIN, LOW);
    }
  
    checkPowerOffSignal();  
}
