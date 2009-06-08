#ifndef ACCELEROMETER_H
#define ACCELEROMETER_H


class Accelerometer {
public:
  Accelerometer(const Filter<2>::A &filterA, const Filter<2>::B &filterB, byte adcPin, double radius,bool invertCompensation) : filter(filterA, filterB), adcPin(adcPin), radius(radius), velocity(0.0), zero(0.0),invertCompensation(invertCompensation) {
  }
  
  void init() {
    unsigned int accumulator = 0;
    for (byte i = 0; i < ACCELEROMETER_ZERO_SAMPLES; i++)
      accumulator += analogRead(adcPin);
    zero = static_cast<double>(accumulator) / ACCELEROMETER_ZERO_SAMPLES;
  }
  
  double read(double vtheta) {
    velocity += (-ACCELEROMETER_TO_CM * (analogRead(adcPin) - zero) + (invertCompensation?-1:1)*vtheta * vtheta * radius)*LOOP_TIME/1000;
    return filter.process(velocity);
  }
  
  void nuke() {
    filter.nuke();
    velocity = 0;
  }
  
  void velSet(double velocity) {
    this->velocity = velocity;
  }
  
private:
  Filter<2> filter;
  const byte adcPin;
  const double radius;
  double velocity;
  double zero;
  bool invertCompensation;
};

#endif
