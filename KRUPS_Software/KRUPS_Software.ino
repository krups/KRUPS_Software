#include <Arduino.h>
#include <IridiumSBD.h>
#include <QueueList.h>
#include <SerialFlash.h>
#include <elapsedMillis.h>
#include <brieflzCompress.h>
#include "Packet.h"
//#include <String>

#define TEST (true) //decides to use real or fake functions to allow testing
#if TEST
#include "Debug.h"
#else
#include <KRUPS_TC.h>
#include <KRUPS_Sensor.h>
#endif


#define BUFFER_SIZE   (12000) //size of the measurement buffer aboutthe max ammount of data that can be compressed to packet size
#define FLASH_PIN (0) //select pin for the flash chip
#define TIME_TO_SPLASH_DOWN (340000) //Time until splash down routine begins in ms
#define WORKSPACESIZE ((1UL << 8)*8) //extra space for compression work
#define MEASURE_READ (53) //size of all the bytes coming in form the sensors

volatile bool launched = false, ejected = true, splash_down = false, data_resend_complete = false;

uint8_t priority_buf[BUFFER_SIZE]; // holds readings to be stored in priority packets
size_t ploc = 0; //location in the buffer
elapsedMillis timeSinceP;

uint8_t regular_buf[BUFFER_SIZE];
size_t rloc = 0; //location in regular buffer
elapsedMillis timeSinceR;

uint8_t compressedData[10000]; //storage for compressed data
uint8_t workspace[WORKSPACESIZE];

IridiumSBD isbd(Serial1);

int16_t num_packets = 0; //counter for number of packets to establish chronlogical order
int16_t measure_reads = 0; //tells which cycle read we are on (0-3) to determine where to put data

QueueList<Packet> message_queue; //queue of messages left to send, messages pushed into front, pulled out back
QueueList<Packet> priority_queue; //queue of messages to send with higher priority

bool inCallBack = false; //used for debug out put

int16_t fTemp = 1;

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
#endif

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


//Writes the packet to the onboard flash chip
//prefers 256 byte chunks as the library says this amount is the most efficent
void save_packet(Packet packet)
{
  String filename = "Packet_" + String(num_packets);
  SerialFlash.create(filename.c_str(), packet.getLength());
  SerialFlashFile file = SerialFlash.open((filename + ".bin").c_str());
  for(int i = 0; i < packet.getLength(); i += 256)
  {
    if(packet.getLength() - i < 256) //if we don't have enough space for a full chunk to be written
    {
      file.write(packet.getArrayAt(i), packet.getLength() - i); //write to the end of the packet
    }
    else
    {
      file.write(packet.getArrayAt(i), 256); // else write a 256 chunk
    }
  }
}

