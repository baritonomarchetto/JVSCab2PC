/*SEGA JVS CABINET TO PC INPUT/OUTPUT REPLACEMENT BOARD
 * 
 * Sketch to be used in a one player SEGA JVS driving cabinet with sequential shifter and start button.
 * Registers handling is kept verbose to help figure out how it works.
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
#include <Keyboard.h>

#define DIGITALS 6 //number of digital inputs actually wired in the cabinet
#define IN_REGISTERS 4 //number of input shift registers (for debug)
#define Accel_min 300
#define Accel_MAX 600
#define Brake_min 300
#define Brake_MAX 600

//PINOUT
const byte inRegPin = 5; //input registers latch pin
const byte outRegPin = 10; //output register latch pin
const int brakePin = A1; 
const int accelPin = A2; 
const int steerPin = A3;
//const int analog3 = A0;//this is actually hardwired, but unused in this sketch
//const int analog4 = A9;//this is actually hardwired, but unused in this sketch
//const int analog5 = A8;//this is actually hardwired, but unused in this sketch
//const int analog6 = A7;//this is actually hardwired, but unused in this sketch
//const int analog7 = A6;//this is actually hardwired, but unused in this sketch

//analog variables
int steerVal; 
int brakeVal; 
int accelVal;

//outputs variables
unsigned long blinkTime;
bool oflag;
byte obyte;

//inputs variables
byte mask;
byte reg;
byte pad_button;
byte iState;
bool shifted = 0;
int debounceTime = 40; //ms
byte stateByte[IN_REGISTERS];
byte prevStateByte[IN_REGISTERS];
unsigned long debounce [DIGITALS];
bool startBlock; 

//X and Y rotation are L and R triggers (separate axis). 
Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID, 
  JOYSTICK_TYPE_GAMEPAD, (DIGITALS*2), 0, //joy type, button count, hatswitch count
  true, false, false, // X, Y, Z axis
  true, true, false, // X, Y, Z rotation
  false, false, //rudder, throttle
  false, false, false); //accelerator, brake, steering

void setup (){
  pinMode (inRegPin, OUTPUT);
  pinMode (outRegPin, OUTPUT);
  digitalWrite (inRegPin, HIGH);
  digitalWrite (outRegPin, HIGH);
  //initialize debounce
  for (int a = 0; a < DIGITALS; a++){
    debounce[a] = millis();
  }
  //start SPI
  SPI.begin ();
  //start joystick
  Joystick.begin();// Initialize Joystick
  Joystick.setXAxisRange(100, 923);//steer
  //Joystick.setYAxisRange(Brake_min, Accel_MAX);//combi steer and brake
  Joystick.setRyAxisRange(Accel_min, Accel_MAX);//accel
  Joystick.setRxAxisRange(Brake_min, Brake_MAX);//brake
  //Joystick.setYAxis(512);//this is to avoid false Y readings
  //start keyboard
  Keyboard.begin();//Initialize keyboard
}  // end of setup

void loop (){
  AnalogHandle();
  InRegPulse();
  InRegRead();//QUESTA FUNZIONE INIBISCE OutRegHandle()
  InRegHandle();
  OutRegHandle();
}  // end of loop

void InRegHandle(){ //Handle registry inputs (74HC165)
  //----------- P1 START----------- 
  mask = 0x04;
  reg = 0;
  pad_button = 0;
  iState = stateByte[reg] & mask;
  if (millis()- debounce[pad_button] > debounceTime && iState != (prevStateByte[reg] & mask)){
    debounce[pad_button] = millis();
    shifted = !iState;//set the shift flag
    if (shifted == false) {//ON RELEASE
      if(startBlock == false){
        Joystick.setButton(pad_button, 1);
        delay(100);
        Joystick.setButton(pad_button, 0);
        delay(70);
      }
      else{
        startBlock = false;
      }
      _RelAllSft();
    }
  }
  //CONTINUOS CHECK... (not only on button state change)
  if(shifted == true && brakeVal>500){//start pressed and brake pressed
    startBlock = true;
    Keyboard.press(177);//ESC
    delay(100);
    Keyboard.release(177);
    delay(500);//long delay to avoid double presses
  }
  //----------- P1 UP ----------- 
  mask = 0x80;
  reg = 0;
  pad_button = 1;
  iState = stateByte[reg] & mask;
  if (millis()- debounce[pad_button] > debounceTime && iState != (prevStateByte[reg] & mask)){
    debounce[pad_button] = millis();
    if(shifted == 0){//not shifted
      Joystick.setButton(pad_button, !iState);
      delay(8);
    }
    else {
      Joystick.setButton(pad_button + DIGITALS, !iState);
      delay(8);
      startBlock = true;
    }
  }  
  //----------- P1 DOWN ----------- 
  mask = 0x04;
  reg = 1;
  pad_button = 2;
  iState = stateByte[reg] & mask;
  if (millis()- debounce[pad_button] > debounceTime && iState != (prevStateByte[reg] & mask)){
    debounce[pad_button] = millis();
    if(shifted == 0){//not shifted
      Joystick.setButton(pad_button, !iState);
      delay(8);
    }
    else {
      Joystick.setButton(pad_button + DIGITALS, !iState);
      delay(8);
      startBlock = true;
    }
  }  
  //----------- TEST ----------- 
  mask = 0x02;
  reg = 3;
  pad_button = 3;
  iState = stateByte[reg] & mask;
  if (millis()- debounce[pad_button] > debounceTime && iState != (prevStateByte[reg] & mask)){
    debounce[pad_button] = millis();
    if(shifted == 0){//not shifted
      Joystick.setButton(pad_button, !iState);
      delay(8);
    }
    else {
      Joystick.setButton(pad_button + DIGITALS, !iState);
      delay(8);
      startBlock = true;
    }
  }  
  //----------- P1 SERVICE ----------- 
  mask = 0x08;
  reg = 3;
  pad_button = 4;
  iState = stateByte[reg] & mask;
  if (millis()- debounce[pad_button] > debounceTime && iState != (prevStateByte[reg] & mask)){
    debounce[pad_button] = millis();
    if(shifted == 0){//not shifted
      Joystick.setButton(pad_button, !iState);
      delay(8);
    }
    else {
      Joystick.setButton(pad_button + DIGITALS, !iState);
      delay(8);
      startBlock = true;
    }
  }  
  //----------- P1 COIN ----------- 
  mask = 0x10;
  reg = 3;
  pad_button = 5;
  iState = stateByte[reg] & mask;
  if (millis()- debounce[pad_button] > debounceTime && iState != (prevStateByte[reg] & mask)){
    debounce[pad_button] = millis();
    if(shifted == 0){//not shifted
      Joystick.setButton(pad_button, !iState);
      delay(8);
    }
    else {
      Joystick.setButton(pad_button + DIGITALS, !iState);
      delay(8);
      startBlock = true;
    }
  }  
  //update stateBytes
  prevStateByte[0] = stateByte[0];
  prevStateByte[1] = stateByte[1];
  prevStateByte[2] = stateByte[2];
  prevStateByte[3] = stateByte[3];
}

void AnalogHandle(){ //handle wheel, accel and brake
  steerVal = analogRead(steerPin);
  if(steerVal > 514 || steerVal < 510){ //very small deadzone (+/-2)
    Joystick.setXAxis(steerVal);
  }
  brakeVal = analogRead(brakePin);
  Joystick.setRxAxis(brakeVal);//brake, separate axis
  accelVal = analogRead(accelPin);
  Joystick.setRyAxis(accelVal);//accel, separate axis
}

void OutRegHandle(){
//button lamps alternating flashing. Start button lamp mask 0b00001000 (OUT1_1, pin 51 on CN3)
//hardware outputs array: OUT1_3 | OUT2_3 | OUT1_2 | OUT2_2 | OUT1_1 | OUT2_1 | NC | NC
if(millis()-blinkTime > 500) {
  blinkTime = millis();
  oflag = !oflag;
  if(oflag == 1){
    obyte = 0b10101010;
  }
  else{
    obyte = 0b01010101;
  }
  SPI.transfer(obyte/*, sizeof(obyte)*/);
  digitalWrite (outRegPin, LOW);
  digitalWrite (outRegPin, HIGH);
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

void _RelAllSft(){//release all shifted buttons (if you release shift before button, the shift button stay pressed)
  for (int j = 1; j < DIGITALS; j++){
      Joystick.setButton(j + DIGITALS, 0);
      delay(8);
  }
}