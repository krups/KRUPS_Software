#include <Arduino.h>
#include <KRUPS_TC.h>
#include <KRUPS_Sensor.h>
#include <compress.h>
#include <IridiumSBD.h>
#include <QueueList.h>
#include <SerialFlash.h>


#define BUFF_SIZE   (430) //size of the measurement buffer
#define PACKET_SIZE (44) //size of a packets to send
#define HEADER_SIZE (2)  //number of bytes taken up in the packet by buffer
#define TC_NUM (12) //number of TC for testing
#define FLASH_PIN (0) //select pin for the flash chip

volatile bool launched = false, ejected = true, splash_down = false;
uint8_t measure_buf[BUFF_SIZE]; // don't forget to change to a larger size
size_t loc = 0;
IridiumSBD isbd(Serial1);
int16_t num_packets = 0; //counter for number of packets to establish chronlogical order
QueueList<uint8_t*> message_queue; //queue of messages left to send, messages pushed into front, pulled out back

//Fake read funcitons for testing 
/*
void Read_gyro(uint8_t *buf, size_t &loc)
{
    append(buf, loc, 0+num_packets);
    append(buf, loc, 1+num_packets);
    append(buf, loc, 2+num_packets);
}

void Read_loaccel(uint8_t *buf, size_t &loc)
{
    append(buf, loc, 10+num_packets);
    append(buf, loc, 11+num_packets);
    append(buf, loc, 12+num_packets);
}

void Read_mag(uint8_t *buf, size_t &loc)
{
    append(buf, loc, 20+num_packets);
    append(buf, loc, 21+num_packets);
    append(buf, loc, 22+num_packets);
}

void Read_TC(uint8_t *buf, size_t &loc)
{
  for(int i = 0; i < TC_NUM; i++)
  {
    append(buf, loc, i*7 + num_packets);
  }
  delay(TC_NUM/3 * 100);
}
*/

void setup() {
    init_Sensors(); init_TC();
    init_accel_interrupt(1.75, .1, 0x00);       // set for launch detection
    init_gyro_interrupt(180, 0, 0x00);          // set for Ejection detection

    //iridium
    isbd.begin();
    isbd.adjustSendReceiveTimeout(45);
    isbd.useMSSTMWorkaround(false);
    isbd.setPowerProfile(0);

    //flash chip
    SerialFlash.begin(FLASH_PIN);
}


void do_tasks()
{
        Read_gyro(measure_buf, loc);        // 6 bytes
        Read_loaccel(measure_buf, loc);     // 6 bytes
        Read_mag(measure_buf, loc);         // 6 bytes
        Read_TC(measure_buf, loc);          // 16 bytes
        //Read_hiaccel(measure_buf, loc);     // 6 bytes
        // TODO: store the measurements in the EEPROM/FLASH
                
        //NOTE: make sure packet_size-header_size is a mutliple of the number of bytes in a read cycle
        //to make sure no splicing occurs
        if(loc >= PACKET_SIZE - HEADER_SIZE)  //if we have enough bytes to fill the packet (leaving space for the header)
        {
            //new packet
            uint8_t packet[PACKET_SIZE]; 
            size_t p_loc = 0;

            //header
            append(packet, p_loc, num_packets); // add counter to the front of the packet
            //end header

            //copy readings
            for(int i = 0; i < PACKET_SIZE-HEADER_SIZE; i++)
            {
                packet[p_loc] = measure_buf[i];
                p_loc++;
            }
            
            //add to queue
            message_queue.push(packet);

            //write packet the flash chip
            String filename = "Packet_" + num_packets;
            SerialFlash.create(filename, PACKET_SIZE);
            SerialFlashFile file = SerialFlash.open(filename + ".bin");
            for(int i = 0; i < PACKET_SIZE; i = i + 256)
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
            
            num_packets++; //we now have one more packet
            loc = 0;  //reset where the sensors are writing to in the buffer
        }
}

void loop() {
    // TODO: make splash-down detection routine
    if (splash_down) {
        // potentially start GPS ping and LED beacon
    }
    else if (ejected) {
        do_tasks(); //complete needed tasks for measurment

        //transmit data packets from the queue
        if(!message_queue.isEmpty())
        {
            uint8_t* packet;
            packet = message_queue.peek();
            uint16_t loc;
            isbd.sendSBDBinary(packet, loc);
            message_queue.pop();
        }
    }
    else if (launched) {
        // wait for ejection
    }
}

void launch(void) {
    launched = true;
    read8(L3GD20_ADDRESS, GYRO_REGISTER_INT1_SRC);
}

///run if packet is not sent on a try
bool ISBDCallback() {
    do_tasks(); //make sure we are still reading in measurements and building packets
    return true;
}
