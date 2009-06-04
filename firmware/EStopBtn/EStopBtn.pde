#include "WProgram.h"
#include "HardwareSerial.h"

#define BUTTON_PIN 24
#define POSITIVE_PIN 25
#define NEGATIVE_PIN 26

void setup() {
  pinMode(BUTTON_PIN, INPUT);
  pinMode(POSITIVE_PIN, OUTPUT);
  digitalWrite(POSITIVE_PIN, HIGH);
  pinMode(NEGATIVE_PIN, OUTPUT);
  digitalWrite(NEGATIVE_PIN, LOW);
  Serial.begin(9600);
}

void loop() {
  if (digitalRead(BUTTON_PIN) == HIGH) {
    Serial.print('0');
  } else {
    Serial.print('1');
  }
  delay(100);
}
