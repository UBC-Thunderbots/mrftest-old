#ifndef ACCELEROMETER_H
#define ACCELEROMETER_H

class Accelerometer {
public:
  Accelerometer(const Filter<1>::A &filterA, const Filter<1>::B &filterB, byte adcPin, double radius) : filter(filterA, filterB), adcPin(adcPin), radius(radius), zero(0.0) {
  }
  
  void init() {
    unsigned int accumulator = 0;
    for (byte i = 0; i < ACCELEROMETER_ZERO_SAMPLES; i++)
      accumulator += analogRead(adcPin);
    zero = static_cast<double>(accumulator) / ACCELEROMETER_ZERO_SAMPLES;
  }
  
  double read(double vtheta) {
    return filter.process(-ACCELEROMETER_TO_CM * (analogRead(adcPin) - zero) + vtheta * vtheta * radius);
  }
  
  void nuke() {
    filter.nuke();
  }
  
private:
  Filter<1> filter;
  const byte adcPin;
  const double radius;
  double zero;
};

#endif
