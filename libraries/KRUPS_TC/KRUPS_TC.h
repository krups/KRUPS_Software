// Library of wrapper functions for reading the thermalcouples
// on the KRUPS Breakout board
#ifndef KRUPS_TC_H
#define KRUPS_TC_H
#include <compress.h>

#define DEBUG		(0)

#define PINEN		(5)
#define PINMUX0		(2)
#define PINMUX1		(3)
#define PINCS1		(8)
#define PINCS2		(7)
#define PINSO		(12)
#define CLK			(13)

// initializes the Thermocouple converter and the associated multiplexors
void init_TC(void) {
	pinMode(PINEN, OUTPUT);
	pinMode(PINMUX0, OUTPUT);
	pinMode(PINMUX1, OUTPUT);
	pinMode(PINSO, INPUT);
	pinMode(PINCS1, OUTPUT);
	pinMode(PINCS2, OUTPUT);
	pinMode(CLK, OUTPUT);

	digitalWrite(PINEN, HIGH);   	// enable on
	digitalWrite(PINMUX0, LOW);     	// low, low = channel 1
	digitalWrite(PINMUX1, LOW);
	digitalWrite(CLK, LOW);     	//put clock in low
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
	int i, d = 0;
	digitalWrite(PINCS, LOW);

	for (i = 31; i >= 0; i--) {
		digitalWrite(CLK, LOW);

		d <<= 1;
		if (digitalRead(PINSO)) {
			d |= 1;
			//Serial.print("1");
		}
		//else Serial.print("0");
		digitalWrite(CLK, HIGH);
	}
	if (d & 0x10000) {
		d = 0xFFFFFFFF;
		//if ( d & 0x0004 ) d = -999999;
		//else if ( d & 0x0002 ) d = -99999;
		//else if ( d & 0x0001 ) d = -9999;
	}
	//else d = (d >> 18) & 0x00000FFFF;

	digitalWrite(PINCS, HIGH);
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

#endif
