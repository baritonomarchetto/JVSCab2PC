/* JVSCab2PC
 * Analog axis calibration sketch
 * 
 * by barito 2022
 */

//pins to be monitored
const int brakePin = A1; 
const int accelPin = A2; 
const int steerPin = A3;

void setup() {
  Serial.begin(9600);
}

void loop() {
  Serial.print ("Steer");
  Serial.print (" value: ");
  Serial.println (analogRead(steerPin));
  Serial.print ("Accelerator");
  Serial.print (" value: ");
  Serial.println (analogRead(accelPin));
  Serial.print ("Brake");
  Serial.print (" value: ");
  Serial.println (analogRead(brakePin));
  delay(10);
}
