#include <Arduino.h>
#include <compress.h>
#include <IridiumSBD.h>
#include <QueueList.h>
#include <SerialFlash.h>
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

volatile bool launched = false, ejected = true, splash_down = false;
uint8_t measure_buf[BUFF_SIZE]; // holds readings to be stored in packets
size_t loc = 0; //location in the buffer
IridiumSBD isbd(Serial1);
int16_t num_packets = 0; //counter for number of packets to establish chronlogical order
int16_t measure_reads = 0;
QueueList<Packet> message_queue; //queue of messages left to send, messages pushed into front, pulled out back


//messages for non essential tasks
const uint8_t final_message[17] = {'C','O','M','E',' ','S','A','V','E',' ','M','E',' ','E','V','A','N'};
const int final_message_length = 17;

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

//REWRITE
//Writes the packet to the onboard flash chip
//prefers 256 byte chunks as the library says this amount is the most efficent
void save_packet(uint8_t* packet)
{
  String filename = "Packet_" + num_packets;
  SerialFlash.create(filename.c_str(), PACKET_SIZE);
  SerialFlashFile file = SerialFlash.open((filename + ".bin").c_str());
  for(int i = 0; i < PACKET_SIZE; i += 256)
  {
    if(PACKET_SIZE - i < 256) //if we don't have enough space for a full chunk to be written
    {
      file.write(&packet[i], PACKET_SIZE - i); //write to the end of the packet
    }
    else
    {
      file.write(&packet[i], 256); // else write a 256 chunk
    }
  }
}

/*
 * Tasks to be completed at each measurement cycle
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


void do_tasks()
{
        //Read_gyro(measure_buf, loc);        // 6 bytes
        Read_loaccel(measure_buf, loc);     // 6 bytes
        Read_mag(measure_buf, loc);         // 6 bytes
        //Read_TC(measure_buf, loc);          // 16 bytes
        //Read_hiaccel(measure_buf, loc);     // 6 bytes
        delay(400);
        Serial.println("Measured");
                
        //NOTE: make sure packet_size-header_size is a mutliple of the number of bytes in a read cycle
        //to make sure no splicing occurs
        if(loc >= PACKET_SIZE - HEADER_SIZE)  //if we have enough bytes to fill the packet (leaving space for the header)
        {
            //new packet
            Packet packet = Packet(num_packets, measure_buf, PACKET_SIZE); 
            message_queue.push(packet); //add to queue
            num_packets++; //we now have one more packet
            Serial.println("Packet Added");
            Serial.print("Total Packets: ");
            Serial.println(num_packets);
            Serial.print("In queue: ");
            Serial.println(message_queue.count());
            //save_packet(packet); //save to flash

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
 * Last important tasks that need 
 * to be completeded before power loss
 */
void final_essential_tasks()
{
  //empty the queue
  Serial.println("Final Empty");
  while(!message_queue.isEmpty())
  {
    startSendingMessage();
  }
}

/*
 * tasks to complete after all final essential tasks are completed
 * while run until power loss
 */
void final_nonessential_tasks()
{
  while(true) //run until power loss
  {
      //TODO: change to read files from the flash chip and retransmit them
      //send a message to allow gps data to used for possible retrival
      isbd.sendSBDBinary(final_message, final_message_length);
      Serial.println("DONE");
      delay(60000); //wait 1 minute
  }
}

/*
 * routine of tasks to be called once splash down has occured
 */
void splashDown()
{
  splash_down = true;
  final_essential_tasks();
  final_nonessential_tasks();  
}

///run if packet is not sent on a try
bool ISBDCallback() {
    Serial.println("Call Back");
    if(!splash_down) //if we haven't splashed down
    {
          do_tasks(); //make sure we are still reading in measurements and building packets
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
    
      //transmit data packets from the queue
      if(!message_queue.isEmpty())
      {
        startSendingMessage();
      }
    }
}
