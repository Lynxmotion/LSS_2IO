/*
 *  Author:     Brahim Daouas (support@robotshop.com)
 *  Version:    1.0.1
 *  Licence:    LGPL-3.0 (GNU Lesser General Public License version 3)
 *  
 *  Desscription: Example showing the functionalities of the LSS-2IO in Arduino and 2RC Modes.
 *  A SPDT switch changes between 2 modes : first mode allows to mimic an LSS position on
 *  an RC servo connected to an LSS 2IO. Second mode allows to query a Sharp GP2Y0A21YK0F sensor
 *  and move an LSS and an RC servo accordingly.
 */

#include <LSS.h>

#define LSS_BAUD	(LSS_DefaultBaud)

// Create LSS objects
LSS LSS_Servo = LSS(0);
LSS LSS_2RC_RC_Servo = LSS(209);
LSS LSS_2RC_Sharp = LSS(203);
LSS LSS_2RC_Led = LSS(207);

int USB_Tx_Buffer_enable_pin = 7; //Controls the tri-state buffer (IC4) on CH340E Tx / ATMega328P Rx serial line (pulled-up). (more information here : https://www.robotshop.com/info/wiki/lynxmotion/view/servo-erector-set-system/ses-electronics/ses-modules/lss-2io-board/#HArduino)

// Set 2IO RGB pins
int red_light_pin = 3;
int green_light_pin = 6;
int blue_light_pin = 5;

//LSS Mimic or Sharp selection mode button
int Button_pin = 11;

unsigned long lastTime = 0;
#define LSS_Interval  (10)  // ms

void setup()
{
	// Initialize the LSS bus
	LSS::initBus(Serial, LSS_BAUD); // Serial port initialization
	pinMode(0, INPUT_PULLUP); // Activate internal pull-up on ATMega328P Rx pin

	pinMode(USB_Tx_Buffer_enable_pin, OUTPUT);
	digitalWrite(USB_Tx_Buffer_enable_pin, LOW); //Set tri-state buffer (IC4) enable pin low to deassert CH340E Tx ( https://www.robotshop.com/info/wiki/lynxmotion/view/servo-erector-set-system/ses-electronics/ses-modules/lss-2io-board/#HArduino)

	//Set RGB pins as outputs (active low)
	pinMode(red_light_pin, OUTPUT);
	pinMode(green_light_pin, OUTPUT);
	pinMode(blue_light_pin, OUTPUT);
	digitalWrite(red_light_pin, HIGH);
	digitalWrite(green_light_pin, HIGH);
	digitalWrite(blue_light_pin, HIGH);

	pinMode(Button_pin, INPUT_PULLUP); // Activate internal pull-up for the switch input pin

	delay(2000);
	LSS_Servo.move(0);    // Rotate LSS to 0 degrees position
	LSS_2RC_RC_Servo.move(0); // Rotate RC servo to 0 degrees position

	// Wait for it to get there
	delay(1000);
	LSS_Servo.limp();

	//Change control mode
	Serial.write("#0EM0\r");
	Serial.write("#0IPE0\r");
}

void loop()
{
	if (millis() > (lastTime + LSS_Interval))
	{
		// Reset counter to current time
		lastTime = millis();

		switch (digitalRead(Button_pin))
		{    // Mode selection
			case 0:
				Sharp();
				break;
			case 1:
				LSS_Mimic();
				break;
		}
	}
}

void LSS_Mimic()
{
	// update LEDs
	LSS_2RC_Led.setColorLED(LSS_LED_Black);
	LSS_Servo.setColorLED(LSS_LED_Green);
	RGB_color(1, 0, 1);

	LSS_Servo.limp();
	uint16_t distance = (uint16_t) query_LSS();
	LSS_2RC_RC_Servo.move(distance);  // update RC servo position
}

void RGB_color(int red_light, int green_light, int blue_light)  // RGB control function
{
	digitalWrite(red_light_pin, red_light);
	digitalWrite(green_light_pin, green_light);
	digitalWrite(blue_light_pin, blue_light);
}

void Sharp()
{
	// update LEDs
	LSS_Servo.setColorLED(LSS_LED_Black);
	RGB_color(1, 1, 1);
	LSS_2RC_Led.setColorLED(LSS_LED_Green);

	uint16_t distance = (uint16_t) query_Sharp();

	if (distance >= 350)
	{ // If Sharp reading is out of range LSS and RC goes to 0
		distance = 0;
		LSS_Servo.move(distance);
		LSS_2RC_RC_Servo.move(distance);
	}
	else
	{                 // Update servos poisitions
		distance = constrain(distance, 100, 300);
		distance = map(distance, 100, 300, -900, 900);
		LSS_Servo.move(distance);
		LSS_2RC_RC_Servo.move(distance);
	}
}

int32_t query_Sharp()
{
	int32_t posLSS = 0;
	int32_t lastPosLSS = -1;
	char readBuffer[100];
	lastPosLSS = posLSS;
	Serial.write("#203QA2\r");
	while (Serial.available() == 0)
	{
	}
	size_t readLength = 0;
	readLength = Serial.readBytesUntil('A', readBuffer, 100); // Read until after the command (QA), indicating the start of the returned value (position in 1/10 deg)
	readLength = Serial.readBytesUntil('\r', readBuffer, 100); // Read until the carriage return (\r), indicating the end of the reply
	readBuffer[readLength] = 0;
	if (readLength > 0)
	{
		if (LSS_2RC_Sharp.charToInt(readBuffer, &posLSS))
		{
			return posLSS;
		}
	}
}

int32_t query_LSS()
{
	int32_t posLSS = 0;
	int32_t lastPosLSS = -1;
	char readBuffer[100];
	lastPosLSS = posLSS;
	Serial.write("#0QD\r");
	while (Serial.available() == 0)
	{
	}
	size_t readLength = 0;
	readLength = Serial.readBytesUntil('D', readBuffer, 100); // Read until after the command (QD), indicating the start of the returned value (position in 1/10 deg)
	readLength = Serial.readBytesUntil('\r', readBuffer, 100); // Read until the carriage return (\r), indicating the end of the reply
	readBuffer[readLength] = 0;
	if (readLength > 0)
	{
		if (LSS_2RC_Sharp.charToInt(readBuffer, &posLSS))
		{
			return posLSS;
		}
	}
}
