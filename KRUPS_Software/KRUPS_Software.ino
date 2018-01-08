#include <Arduino.h>
#include <IridiumSBD.h>
#include <QueueList.h>
#include <elapsedMillis.h>
#include <brieflzCompress.h>

#include "Packet.h"
#include "Config.h"
#include "Control.h"
//#include <SerialFlash.h>
#include "KRUPS_GPS.h" //TODO: add debug functions

#if DEBUG
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

/*
 * Variables associated with creating and tracking regular packets
 */
uint8_t regular_buf[BUFFER_SIZE];
size_t rloc = 0; //location in regular buffer
uint8_t numRegular = 0; //number of packets of this time made
double avgCompressR = 0; //avg compression value for this type of packet
elapsedMillis timeSinceR = 0; //time since this packet has been made last (ms)
double avgTimeSinceR  = 0; //avg time to make a regular packet (s)

/*
 * variables for compression
 */
uint8_t compressedData[COMPRESS_BUFF_SIZE]; //storage for compressed data
uint8_t workspace[WORKSPACESIZE]; //workspace required by the compression library

/*
 * Stat tracking variables
 */
uint16_t bytesMade = 0; //total data generated to this point
//uint16_t bytesSent = 0; //bytes sent by the modem
uint8_t sendAttemptErrors = 0; // number of times the iridium modem throws an error while attempting to send
int16_t measure_reads = 0; //tells which cycle read we are on (0-3) to determine where to put data

#if USE_MODEM
  IridiumSBD isbd(Serial1);  //the object for the iridium modem
#endif

/*
 * Message Queues 
 */
QueueList<Packet> message_queue; //queue of messages left to send, messages pushed into front, pulled out back
QueueList<Packet> priority_queue; //queue of messages to send with higher priority

bool inCallBack = false; //used for debug out put

/*
 * GPS testing mode funciton to blast iridium at full pace
 */
 void GPS_Test_Mode()
 {
  GPS_Mode = true; //set flag to disable nonGPS actions in call back
  blinkLed(2, 250);
  
  #if USE_MODEM
    isbd.sendSBDText("GPS Mode Activated"); //send an indicator message
  #endif
  
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
    #if USE_MODEM
      isbd.sendSBDBinary(compressedData, 1960);
    #endif
    checkPowerOffSignal();
    delay(GPS_MODE_FREQ*1000);
  }
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
  #if USE_MODEM
    int response =  isbd.sendSBDBinary(packet.getArrayBase(), loc);
  #else
    int response = 0;
  #endif
  
  //if it returns 0 message is sent
  if(response == 0)
  {
    queue.pop(); //remove message from the list
    #if OUTPUT_PACKETS
      printPacket(packet);
    #endif
  }
  else //if not we have an error
  {
    //print the error
    printMessage("Error while sending packet: ");
    printMessageln(response);

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
    
    //read in sensor data, only if corresponding flag is set
    #if USE_TIME
      save_time(buff, loc);        // 3 bytes
    #endif

    #if USE_GYRO 
      readGyro(buff, loc);        // 6 bytes
    #endif

    #if USE_HI_ACCEL
      Read_hiaccel(buff, loc);     // 6 bytes
    #endif

    #if USE_LO_ACCEL
      readAccel(buff, loc);   //lo acccel  // 6 bytes
    #endif
    
    #if USE_MAG
      readMag(buff, loc);         // 6 bytes
    #endif

    
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
        
        printMessage("Compressing: ");
        printMessageln(priorloc);
        printMessage("To: ");
        printMessageln(compressLen);
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
        //if priority queue
        if(&queue == &priority_queue)
        {
          avgTimeSinceP += timeSinceP/1000;
          avgCompressP += efficency;
          timeSinceP = 0;
          numPriority++;
        }
        //if regular
        else
        {
          avgTimeSinceR += timeSinceR/1000;
          avgCompressR += efficency;
          timeSinceR = 0;
          numRegular++;
        }
        
        printMessageln("PACKET READY");
        printMessage(priorloc - MEASURE_READ);
        printMessage(" compressed to ");
        printMessageln(compressLen);
        printMessage(100 - 100 * double(compressLen)/(priorloc - MEASURE_READ));
        printMessageln("%");
        printMessage("Unused Space: ");
        printMessageln(1960 - p.getLength());
        printMessageln("Packet added");
        printMessageln(p.getLength());
        printMessage("Total Packets: ");
        printMessageln(numPriority + numRegular);
        printMessage("In  priority_queue: ");
        printMessageln(priority_queue.count());
        printMessage("In queue: ");
        printMessageln(message_queue.count());
        printMessageln("Copying data to front");
        printMessage("Time since last packet: ");
        printMessageln(timeSinceP);
        printMessageln(timeSinceR);
      }
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
  printMessageln("Splashing Down");
  unsigned long compressLen;
  splash_down = true; //set flag
  
  //clean up the rest of the data in the buffers and put in queue
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
      printMessageln("Packing");
      printMessage(ploc+rloc);
      printMessage(" compressed to ");
      printMessageln(compressLen);
      printMessage(100 - 100* double(compressLen)/(ploc+rloc));
      printMessageln("%");
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

    printMessage(ploc);
    printMessage(" compressed to ");
    printMessageln(compressLen);
    printMessage(100 - 100* double(compressLen)/(ploc));
    printMessageln("%");


    //regular packet
    compressLen = blz_pack(regular_buf, compressedData, rloc, workspace);
    p = Packet(rloc, compressedData, compressLen);
    message_queue.push(p);
    numRegular++;
    efficency = 100 - 100* double(compressLen)/(rloc);
    avgCompressR += efficency;
    
    printMessage(rloc);
    printMessage(" compressed to ");
    printMessageln(compressLen);
    printMessage(100 - 100* double(compressLen)/(rloc));
    printMessageln("%");
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
    //handles call back flag
    if(!inCallBack)
    {
      //Serial.println("Call Back");
      inCallBack = true;
    }

    
    if(!splash_down && !GPS_Mode) //if we haven't splashed down or we arent in GPS Mode
    {
          do_tasks(); //make sure we are still reading in measurements and building packets
    }
    else if(!GPS_Mode)//post splash down tasks
    {
      poll_GPS(); //if we are done reading sensors (i.e. splashed down) read in GPS data
    }
    
    checkSerialIn(); //check for serial commands
    checkPowerOffSignal();
    
    return true;
}

