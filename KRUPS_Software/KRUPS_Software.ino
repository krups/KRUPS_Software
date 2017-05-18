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


#define BUFF_SIZE   (1960) //size of the measurement buffer
#define FLASH_PIN (0) //select pin for the flash chip
#define TIME_TO_SPLASH_DOWN (350000) //Time until splash down routine begins in ms

volatile bool launched = false, ejected = true, splash_down = false;
uint8_t measure_buf[BUFF_SIZE]; // holds readings to be stored in packets
size_t loc = 0; //location in the buffer
//IridiumSBD isbd(Serial1);
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
void do_tasks()
{
        Serial.println("MEASURING");
        //Read_gyro(measure_buf, loc);        // 6 bytes
        Read_loaccel(measure_buf, loc);     // 6 bytes
        Read_mag(measure_buf, loc);         // 6 bytes
        //Read_TC(measure_buf, loc);          // 16 bytes
        //Read_hiaccel(measure_buf, loc);     // 6 bytes
        delay(400);


        if(millis() > TIME_TO_SPLASH_DOWN)
        {
          splashDown();
        }
                
        //NOTE: make sure packet_size-header_size is a mutliple of the number of bytes in a read cycle
        //to make sure no splicing occurs
        if(loc >= PACKET_SIZE - HEADER_SIZE)  //if we have enough bytes to fill the packet (leaving space for the header)
        {
            //new packet
            Packet packet = Packet(num_packets, measure_buf, PACKET_SIZE); 
            message_queue.push(packet); //add to queue
            //save_packet(packet); //save to flash
            num_packets++; //we now have one more packet
            loc = 0;  //reset where the sensors are writing to in the buffer
            //measure_reads = 0;
        }
}

/*
 * Last important tasks that need 
 * to be completeded before power loss
 */
void final_essential_tasks()
{
  //fill the rest of the packet with junk so we dont get old data
  for(int i = loc; i < PACKET_SIZE; i++)
  {
    measure_buf[i] = 255;
  }

  //pack the packet
  Packet packet = Packet(num_packets, measure_buf, PACKET_SIZE); 
  message_queue.push(packet); //add to queue
  //save_packet(packet); //save to flash

  //empty the queue
  while(!message_queue.isEmpty())
  {
      Packet currPacket = message_queue.peek();
      uint16_t loc = PACKET_SIZE;
      //isbd.sendSBDBinary(currPacket, loc);
      printPacket(currPacket, PACKET_SIZE);
      message_queue.pop();
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
      //send a message to allow gps data to used for possible retrival
      //isbd.sendSBDBinary(final_message, final_message_length);
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
    while(!Serial);
    Serial.println("1");
    //sensors
    init_Sensors(); 
    Serial.println("2");
    //init_TC();
    Serial.println("3");
    init_accel_interrupt(1.75, .1, 0x00);       // set for launch detection
    Serial.println("4");
    //init_gyro_interrupt(180, 0, 0x00);          // set for Ejection detection
    Serial.println("5");

    //iridium
    /*
    isbd.begin();
    isbd.adjustSendReceiveTimeout(45);
    isbd.useMSSTMWorkaround(false);
    isbd.setPowerProfile(0);
    */
    
    //flash chip
    //SerialFlash.begin(FLASH_PIN);
}

void loop() {
    Serial.println("LOOOOP");
    //after ejection before splash_down routine
    if(!splash_down)
    {
      do_tasks(); //complete needed tasks for measurment
    
      //transmit data packets from the queue
      if(!message_queue.isEmpty())
      {
          Packet packet = message_queue.peek();
          uint16_t loc  = PACKET_SIZE;
          //isbd.sendSBDBinary(packet, loc);
          printPacket(packet, PACKET_SIZE);
          message_queue.pop();
      }
    }
}
