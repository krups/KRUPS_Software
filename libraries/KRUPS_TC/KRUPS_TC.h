// Library of wrapper functions for reading the thermalcouples
// on the KRUPS Breakout board
#ifndef KRUPS_TC_H
#define KRUPS_TC_H
#include <compress.h>
#include <SPI.h>

#define DEBUG		(0)

#define PINEN		(5)
#define PINMUX0		(2)
#define PINMUX1		(3)
#define PINCS1		(9)
#define PINCS2		(7)
#define PINCS3		(8)
#define PINSO		(12)
#define CLK			(13)

// initializes the Thermocouple converter and the associated multiplexors
void init_TC(void) {
	pinMode(PINEN, OUTPUT);
	pinMode(PINMUX0, OUTPUT);
	pinMode(PINMUX1, OUTPUT);
	pinMode(PINCS1, OUTPUT);
	pinMode(PINCS2, OUTPUT);
	pinMode(PINCS3, OUTPUT);

	digitalWrite(PINEN, HIGH);   		// enable on
	digitalWrite(PINMUX0, LOW);     	// low, low = channel 1
	digitalWrite(PINMUX1, LOW);
	SPI.begin();
}

// Sets the output channel of the multiplexors
void setMUX(int j) {
	switch (j) {
    case 0 :
		digitalWrite(PINMUX0, LOW);
		digitalWrite(PINMUX1, LOW);
		break;
    case 1 :
		digitalWrite(PINMUX0, HIGH);
		digitalWrite(PINMUX1, LOW);
		break;
    case 2 :
		digitalWrite(PINMUX0, LOW);
		digitalWrite(PINMUX1, HIGH);
		break;
    case 3 :
		digitalWrite(PINMUX0, HIGH);
		digitalWrite(PINMUX1, HIGH);
		break;
	}
}

// read the output of the thermocouple converter and return the value
// returns all ones (-.25 C) if an error was detected
int16_t spiread32(int PINCS) {
	digitalWrite(PINCS, LOW);
	SPI.beginTransaction(SPISettings(14000000, MSBFIRST, SPI_MODE0));
	int d = SPI.transfer(0x00);
	d = (d<<8) + SPI.transfer(0x00);
	d = (d<<8) + SPI.transfer(0x00);
	d = (d<<8) + SPI.transfer(0x00);
	SPI.endTransaction(); digitalWrite(PINCS, HIGH);
	if (d & 0x10000) {
		//d = 0xFFFFFFFF;
		if ( d & 0x0004 ) {
		d = -999999;			// acknowledge short to VCC error
		//Serial.print("SHORT to VCC ");
		}
		else if ( d & 0x0002 ){ 
		d = -99999;		// acknowledge short to GND error
				//Serial.print("SHORT to GND ");
			}
		else if ( d & 0x0001 ){
		 d = -9999;			// acknowledge open circuit error
				//Serial.print("OPEN circuit ");
			}
	}
	//else d = (d >> 18) & 0x00000FFFF;				// shift and mask return value
	return d >> 18;
}

// reads all thermocouples and appends the values to the input buffer
// and moves the location pointer appropriately, requires 16 free bytes in buf
void Read_TC(uint8_t *buf, size_t &loc) {
	int16_t temperatures[8];
	for (int i = 0; i < 4; i++) {
		setMUX(i); delay(100);
		temperatures[3-i] = spiread32(PINCS1);
		temperatures[4+i] = spiread32(PINCS2);
	}

	for (int j = 0; j < 8; j++) {
		append(buf, loc, temperatures[j]);
		#if DEBUG
		Serial.print(temperatures[j]);
		#endif
	}
}

void Read_TC_at_MUX(uint8_t *buf, size_t &loc)
{
	int16_t one = spiread32(PINCS1);
	int16_t two = spiread32(PINCS2);
	int16_t three = spiread32(PINCS3);
	
	Serial.print(float(one) *.25);
	Serial.print("\t");
	Serial.print (float(two)* .25);
	Serial.print("\t");
	Serial.println(float(three) * .25);
	
	append(buf, loc, one);
	append(buf, loc, two);
	append(buf, loc, three);
}

#endif
