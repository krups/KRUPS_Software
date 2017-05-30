#include <Arduino.h>
#include <compress.h>
#include <IridiumSBD.h>
#include <QueueList.h>
#include <SerialFlash.h>
#include <String>
#include "Packet.h"
#include "BufferedCompressor.h"

#define TEST (false) //decides to use real or fake functions to allow testing
#if TEST
#include "Debug.h"
#else
#include <KRUPS_TC.h>
#include <KRUPS_Sensor.h>
#endif


#define BUFFER_SIZE   (2000) //size of the measurement buffer
#define FLASH_PIN (0) //select pin for the flash chip
#define TIME_TO_SPLASH_DOWN (350000) //Time until splash down routine begins in ms

volatile bool launched = false, ejected = true, splash_down = false, data_resend_complete = false;
uint8_t measure_buf[BUFFER_SIZE]; // holds readings to be stored in packets
size_t loc = 0; //location in the buffer
IridiumSBD isbd(Serial1);
int16_t num_packets = 0; //counter for number of packets to establish chronlogical order
int16_t measure_reads = 0; //tells which cycle read we are on (0-3) to determine where to put data
QueueList<Packet> message_queue; //queue of messages left to send, messages pushed into front, pulled out back
QueueList<Packet> priority_queue; //queue of messages to send with higher priority
bool inCallBack = false;
BufferedCompressor compressor = BufferedCompressor(); // compressor for the packets
BufferedCompressor priorityCompressor = BufferedCompressor(); //compressor for priority packets

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
  uint8_t zeroByte = time | 0xFF;
  uint8_t oneByte = time >> 8;
  uint8_t twoByte = time >> 16;
  buff[loc] = zeroByte;
  loc++;
  buff[loc] = oneByte;
  loc++;
  buff[loc] = twoByte;
  loc++
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
void startSendingMessage(bool priority)
{
  //grab a packet
  Packet packet;
  if(priority)
  {
    packet = priority_queue.peek();
  }
  else
  {
    packet = message_queue.peek();
  }
  uint16_t loc  = packet.getLength();
  //attempt to send
  int response = isbd.sendSBDBinary(packet.getArrayBase(), loc);
  //if it returns 0 message is sent
  if(response == 0)
  {
    message_queue.pop(); //remove message from the list
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

/*
 * Tasks to be completed at each measurement cycle
 */
void do_tasks()
{
        save_time(measure_buf,loc); // 3 bytes
        //Read_gyro(measure_buf, loc);        // 6 bytes
        Read_loaccel(measure_buf, loc);     // 6 bytes
        Read_mag(measure_buf, loc);         // 6 bytes
        //Read_TC(measure_buf, loc);          // 16 bytes
        //Read_hiaccel(measure_buf, loc);     // 6 bytes
        delay(400);

        //sink data into the correct compressor
        if(measure_reads == 0)
        {
            priorityCompressor.sink(measure_buf, loc);
        }
        else
        {
          compressor.sink(measure_buf, loc); //puts the data into the compressor buffer
        }
        measure_reads++ //incremeant measure_reads
        if(measure_reads > 3) //handle wrap around
        {
          measure_reads = 0;
        }
        loc = 0; //reset where data is being read into the buffer

        if(priorityCompressor.isFull()) //if the full bit is set we need to load out the data
        {
          Packet p = priorityCompressor.readIntoPacket()
          Serial.println("Priority Packet added");
          priority_queue.push(p);
          Serial.println("Saving packet");
          save_packet(p);
          num_packets++;
          Serial.print("Total Packets: ");
          Serial.println(num_packets);
          Serial.print("In  priority_queue: ");
          Serial.println(priority_queue.count());
          Serial.print("In queue: ");
          Serial.println(message_queue.count());
        }

        if(compressor.isFull()) //if the full bit is set we need to load out the data
        {
          Packet p = compressor.readIntoPacket()
          Serial.println("Packet added");
          message_queue.push(p);
          Serial.println("Saving packet");
          save_packet(p);
          num_packets++;
          Serial.print("Total Packets: ");
          Serial.println(num_packets);
          Serial.print("In queue: ");
          Serial.println(message_queue.count());
          Serial.print("In  priority_queue: ");
          Serial.println(priority_queue.count());
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
    Packet p = priorityCompressor.readIntoPacket()
    Serial.println("Priority Packet added");
    priority_queue.push(p);
    Serial.println("Saving packet");
    save_packet(p);
    num_packets++;
  }

  if(!compressor.isEmpty())
  {
    Packet p = compressor.readIntoPacket()
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
  Serial.println(priority_queue.count())
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
      //loadFlashToQueue(); //reload saved data and retransmit
    }

    if(!priority_queue.isEmpty()) //if there is priority data to send, do so
    {
      startSendingMessage(true); //send a message from the priority list
      inCallBack = false;
    }
    else if(!message_queue.isEmpty()) //if no priority data, check regular data
    {
      startSendingMessage(false); //send a message from the non priority list
      inCallBack = false;
    }
}
