/* SEGA JVS CABINET TO PC INPUT/OUTPUT REPLACEMENT BOARD
 * 
 * OUTPUTS TEST
 * 
 * by barito 2022 (last update: 12 august 2022)
 */

#include <SPI.h>

//PINOUT
const byte outRegPin = 10; //output register latch pin

//outputs variables
unsigned long blinkTime;
bool oflag;
byte obyte;

void setup (){
  pinMode (outRegPin, OUTPUT);
  digitalWrite (outRegPin, HIGH);
  //start SPI
  SPI.begin ();
}  // end of setup

void loop (){
  OutRegHandle();
}  // end of loop

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