void setup() {
    analogReadRes(12); //Set Analog Reading to full 12 bit range
    int powerMode = analogRead(PWR_PIN); //save power on voltage to set mode later
    
    //set power latch pin as output and hold high
    pinMode(PWR_PIN, OUTPUT);
    digitalWrite(PWR_PIN, HIGH);

    //blink led to show succesful power on
    blinkLed(1, 250);

    #if OUTPUT_MESSAGES
      Serial.begin(9600); //debug output
    #endif
    Serial1.begin(19200); //hardware serial to iridum modem

    printMessageln("Power on");
    printMessageln("Initializing Sensors");
    
    //sensors
    initSensors();
    init_TC();

    //DEPRECATED interupts
    //init_accel_interrupt(1.75, .1, 0x00);       // set for launch detection
    //init_gyro_interrupt(180, 0, 0x00);          // set for Ejection detection

    //Settings for iridium Modem
    #if OUTPUT_MESSAGES && DEBUG_IRIDIUM && USE_MODEM
      isbd.attachConsole(Serial);
      isbd.attachDiags(Serial);
    #endif

    #if USE_MODEM
      isbd.adjustSendReceiveTimeout(45);
      isbd.useMSSTMWorkaround(false);
      isbd.setPowerProfile(0);
    

    printMessage("Turning on Modem");
    int err;
    do
    {
      err = isbd.begin(); //try to start modem
      if(err != 0) //if we have an error report it
      {
        printMessage("Error Turning on Modem:");
        printMessage(err);
      }
    }while(!(err == 0)); //if err == 0 we successfully started up
    
    //report starting up and time to start up
    printMessage("Modem started: ");
    printMessage(millis()/1000);
    printMessageln(" s");
    #endif

    init_GPS(); //start the GPS

    //check the input voltage and turn of GPS mode if 3.3v
    if(powerMode > 3000 && powerMode < 3250)
    {
      GPS_Test_Mode();
    }

    //init packet timers
    timeSinceR = 0;
    timeSinceP = 0;
}

void loop() {
    if(!splash_down) //pre splashdown tasks
    {
      do_tasks(); //complete needed tasks for measurment
    }
    else //post splash down tasks
    {
      poll_GPS(); //if we are done reading sensors (i.e. splashed down) read in GPS data
    }


    /*
     * communication control
     */
    //if we have a valid GPS pos that hasnt been sent trasmit it
    //always has priority after splashdown
    if(splash_down && haveValidPos() && !haveTransmitted())
    {
      #if USE_MODEM
        int response = isbd.sendSBDText(currGPSPos().c_str());
      #else
        int response = 0;
        Serial.println("Current Pos");
        Serial.println(currGPSPos());
      #endif
      if(response == 0)
      {
        GPS_transmissionComplete(); //if transmission was succesful set flag
      }
    }
    else if(!priority_queue.isEmpty()) //if there is priority data to send, do so
    {
      startSendingMessage(priority_queue); //send a message from the priority list
      inCallBack = false; //reset callback flag after leaving message sending protocol
    }
    else if(!message_queue.isEmpty()) //if no priority data, check regular data
    {
      startSendingMessage(message_queue); //send a message from the non priority list
      inCallBack = false;
    }

    checkSerialIn();
    checkPowerOffSignal();  

    //No longer need the capsule to turn off, if it is sending data we can find it, if not who cares
    //If we have gone for long enough turn off the teensy
    /*
    if(splash_down && millis() >= (TIME_TO_SPLASH_DOWN + 60*1000) && message_queue.isEmpty() && priority_queue.isEmpty())
    {
      isbd.sendSBDText("Tweezers");
      digitalWrite(PWR_PIN, LOW);
    }
    */
}
