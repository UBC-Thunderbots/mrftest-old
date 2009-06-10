#ifndef WHEEL_H
#define WHEEL_H

class Wheel {
public:
  Wheel(const Filter<1>::A &rpmFilterA, const Filter<1>::B &rpmFilterB,
    const Filter<2>::A &wheelControllerA, const Filter<2>::B &wheelControllerB,
    byte counterSelectPin, byte motorDirectionPin1, byte motorDirectionPin2, byte motorPWMPin,
    const double (&scaleFactors)[3])
    : rpmFilter(rpmFilterA, rpmFilterB), wheelController(wheelControllerA, wheelControllerB), lastCounterValue(0),
    counterSelectPin(counterSelectPin), motorDirectionPin1(motorDirectionPin1), motorDirectionPin2(motorDirectionPin2),
    motorPWMPin(motorPWMPin), scaleFactors(scaleFactors) {
  }
  
  void init() {
    digitalWrite(counterSelectPin, HIGH);
    digitalWrite(motorDirectionPin1, HIGH);
    digitalWrite(motorDirectionPin2, LOW);
    analogWrite(motorPWMPin, 0);
    pinMode(counterSelectPin, OUTPUT);
    pinMode(motorDirectionPin1, OUTPUT);
    pinMode(motorDirectionPin2, OUTPUT);
  }
  
  void update(double axActuator, double ayActuator, double vthetaActuator) {
    // Compute motor percentage setpoint.
    double setpoint = axActuator * scaleFactors[0] + ayActuator * scaleFactors[1] + vthetaActuator * scaleFactors[2];
    
    // Read counter and determine current RPM.
    digitalWrite(counterSelectPin, LOW);
    byte curCounterValue = portRead(IOPORT_COUNTER_DATA);
    digitalWrite(counterSelectPin, HIGH);
    char diff = curCounterValue - lastCounterValue;
    lastCounterValue = curCounterValue;
    double curRPM = rpmFilter.process(ENCODER_COUNTS_TO_RPM / LOOP_TIME * diff);
    
    // Pass the error in motor percentage through the controller.
    #if W_CONTROLLER_ENABLED
    double power = wheelController.process(setpoint - curRPM / MOTOR_MAX_RPM);
    #else
    double power = setpoint;
    #endif
    
    // Drive the motor.
    if (power > 0.0) {
      digitalWrite(motorDirectionPin1, HIGH);
      digitalWrite(motorDirectionPin2, LOW);
    } else {
      digitalWrite(motorDirectionPin1, LOW);
      digitalWrite(motorDirectionPin2, HIGH);
    }
    analogWrite(motorPWMPin, min(static_cast<unsigned int>(abs(power) * 1023 + 0.5), MOTOR_CAP));
  }
  
  void nuke() {
    rpmFilter.nuke();
    wheelController.nuke();
    analogWrite(motorPWMPin, 0);
  }
  
private:
  Filter<1> rpmFilter;
  Filter<2> wheelController;
  byte lastCounterValue;
  const byte counterSelectPin, motorDirectionPin1, motorDirectionPin2, motorPWMPin;
  const double (&scaleFactors)[3];
};

#endif