void loadFlashToQueue()
{
  static int loadedCount = 0;
  static int timesLoaded = 0;
  if(loadedCount < num_packets)
  {
    SerialFlashFile file;
    String filename = "Packet_" + String(loadedCount) + ".bin";
    Serial.print("Opening ");
    Serial.print(filename);
    Serial.print(" from memory...");
    file = SerialFlash.open(filename.c_str());
    if(!file)
    {
      Serial.println("No file with that name");
    }
    else
    {
     Serial.println("done");
     Serial.println("Reading");
     file.read(measure_buf, file.size());
     Serial.println("Packing");
     Packet packet = Packet(loadedCount, measure_buf, file.size());
     Serial.println("Adding to queue");
     message_queue.push(packet);
     loadedCount++;
    }
  }
  else
  {
    timesLoaded++;
    if(timesLoaded < 2)
    {
      loadedCount = 0;
    }
    else
    {
        Serial.println("Data loaded in twice, stopping data retransmission");
        data_resend_complete = true;
    }
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
  //attempt to send
  int response = isbd.sendSBDBinary(packet.getArrayBase(), loc);
  //if it returns 0 message is sent
  if(response == 0)
  {
    queue.pop(); //remove message from the list
    printPacket(packet, packet.getLength());
  }
  else //if not we have an error
  {
    //print the error
    Serial.print("Error while sending packet: ");
    Serial.println(response);
    //don't remove from the queue
  }
}


void readInData(uint8_t* buff, size_t& loc, QueueList<Packet>& queue )
{
    unsigned long compressLen = 0;
    size_t priorloc;
    priorloc = loc;
    save_time(buff, loc); // 3 bytes
    Read_gyro(buff, loc);        // 6 bytes
    Read_loaccel(buff, loc);     // 6 bytes
    Read_hiaccel(buff, loc);     // 6 bytes
    Read_mag(buff, loc);         // 6 bytes
    Read_temp(buff, loc); //2 bytes
   static elapsedMillis timeUsed;
   for(int i = 0; i < 4; i++)
   {
    //set the mux posistion
    setMUX(i);
    //track time used to do extra work
    timeUsed = 0;
    //do extra work
    if(i == 0)
    {
      if(priorloc > PACKET_MAX)
      {
        //Serial.print("Compressing: ");
        //Serial.println(priorloc);
        compressLen = blz_pack(buff, compressedData, priorloc, workspace);
        //Serial.print("To: ");
        //Serial.println(compressLen);
      }
      if(compressLen > PACKET_MAX) //we need to roll back one measurement and make a packet
      {
        //Serial.println("PACKET READY");
        compressLen = blz_pack(buff, compressedData, priorloc - MEASURE_READ, workspace);
        //Serial.print(priorloc - MEASURE_READ);
        //Serial.print(" compressed to ");
        //Serial.println(compressLen);
        //Serial.print(100 - 100 * double(compressLen)/(priorloc - MEASURE_READ));
        //Serial.println("%");
        Packet p = Packet(priorloc - MEASURE_READ, compressedData, compressLen);
        //Serial.println("Packet added");
        queue.push(p);
        Serial.println(p.getLength());
        //Serial.println("Saving packet");
        //save_packet(p);
        num_packets++;
        //Serial.print("Total Packets: ");
        //Serial.println(num_packets);
        //Serial.print("In  priority_queue: ");
        //Serial.println(priority_queue.count());
        //Serial.print("In queue: ");
        //Serial.println(message_queue.count());
        //Serial.println("Copying data to front");
        for(int j = 0; j < loc - (priorloc - MEASURE_READ); j++)
        {
			    buff[j] = buff[priorloc - MEASURE_READ + j];
        }
        loc -= (priorloc - MEASURE_READ);

        if(&queue == &priority_queue)
        {
          //Serial.print("Time since last packet: ");
          //Serial.println(timeSinceP);
          timeSinceP = 0;
        }
        else
        {
          //Serial.print("Time since last packet: ");
          //Serial.println(timeSinceR);
          timeSinceR = 0;
        }
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
    /*
    Serial.print(i);
    Serial.print(": ");
    Serial.print((float)(buff[loc-6]+256*buff[loc-5])*.25);
    fTemp = buff[loc-6] + 256*buff[loc-5];
    Serial.print("deg C");
    Serial.print('\t');
    */
   }
   //Serial.println();
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
  Serial.println("Splashing Down");
  splash_down = true;

  if(!priorityCompressor.isEmpty())
  {
    Packet p = priorityCompressor.readIntoPacket();
    Serial.println("Priority Packet added");
    priority_queue.push(p);
    Serial.println("Saving packet");
    save_packet(p);
    num_packets++;
  }

  if(!compressor.isEmpty())
  {
    Packet p = compressor.readIntoPacket();
    Serial.println("Packet added");
    message_queue.push(p);
    Serial.println("Saving packet");
    save_packet(p);
    num_packets++;
  }

  Serial.print("Total Packets: ");
  Serial.println(num_packets);
  Serial.print("In queue: ");
  Serial.println(message_queue.count());
  Serial.print("In  priority_queue: ");
  Serial.println(priority_queue.count());
}

///run if packet is not sent on a try
bool ISBDCallback() {
    //reports that we have entered call back
    if(!inCallBack)
    {
      Serial.println("Call Back");
      inCallBack = true;
    }
    if(!splash_down) //if we haven't splashed down
    {
          do_tasks(); //make sure we are still reading in measurements and building packets
    }
    else if(!data_resend_complete) // if we have splashed down retransmit data
    {
      //loadFlashToQueue();
    }
    return true;
}

void setup() {

    Serial.begin(9600);
    Serial1.begin(19200);
    while(!Serial1);
    //sensors
    Serial.println("Setting up");
    init_Sensors();
    init_TC();
    init_accel_interrupt(1.75, .1, 0x00);       // set for launch detection
    init_gyro_interrupt(180, 0, 0x00);          // set for Ejection detection

    //iridium
    isbd.attachConsole(Serial);
    isbd.attachDiags(Serial);
    isbd.adjustSendReceiveTimeout(45);
    isbd.useMSSTMWorkaround(false);
    isbd.setPowerProfile(1);
        
    timeSinceR = 0;
    timeSinceP = 0;

    Serial.println("Starting modem");
    int err;
    do
    {
      err = isbd.begin(); //try to start modem
      if(err != 0) //if we have an error report it
      {
        Serial.print("Error: ");
        Serial.println(err);
      }
    }while(!(err == 0)); //if err == 0 we successfully started up
    //report starting up and time to start up
    Serial.print("Started: ");
    Serial.print(millis()/1000);
    Serial.println(" s");

    //flash chip
    //SerialFlash.begin(FLASH_PIN);
    Serial.println("Set up");
}

void loop() {
    //after ejection before splash_down routine
    if(!splash_down)
    {
      do_tasks(); //complete needed tasks for measurment
    }
    else if(!data_resend_complete) // if we have splashed down retransmit data
    {
      Serial.println("Resend");
      data_resend_complete = true;
      //loadFlashToQueue(); //reload saved data and retransmit
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
}
