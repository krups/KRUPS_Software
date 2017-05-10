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
    isbd.begin();
    isbd.adjustSendReceiveTimeout(45);
    isbd.useMSSTMWorkaround(false);
    isbd.setPowerProfile(0);
}


void do_tasks()
{
        Read_gyro(measure_buf, loc);        // 6 bytes
        Read_loaccel(measure_buf, loc);     // 6 bytes
        //Read_hiaccel(measure_buf, loc);     // 6 bytes
        Read_mag(measure_buf, loc);         // 6 bytes
        Read_TC(measure_buf, loc);          // 16 bytes
        // TODO: store the measurements in the EEPROM/FLASH
        // TODO: reorder packets for transmission
        
        //NOTE: CHANGE TO LESS STRINGENT CONDITION make it a multiple of total read size
        if(loc >= PACKET_SIZE - HEADER_SIZE)  //if we have enough bytes to fill the packet (leaving space for the header)
        {
            //TODO: make any edits to packet header etc
            //use counter to number packet
            uint8_t packet[PACKET_SIZE]; 
            size_t p_loc = 0;

            //add header
            append(packet, p_loc, num_packets); // add counter to the front of the packet

            //copy readings
            for(int i = 0; i < PACKET_SIZE-HEADER_SIZE; i++)
            {
                packet[p_loc] = measure_buf[i];
                p_loc++;
            }

            message_queue.push(packet);
            Serial.println("Packet added");
            num_packets++; //we now have one more packet

            //reset where the sensors are writing to in the buffer
            loc = 0;
        }
}

void loop() {
    // TODO: make splash-down detection routine
    if (splash_down) {
        // potentially start GPS ping and LED beacon
    }
    else if (ejected) {
        do_tasks(); //complete needed tasks for measurment
        // TODO: transmit data packets
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
    //read8(L3GD20_ADDRESS, GYRO_REGISTER_INT1_SRC);
}

///run if packet is not sent on a try
bool ISBDCallback() {
    do_tasks(); //make sure we are still reading in measurements and building packets
    return true;
}
