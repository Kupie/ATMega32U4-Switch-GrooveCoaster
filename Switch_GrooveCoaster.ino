/*

Debouncing based Heavily on this tutorial page
https://arduinogetstarted.com/tutorials/arduino-button-debounce

 */

// set to 'true' for debug on, 'false' for debug off
#define DEBUG false
#define flashIterations 10000

#include "LUFAConfig.h"
#include <LUFA.h>
#include "Joystick.h"

const int DEBOUNCE_DELAY = 5;	 // the debounce time; increase if the output flickers

#define DPAD_UP_MASK 0x00
#define DPAD_UPRIGHT_MASK 0x01
#define DPAD_RIGHT_MASK 0x02
#define DPAD_DOWNRIGHT_MASK 0x03
#define DPAD_DOWN_MASK 0x04
#define DPAD_DOWNLEFT_MASK 0x05
#define DPAD_LEFT_MASK 0x06
#define DPAD_UPLEFT_MASK 0x07
#define DPAD_NOTHING_MASK 0x08
#define Y_MASK 0x01
#define B_MASK 0x02
#define A_MASK 0x04
#define X_MASK 0x08
#define LB_MASK 0x10
#define RB_MASK 0x20
#define ZL_MASK 0x40
#define ZR_MASK 0x80
#define SELECT_MASK 0x100
#define START_MASK 0x200
#define L3_MASK 0x400
#define R3_MASK 0x800
#define HOME_MASK 0x1000
#define CAPTURE_MASK 0x2000

// Number of inputs
const int y = 11;

// Mappings for inputs to each button
//	First 4 are dpad:		Up,	Dwn,	L,	R,	A,	B,	X,	Y,	LB,	RB, Plus
//							 
const int Inputs[y] =		{3,	2,		5,	4,	14,	16,	10,	15,	A0,	A1, 9};

// Variables that will change:
int currentState[y];
int lastFlickerableState[y];
bool lastSteadyState[y];

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime[y];	// the last time the output pin was toggled


// Some setup needs to run in the loop() due to variables, but also run only once
boolean oneTime = true;

static int rightLedPrevious = LOW;
static int leftLedPrevious = LOW;
static int flashingLedNum = 0;

void setup() {
	// initialize the pushbutton pin as an pull-up input
	// the pull-up input pin will be HIGH when the switch is open and LOW when the switch is closed.
	for (int x = 0; x < y; x++) {
		pinMode(Inputs[x], INPUT_PULLUP);
	}
	pinMode(A2, OUTPUT);
	pinMode(A3, OUTPUT);
	// See how fast it blinks
	pinMode(8, OUTPUT);
	digitalWrite(8, LOW);
	//Only for testing outputs to LED
//	digitalWrite(A2, HIGH);
//	digitalWrite(A3, HIGH);
	SetupHardware();
	GlobalInterruptEnable();
}


void loop() {
	HID_Task();
	
    // We also need to run the main USB management task.
    USB_USBTask();
	
	if (oneTime) {
		oneTime = false;
		for (int x = 0; x < y; x++) {
			int lastFlickerableState[x] = {LOW};
			bool lastSteadyState[x] = {false};
		}
		

	}
	// Toggle LED for each 1000 iterations of the loop if DEBUG is true
	if (DEBUG) {
		flashingLedNum += 1;
		if (flashingLedNum == flashIterations) {
			digitalWrite(8, !digitalRead(8));
			flashingLedNum = 0;
		}
		
	}
	// Debounce the input vars and then store them to lastSteadyState[x]
	for (int x = 0; x < y; x++) {
		// read the state of the switch/button x:
		currentState[x] = digitalRead(Inputs[x]);
		
		// check to see if you just pressed the button
		// (i.e. the input went from LOW to HIGH), and you've waited long enough
		// since the last press to ignore any noise:
		
		// If the switch/button changed, due to noise or pressing:
		if (currentState[x] != lastFlickerableState[x]) {
			// reset the debouncing timer
			lastDebounceTime[x] = millis();
			// save the the last flickerable state
			lastFlickerableState[x] = currentState[x];
		}
		
		if ((millis() - lastDebounceTime[x]) > DEBOUNCE_DELAY) {
			// whatever the reading is at, it's been there for longer than the debounce
			// delay, so set the lastSteadyState to the current state
			
			if (currentState[x] == LOW) {lastSteadyState[x] = true;}
			else {lastSteadyState[x] = false;}
		}
	}
	
	// Go through the inputs and output the stuffs.
	
	// Read DPAD and set dpad mask. Inputs go in order for lastSteadyState:
	// Up, Down, Left, Rright, A, B, X, Y, LB, RB
	//  0,    1,    2,      3, 4, 5, 6, 7,  8,  9
	byte dpadMask =	B00000000;
	if ((lastSteadyState[0]) && (lastSteadyState[3])) 		{ReportData.HAT  = DPAD_UPRIGHT_MASK;}
	else if ((lastSteadyState[1]) && (lastSteadyState[3]))	{ReportData.HAT = DPAD_DOWNRIGHT_MASK;} 
	else if ((lastSteadyState[1]) && (lastSteadyState[2]))	{ReportData.HAT = DPAD_DOWNLEFT_MASK;}
	else if ((lastSteadyState[0]) && (lastSteadyState[2]))	{ReportData.HAT = DPAD_UPLEFT_MASK;}
	else if (lastSteadyState[0]) 							{ReportData.HAT = DPAD_UP_MASK;}
	else if (lastSteadyState[1]) 							{ReportData.HAT = DPAD_DOWN_MASK;}
	else if (lastSteadyState[2]) 							{ReportData.HAT = DPAD_LEFT_MASK;}
	else if (lastSteadyState[3]) 							{ReportData.HAT = DPAD_RIGHT_MASK;}
	else{ReportData.HAT = DPAD_NOTHING_MASK;}
	
	
	
	uint16_t buttonMask = 0x0;
	if (lastSteadyState[4]) {buttonMask |= A_MASK;}
	if (lastSteadyState[5]) {buttonMask |= B_MASK;}
	if (lastSteadyState[6]) {buttonMask |= X_MASK;}
	if (lastSteadyState[7]) {buttonMask |= Y_MASK;}
	if (lastSteadyState[10]) {buttonMask |= START_MASK;}
	
//	if (lastSteadyState[8]) {buttonMask |= LB_MASK;}
//	if (lastSteadyState[9]) {buttonMask |= RB_MASK;}

	int rightLed = LOW;
	int leftLed = LOW;
//	LED lightup code
	if (lastSteadyState[8]) {
		buttonMask |= LB_MASK;
		leftLed = HIGH;
		}
	if (lastSteadyState[9]) {
		buttonMask |= RB_MASK;
		rightLed = HIGH;
	}
	if (lastSteadyState[10] && DEBUG) {
		leftLed = HIGH;
		rightLed = HIGH;
	}

	// Only digitalWrite if the LED statuses have changed from last time
	if (rightLedPrevious != leftLed) {
		digitalWrite(A2, leftLed);
		leftLedPrevious = leftLed;
	}
	
	if (leftLedPrevious != rightLed) {
		digitalWrite(A3, rightLed);
		rightLedPrevious = rightLed;
	}
	
	ReportData.Button = buttonMask;

	
	// Set joysticks to neutral
	{ReportData.RX = 128;ReportData.RY = 128;}
	{ReportData.LX = 128;ReportData.LY = 128;}

}
