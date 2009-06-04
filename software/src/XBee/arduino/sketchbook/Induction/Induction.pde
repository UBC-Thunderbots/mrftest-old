#include <FieldOrientedControl.h>

FieldOrientedControl foc;

void setup(){
  Serial.begin(9600);
  if(!foc.initializeMotor()){
    Serial.println("Motor initialization failed");
    delay(3000);
    setup();
  }
}

void loop(){
  int iter = 1000;
  int startTime = millis();
  foc.runMotor(iter);
  int calcTime = millis()-startTime;
  Serial.print("Iterations per second: ");
  Serial.println(iter/(float)(calcTime/(float)1000), DEC);
}
