/*SEGA JVS CABINET TO PC INPUT/OUTPUT REPLACEMENT BOARD
 * 
 * Sketch for two players SEGA JVS cabinet (e.g SEGA Universal). 
 * 
 * Hardware description: 
 * 4 input shift registers (74HC165) monitor cabinet buttons states and send them via SPI. 
 * One output shift register (74HC595) sends inputs to lamp driver (ULN2003).
 * Arduino pro-micro monitors 8 analog axes directly.
 * Arduino pro-micro emulates a multi axis controller.
 * 
 * Outputs array: OUT1_3 | OUT2_3 | OUT1_2 | OUT2_2 | OUT1_1 | OUT2_1 | NC | NC
 * 
 * Inputs array: buttons coordinates (JVSCab2PC specific):
 * (REGISTER 0)
 * P1_RIGHT -> stateByte[0] & 0x01;   
 * P2_RIGHT -> stateByte[0] & 0x02;  
 * P1_START -> stateByte[0] & 0x04;  
 * P2_START -> stateByte[0] & 0x08;  
 * P2_LEFT -> stateByte[0] & 0x10;
 * P1_LEFT -> stateByte[0] & 0x20;
 * P2_UP -> stateByte[0] & 0x40;
 * P1_UP -> stateByte[0] & 0x80;
 * (REGISTER 1)
 * P1_SW1 -> stateByte[1] & 0x01;
 * P2_SW1 -> stateByte[1] & 0x02;
 * P1_DOWN -> stateByte[1] & 0x04;
 * P2_DOWN -> stateByte[1] & 0x08;
 * P2_SW2 -> stateByte[1] & 0x10;
 * P1_SW2 -> stateByte[1] & 0x20;
 * P2_SW3 -> stateByte[1] & 0x40;
 * P1_SW3 -> stateByte[1] & 0x80;
 * (REGISTER 2)
 * P1_SW5 -> stateByte[2] & 0x01;
 * P2_SW5 -> stateByte[2] & 0x02;
 * P1_SW4 -> stateByte[2] & 0x04;
 * P2_SW4 -> stateByte[2] & 0x08;
 * P1_SW6 -> stateByte[2] & 0x10;
 * P2_SW6 -> stateByte[2] & 0x20;
 * P1_SW7 -> stateByte[2] & 0x40;
 * P2_SW7 -> stateByte[2] & 0x80;
 * (REGISTER 3)
 * TILT -> stateByte[3] & 0x01;
 * TEST -> stateByte[3] & 0x02;
 * P2_SERVICE -> stateByte[3] & 0x04;
 * P1_SERVICE -> stateByte[3] & 0x08;
 * P1_COIN -> stateByte[3] & 0x10;
 * P2_COIN -> stateByte[3] & 0x20;
 * UNUSED1 -> stateByte[3] & 0x40;
 * UNUSED2 -> stateByte[3] & 0x80;
 * 
 * More info at the following link (Instructables)
 * https://www.instructables.com/JVSCab2PC-SEGA-JVS-Cabinet-to-PC-IO-Replacement-Bo/
 * 
 * by barito 2022 (last update: march 2023)
 */

#include <SPI.h>
#include <Joystick.h>

#define SW_INPUTS 30 //number of digital inputs monitored by input shift registers
#define IN_REGISTERS 4 //number of input shift registers (for debug)

//PINOUT
const byte inRegPin = 5; //input registers latch pin
const byte outRegPin = 10; //output register latch pin
//const int brakePin = A1;//this is actually hardwired, but unused in this sketch
//const int accelPin = A2;//this is actually hardwired, but unused in this sketch
//const int steerPin = A3;//this is actually hardwired, but unused in this sketch
//const int analog3 = A0;//this is actually hardwired, but unused in this sketch
//const int analog4 = A9;//this is actually hardwired, but unused in this sketch
//const int analog5 = A8;//this is actually hardwired, but unused in this sketch
//const int analog6 = A7;//this is actually hardwired, but unused in this sketch
//const int analog7 = A6;//this is actually hardwired, but unused in this sketch

//debug variables
unsigned long startTime;
unsigned long endTime;

//inputs variables
byte mask;
byte iState;
int debounceTime = 40; //ms
byte stateByte[IN_REGISTERS];
byte prevStateByte[IN_REGISTERS];
unsigned long debounce [SW_INPUTS];

Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID, 
  JOYSTICK_TYPE_GAMEPAD, SW_INPUTS, 0, //joy type, button count, hatswitch count
  false, false, false, // X, Y, Z axis
  false, false, false, // X, Y, Z rotation
  false, false, //rudder, throttle
  false, false, false); //accelerator, brake, steering

void setup (){
  pinMode (inRegPin, OUTPUT);
  pinMode (outRegPin, OUTPUT);
  digitalWrite (inRegPin, HIGH);
  digitalWrite (outRegPin, HIGH);
  //initialize debounce
  for (int a = 0; a < SW_INPUTS; a++){
    debounce[a] = millis();
  }
  //start SPI
  SPI.begin ();
  //start joystick
  Joystick.begin();// Initialize Joystick
}  // end of setup

void loop (){
  InRegPulse();
  InRegRead();
  InRegHandle();
}  // end of loop

void InRegHandle(){ //Handle registry inputs (74HC165)
  for (int a = 0; a < IN_REGISTERS; a++){
    mask = 0b00000001;
    for (int i = 0; i < 8; i++){
      iState = stateByte[a] & mask;
      if (millis()- debounce[i+(a*8)] > debounceTime && iState != (prevStateByte[a] & mask)){
        debounce[i+(a*8)] = millis();
        Joystick.setButton(i+(a*8), !iState);
        delay(8);
      }
      mask <<= 1; //move to the next bit
    }
    prevStateByte[a] = stateByte[a];
  }
}

void InRegPulse(){  // pulse the parallel load latch to send bytes serially
  digitalWrite (inRegPin, LOW);
  //delayMicroseconds(5);  
  digitalWrite (inRegPin, HIGH);
}

void InRegRead(){ //read inputs states from registers
  for (int a = 0; a < IN_REGISTERS; a++){
    stateByte[a] = SPI.transfer (0);
  }
}
