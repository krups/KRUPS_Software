#include <Arduino.h>
#include <compress.h>
#include <IridiumSBD.h>
#include <QueueList.h>
#include <SerialFlash.h>
#include <String>
#include "Packet.h"

#define TEST (false) //decides to use real or fake functions to allow testing
#if TEST
#include "Debug.h"
#else
#include <KRUPS_TC.h>
#include <KRUPS_Sensor.h>
#endif


#define BUFF_SIZE   (2000) //size of the measurement buffer
#define FLASH_PIN (0) //select pin for the flash chip
#define TIME_TO_SPLASH_DOWN (350000) //Time until splash down routine begins in ms

volatile bool launched = false, ejected = true, splash_down = false, data_resend_complete = false;
uint8_t measure_buf[BUFF_SIZE]; // holds readings to be stored in packets
size_t loc = 0; //location in the buffer
IridiumSBD isbd(Serial1);
int16_t num_packets = 0; //counter for number of packets to establish chronlogical order
int16_t measure_reads = 0;
QueueList<Packet> message_queue; //queue of messages left to send, messages pushed into front, pulled out back
bool inCallBack = false;

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

//Writes the packet to the onboard flash chip
//prefers 256 byte chunks as the library says this amount is the most efficent
void save_packet(Packet packet)
{
  String filename = "Packet_" + String(num_packets);
  SerialFlash.create(filename.c_str(), PACKET_SIZE);
  SerialFlashFile file = SerialFlash.open((filename + ".bin").c_str());
  for(int i = 0; i < PACKET_SIZE; i += 256)
  {
    if(PACKET_SIZE - i < 256) //if we don't have enough space for a full chunk to be written
    {
      file.write(packet.getArrayAt(i), PACKET_SIZE - i); //write to the end of the packet
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
    String filename = "Packet_" + String(num_packets) + ".bin";
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
     Packet packet = Packet(loadedCount, measure_buf, PACKET_SIZE);
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
void startSendingMessage()
{
  //grab a packet
  Packet packet = message_queue.peek();
  uint16_t loc  = PACKET_SIZE;
  //attempt to send
  int response = isbd.sendSBDBinary(packet.getArrayBase(), loc);
  //if it returns 0 message is sent
  if(response == 0)
  {
    message_queue.pop(); //remove message from the list
    printPacket(packet, PACKET_SIZE);
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
        //Read_gyro(measure_buf, loc);        // 6 bytes
        Read_loaccel(measure_buf, loc);     // 6 bytes
        Read_mag(measure_buf, loc);         // 6 bytes
        //Read_TC(measure_buf, loc);          // 16 bytes
        //Read_hiaccel(measure_buf, loc);     // 6 bytes
        delay(400);
        measure_reads++;
        if(measure_reads > 10)
        {
          Serial.println("Measuring");
          measure_reads = 0;
        }
                
        //NOTE: make sure packet_size-header_size is a mutliple of the number of bytes in a read cycle
        //to make sure no splicing occurs
        if(loc >= PACKET_SIZE - HEADER_SIZE)  //if we have enough bytes to fill the packet (leaving space for the header)
        {
            //new packet
            Packet packet = Packet(num_packets, measure_buf, PACKET_SIZE); 
            Serial.println("Packet Added");
            message_queue.push(packet); //add to queue
            Serial.print("Saving packet...");
            //save_packet(packet); //save to flash
            num_packets++; //we now have one more packet
            Serial.println("done");
            Serial.print("Total Packets: ");
            Serial.println(num_packets);
            Serial.print("In queue: ");
            Serial.println(message_queue.count());

            //check if we've splashed down
            if(millis() > TIME_TO_SPLASH_DOWN)
            {
              Serial.println("Splashing Down");
              splashDown();
            }
            
            loc = 0;  //reset where the sensors are writing to in the buffer
        }
}

/*
 * routine of tasks to be called once splash down has occured
 */
void splashDown()
{
  splash_down = true; 
}

///run if packet is not sent on a try
bool ISBDCallback() {
    if(!inCallBack)
    {
      Serial.println("Call Back");
      inCallBack = true;
    }
    if(!splash_down) //if we haven't splashed down
    {
          do_tasks(); //make sure we are still reading in measurements and building packets
    }
    else if(!data_resend_complete)
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
    else if(!data_resend_complete)
    {
      //loadFlashToQueue(); //reload saved data and retransmit
    }
    
    if(!message_queue.isEmpty()) //if there is data to send, always be trying to send it
    {
      startSendingMessage();
      inCallBack = false;
    }
}
