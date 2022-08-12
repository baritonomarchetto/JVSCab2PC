/* JVSCab2PC
 * Demo sketch to read from 4 74HC165 input shift register. Based on 
 * Nick Gammon's article at https://www.gammon.com.au/forum/?id=11518
 * 
 * by barito 2022
 */

#include <SPI.h>

const byte LATCH = 5;
byte mask;

void setup ()
{
  SPI.begin ();
  Serial.begin (115200);
  Serial.println ("Begin switch test.");
  pinMode (LATCH, OUTPUT);
  digitalWrite (LATCH, HIGH);
}  // end of setup

byte optionSwitch1;
byte oldOptionSwitch1; // previous state
byte optionSwitch2;
byte oldOptionSwitch2; // previous state
byte optionSwitch3;
byte oldOptionSwitch3; // previous state
byte optionSwitch4;
byte oldOptionSwitch4; // previous state

void loop ()
{
  digitalWrite (LATCH, LOW);    // pulse the parallel load latch
  digitalWrite (LATCH, HIGH);
  optionSwitch1 = SPI.transfer (0);
  optionSwitch2 = SPI.transfer (0);
  optionSwitch3 = SPI.transfer (0);
  optionSwitch4 = SPI.transfer (0);
  
  mask = 1;
  for (int i = 1; i <= 8; i++)
    {
    if ((optionSwitch1 & mask) != (oldOptionSwitch1 & mask))
      {
      Serial.print ("Switch ");
      Serial.print (i);
      Serial.print (" now ");
      Serial.println ((optionSwitch1 & mask) ? "closed" : "open");
      Serial.println ((optionSwitch1 & 0x04) ? "closed" : "open");
      }  // end of bit has changed
    mask <<= 1;  
    }  // end of for each bit
  
  oldOptionSwitch1 = optionSwitch1;

  mask = 1;
  for (int i = 1; i <= 8; i++)
    {
    if ((optionSwitch2 & mask) != (oldOptionSwitch2 & mask))
      {
      Serial.print ("Switch ");
      Serial.print (i+8);
      Serial.print (" now ");
      Serial.println ((optionSwitch2 & mask) ? "closed" : "open");
      Serial.println ((optionSwitch1 & 0x04) ? "closed" : "open");
      }  // end of bit has changed
    mask <<= 1;  
    }  // end of for each bit
  
  oldOptionSwitch2 = optionSwitch2;

  mask = 1;
  for (int i = 1; i <= 8; i++)
    {
    if ((optionSwitch3 & mask) != (oldOptionSwitch3 & mask))
      {
      Serial.print ("Switch ");
      Serial.print (i+16);
      Serial.print (" now ");
      Serial.println ((optionSwitch3 & mask) ? "closed" : "open");
      Serial.println ((optionSwitch1 & 0x04) ? "closed" : "open");
      }  // end of bit has changed
    mask <<= 1;  
    }  // end of for each bit
  
  oldOptionSwitch3 = optionSwitch3;

  mask = 1;
  for (int i = 1; i <= 8; i++)
    {
    if ((optionSwitch4 & mask) != (oldOptionSwitch4 & mask))
      {
      Serial.print ("Switch ");
      Serial.print (i+24);
      Serial.print (" now ");
      Serial.println ((optionSwitch4 & mask) ? "closed" : "open");
      Serial.println ((optionSwitch1 & 0x04) ? "closed" : "open");
      }  // end of bit has changed
    mask <<= 1;  
    }  // end of for each bit
  
  oldOptionSwitch4 = optionSwitch4;
  
  delay (10);   // cheapdebounce
}  // end of loop
