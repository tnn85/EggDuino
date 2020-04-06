/* Eggduino-Firmware by Joachim Cerny, 2014

   Thanks for the nice libs ACCELSTEPPER and SERIALCOMMAND, which made this project much easier.
   Thanks to the Eggbot-Team for such a funny and enjoable concept!
   Thanks to my wife and my daughter for their patience. :-)

 */

// implemented Eggbot-Protocol-Version v13
// EBB-Command-Reference, I sourced from: http://www.schmalzhaus.com/EBB/EBBCommands.html
// no homing sequence, switch-on position of pen will be taken as reference point.
// No collision-detection!!
// Supported Servos: I do not know, I use Arduino Servo Lib with TG9e- standard servo.
// Note: Maximum-Speed in Inkscape is 1000 Steps/s. You could enter more, but then Pythonscript sends nonsense.
// EBB-Coordinates are coming in for 16th-Microstepmode. The Coordinate-Transforms are done in weired integer-math. Be careful, when you diecide to modify settings.

/* TODOs:
   1	collision control via penMin/penMax
   2	implement homing sequence via microswitch or optical device
 */

#include "AccelStepper.h" // nice lib from http://www.airspayce.com/mikem/arduino/AccelStepper/
#include <Servo.h>
#include "SerialCommand.h" //nice lib from Stefan Rado, https://github.com/kroimon/Arduino-SerialCommand
#include <avr/eeprom.h>
#include "button.h"

#define initSting "EBBv13_and_above Protocol emulated by Eggduino-Firmware V1.6a"
//Rotational Stepper:
#define step1 A3 //2 //Changed Pinout for MPCNC Nano Estlcam Shield X-Axis STEP
#define dir1 A0 //5 //Changed Pinout for MPCNC Nano Estlcam Shield X-Axis DIR
#define enableRotMotor 0 //8 //Changed Pinout for MPCNC Nano Estlcam Shield 
#define rotMicrostep 16  //MicrostepMode, only 1,2,4,8,16 allowed, because of Integer-Math in this Sketch
//Pen Stepper:
#define step2 A4 //3 //Changed Pinout for MPCNC Nano Estlcam Shield Y-Axis STEP
#define dir2 A1 //6 //Changed Pinout for MPCNC Nano Estlcam Shield Y-Axis DIR
#define enablePenMotor 0 //8 //Changed Pinout for MPCNC Nano Estlcam Shield 
#define penMicrostep 16 //MicrostepMode, only 1,2,4,8,16 allowed, because of Integer-Math in this Sketch

#define servoPin 5 //4 //Servo //Changed Pinout for MPCNC Nano Estlcam Shield SPINDLE RPM (PWM) Signal

// EXTRAFEATURES - UNCOMMENT TO USE THEM -------------------------------------------------------------------

// #define prgButton 8 //2 // PRG button //Changed Pinout for MPCNC Nano Estlcam Shield IN1
// #define penToggleButton 9 //12 // pen up/down button //Changed Pinout for MPCNC Nano Estlcam Shield IN2 
// #define motorsButton 4 // motors enable button // Not available on MPCNC Nano Estlcam Shield. ENABLE is always pulled high (Motors always on)

//-----------------------------------------------------------------------------------------------------------

#define penUpPosEEAddress ((uint16_t *)0)
#define penDownPosEEAddress ((uint16_t *)2)

//make Objects
AccelStepper rotMotor(1, step1, dir1);
AccelStepper penMotor(1, step2, dir2);
Servo penServo;
SerialCommand SCmd;
//create Buttons
#ifdef prgButton
	Button prgButtonToggle(prgButton, setprgButtonState);
#endif
#ifdef penToggleButton
	Button penToggle(penToggleButton, doTogglePen);
#endif
#ifdef motorsButton
	Button motorsToggle(motorsButton, toggleMotors);
#endif
// Variables... be careful, by messing around here, everything has a reason and crossrelations...
int penMin=0;
int penMax=0;
int penUpPos=5;  //can be overwritten from EBB-Command SC
int penDownPos=20; //can be overwritten from EBB-Command SC
int servoRateUp=0; //from EBB-Protocol not implemented on machine-side
int servoRateDown=0; //from EBB-Protocol not implemented on machine-side
long rotStepError=0;
long penStepError=0;
int penState=penUpPos;
uint32_t nodeCount=0;
unsigned int layer=0;
boolean prgButtonState=0;
uint8_t rotStepCorrection = 16/rotMicrostep ; //devide EBB-Coordinates by this factor to get EGGduino-Steps
uint8_t penStepCorrection = 16/penMicrostep ; //devide EBB-Coordinates by this factor to get EGGduino-Steps
float rotSpeed=0;
float penSpeed=0; // these are local variables for Function SteppermotorMove-Command, but for performance-reasons it will be initialized here
boolean motorsEnabled = 0;

void setup() {   
	Serial.begin(9600);
	makeComInterface();
	initHardware();
}

void loop() {
	moveOneStep();

	SCmd.readSerial();

#ifdef penToggleButton
	penToggle.check();
#endif

#ifdef motorsButton
	motorsToggle.check();
#endif

#ifdef prgButton
	prgButtonToggle.check();
#endif
}
