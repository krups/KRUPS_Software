#include <Arduino.h>
//#include <SPI.h>

#include <KRUPS_TC.h>
#include <KRUPS_Sensor.h>
#include <compress.h>
//#include <Snooze.h>
#include <IridiumSBD.h>

volatile bool launched = false, ejected = false, splash_down = false;
uint8_t measure_buf[32]; // don't forget to change to a larger size
size_t loc = 0;
IridiumSBD isbd(Serial1);

void setup() {
    init_Sensors(); init_TC();
    init_accel_interrupt(1.75, .1, 0x00);       // set for launch detection
    init_gyro_interrupt(180, 0, 0x00);          // set for Ejection detection
    isbd.begin();
    isbd.adjustSendReceiveTimeout(45);
    isbd.useMSSTMWorkaround(false);
}

void loop() {
    // TODO: make splash-down detection routine
    if (splash_down) {
        // potentially start GPS ping and LED beacon
    }
    else if (ejected) {
        Read_gyro(measure_buf, loc);        // 6 bytes
        Read_loaccel(measure_buf, loc);     // 6 bytes
        Read_hiaccel(measure_buf, loc);     // 6 bytes
        Read_mag(measure_buf, loc);         // 6 bytes
        Read_TC(measure_buf, loc);          // 16 bytes

        // TODO: store the measurements in the EEPROM/FLASH
        // TODO: reorder packets for transmission

        // TODO: transmit data packets
        isbd.sendSBDBinary(measure_buf, loc);
    }
    else if (launched) {
        // wait for ejection
    }
}

void launch(void) {
    launched = true;
    read8(L3GD20_ADDRESS, GYRO_REGISTER_INT1_SRC);
}

bool ISBDCallback() {
    Read_gyro(measure_buf, loc);
    Read_loaccel(measure_buf, loc);
    Read_hiaccel(measure_buf, loc);
    Read_mag(measure_buf, loc);
    Read_TC(measure_buf, loc);


    return true;
}
