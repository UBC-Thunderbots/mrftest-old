// UBC Thunderbots
// DATE: Feb 20 2009
// Last Modified: May 20, 2009
// For more information about the code, please contact bahador.mousavi@gmail.com
// Main authors: Bahador Moosavi zadeh, George Stelle, Jon Fraser

#include "HardwareSerial.h"
#include "./Constants.h"
#include "./XBee.h"
#include "./Filter.h"
#include "./Wheel.h"
#include "./Accelerometer.h"

// The setpoint filters for the linear velocity setpoints.
static const double setpointFilterA[] = {0.0, 0.0};
static const double setpointFilterB[] = {1.0, 0.0, 0.0};
static Filter<2> setpointXFilter(setpointFilterA, setpointFilterB);
static Filter<2> setpointYFilter(setpointFilterA, setpointFilterB);

// The linear acceleration controllers.
static const double vxControllerA[] = {0.0};
static const double vxControllerB[] = {0.000, 0.000};
static Filter<1> vxController(vxControllerA, vxControllerB);

static const double vyControllerA[] = {0.0};
static const double vyControllerB[] = {0.000, 0.000};
static Filter<1> vyController(vyControllerA, vyControllerB);

// The angular velocity controller.
static const double vtControllerA[] = {-1.9724, 0.9724};
static const double vtControllerB[] = {0.0406, -0.0806, 0.0400};
static Filter<2> vtController(vtControllerA, vtControllerB);

// The feedforward controller.
static const double feedforwardControllerA[] = {0.0};
static const double feedforwardControllerB[] = {3.0/16.0, 0.0};
static Filter<1> feedforwardController(feedforwardControllerA, feedforwardControllerB);

// The wheels.
static const double rpmFilterA[] = {-0.777778};
static const double rpmFilterB[] = {0.111111, 0.111111};
static const double wheelControllerA[] = {-1.7967, 0.7967};
static const double wheelControllerB[] = {1.2369, -2.0521, 0.8397};
static const double m[][3] = {
  {-0.2588,  0.9659, 1.0},
  {-0.2588, -0.9659, 1.0},
  { 0.7071, -0.7071, 1.0},
  { 0.7071,  0.7071, 1.0}
};
static Wheel wheels[] = {
  Wheel(rpmFilterA, rpmFilterB, wheelControllerA, wheelControllerB,
    IOPIN_COUNTER0_OE, IOPIN_MOTOR0A, IOPIN_MOTOR0B, PWMPIN_MOTOR0, m[0]),
  Wheel(rpmFilterA, rpmFilterB, wheelControllerA, wheelControllerB,
    IOPIN_COUNTER1_OE, IOPIN_MOTOR1A, IOPIN_MOTOR1B, PWMPIN_MOTOR1, m[1]),
  Wheel(rpmFilterA, rpmFilterB, wheelControllerA, wheelControllerB,
    IOPIN_COUNTER2_OE, IOPIN_MOTOR2A, IOPIN_MOTOR2B, PWMPIN_MOTOR2, m[2]),
  Wheel(rpmFilterA, rpmFilterB, wheelControllerA, wheelControllerB,
    IOPIN_COUNTER3_OE, IOPIN_MOTOR3A, IOPIN_MOTOR3B, PWMPIN_MOTOR3, m[3]),
};

// The accelerometers.
static const double accelerometerFilterA[] = {-1.8229, 0.8374};
static const double accelerometerFilterB[] = {0.0036, 0.0072, 0.0036};
static Accelerometer accelerometerX(accelerometerFilterA, accelerometerFilterB, ADCPIN_ACCEL1Y,  6.5);
static Accelerometer accelerometerY(accelerometerFilterA, accelerometerFilterB, ADCPIN_ACCEL2Y, -4.5);

// The gyro's zero point.
static double gyroZero;

// Record the time of the last received message.
static long lastReceivedMessageTime;

// Record the time the last kick was started.
static long kickStartTime;

// Record the time the loop last ran.
static long lastLoopTime;

// Record the time we last transmitted battery data.
static long lastBatteryTime;



// Prints a floating-point number with a fix for the negative-number bug.
static void print(double num) {
  if (num < 0) {
    Serial.print('-');
    Serial.print(static_cast<float>(-num));
  } else {
    Serial.print(static_cast<float>(num));
  }
}



// Prints a floating-point number with a fix for the negative-number bug, followed by a newline.
static void println(double num) {
  print(num);
  Serial.println();
}



// Zeroes the gyro.
static void initGyro() {
  int accumulator = 0;
  for (byte i = 0; i < GYRO_ZERO_SAMPLES; i++)
    accumulator += analogRead(ADCPIN_GYRO_DATA) - analogRead(ADCPIN_GYRO_VREF);
  gyroZero = static_cast<double>(accumulator) / GYRO_ZERO_SAMPLES;
}



// Reads from the gyro.
static double readGyro() {
  return GYRO_TO_RADS * (analogRead(ADCPIN_GYRO_DATA) - analogRead(ADCPIN_GYRO_VREF) - gyroZero);
}



// Configures the microcontroller.
void setup() {
  // Initialize the serial ports.
  Serial.begin(BAUD_RATE_USB);
  Serial1.begin(BAUD_RATE_XBEE);

  // Initialize the per-wheel ports.
  for (byte i = 0; i < sizeof(wheels) / sizeof(*wheels); i++)
    wheels[i].init();

  // Initialize the kicker and dribbler.
  analogWrite(PWMPIN_DRIBBLER, 0);
  analogWrite(PWMPIN_KICKER, 0);

  // Configure the IO port where counter values are read from as an input.
  portMode(IOPORT_COUNTER_DATA, INPUT);
  
  // Configure the CPU-busy pin as an output.
  pinMode(IOPIN_CPU_BUSY, OUTPUT);

  // Reset the counters.
  pinMode(IOPIN_COUNTER_RESET, OUTPUT);
  digitalWrite(IOPIN_COUNTER_RESET, LOW);
  digitalWrite(IOPIN_COUNTER_RESET, HIGH);

  // 100Hz PWM is too slow. Increase frequency to 1kHz by tweaking timer prescalers.
  // DIV64 = 120Hz
  // DIV8  = 1kHz
  // DIV1  = 8kHz
  TCCR1B = (TCCR1B & ~TIMER_PRESCALE_MASK) | TIMER_CLK_DIV8;
  TCCR3B = (TCCR3B & ~TIMER_PRESCALE_MASK) | TIMER_CLK_DIV8;
  
  // Zero the accelerometers.
  accelerometerX.init();
  accelerometerY.init();
    
  // Zero the gyro.
  initGyro();

  // Configure the XBee module.
  XBee::init();
  
  // Set all timestamps such that the first loop iteration will start everything up.
  lastReceivedMessageTime = millis();
  kickStartTime           = millis();
  lastLoopTime            = millis() - LOOP_TIME - 1;
  lastBatteryTime         = millis() - TIMEOUT_BATTERY - 1;

  // Tell the user we're good.
  Serial.println("Bot: initialized.");
}



// Zeroes everything.
static void nuke() {
  analogWrite(PWMPIN_KICKER, 0);
  analogWrite(PWMPIN_DRIBBLER, 0);
  for (byte i = 0; i < sizeof(wheels) / sizeof(*wheels); i++)
    wheels[i].nuke();
  setpointXFilter.nuke();
  setpointYFilter.nuke();
  vxController.nuke();
  vyController.nuke();
  vtController.nuke();
  feedforwardController.nuke();
  accelerometerX.nuke();
  accelerometerY.nuke();
  digitalWrite(IOPIN_COUNTER_RESET, LOW);
  digitalWrite(IOPIN_COUNTER_RESET, HIGH);
}



// Updates the drive train appropriately to the compiled operating mode.
#if MANUAL_ACTUATOR
void updateDriveTrain() {
  // Compute actuator levels directly from received data.
  double actx = XBee::rxdata.vx / 127.0 * MANUAL_ACTUATOR_MOTOR_MAX;
  double acty = XBee::rxdata.vy / 127.0 * MANUAL_ACTUATOR_MOTOR_MAX;
  double actt = XBee::rxdata.vt / 127.0 * MANUAL_ACTUATOR_MOTOR_MAX;
  
  // Drive wheels directly.
  for (byte i = 0; i < sizeof(wheels) / sizeof(*wheels); i++)
    wheels[i].update(actx, acty, actt);
}
#else
void updateDriveTrain() {
  // Extract setpoints from most-recently-received packet.
  double vxSetpoint = XBee::rxdata.vx / 127.0 * MAX_SP_VX;
  double vySetpoint = XBee::rxdata.vy / 127.0 * MAX_SP_VY;
  double vtSetpoint = XBee::rxdata.vt / 127.0 * MAX_SP_VT;
  
  if(XBee::rxdata.vxMeasured != -128 && XBee::rxdata.vyMeasured != -128){
    accelerometerX.velSet(XBee::rxdata.vxMeasured /127.0 * MAX_SP_VX);
    accelerometerY.velSet(XBee::rxdata.vyMeasured /127.0 * MAX_SP_VY);
  }
  
  // Differentiate the linear setpoints.
  double vxFilteredSetpoint = setpointXFilter.process(vxSetpoint);
  double vyFilteredSetpoint = setpointYFilter.process(vySetpoint);
  
  // Read the current angular velocity.
  double vt = readGyro();
  
  // Read the current linear accelerations.
  double vx = accelerometerX.read(vt);
  double vy = accelerometerY.read(vt);
  
  // Process errors through controllers to generate actuator levels.
  double actx = vxController.process(vxFilteredSetpoint - vx);
  double acty = vyController.process(vyFilteredSetpoint - vy);
  double actt = vtController.process(vtSetpoint - vt) + feedforwardController.process(actx);

  // Drive wheels.
  for (byte i = 0; i < sizeof(wheels) / sizeof(*wheels); i++)
    wheels[i].update(actx, acty, actt);
}
#endif



// Runs repeatedly to perform control.
void loop() {
  // Send battery voltage every 2000 milliseconds.
  if (millis() - lastBatteryTime > TIMEOUT_BATTERY) {
    unsigned int val = analogRead(ADCPIN_GREEN_BATTERY);
    XBee::txdata.vGreen[0] = val / 256;
    XBee::txdata.vGreen[1] = val % 256;
    val = analogRead(ADCPIN_MOTOR_BATTERY);
    XBee::txdata.vMotor[0] = val / 256;
    XBee::txdata.vMotor[1] = val % 256;
    XBee::send();
    lastBatteryTime = millis();
  }
  
  // See if there's data to receive on the XBee.
  if (XBee::receive()) {
    // Check if we're supposed to be rebooting.
    if (XBee::rxdata.reboot) {
      nuke();
      WDTCR |= 1 << WDE;
      for (;;);
    }
    
    // Record the timestamp.
    lastReceivedMessageTime = millis();
    
    // Start a kick, if this packet requested it.
    if (XBee::rxdata.kick && !kickStartTime) {
      analogWrite(PWMPIN_KICKER, XBee::rxdata.kick * 1023UL / 255UL);
      kickStartTime = millis();
      if (!kickStartTime)
        kickStartTime = 1;
    }
    
    // Set the speed of the dribbler.
    analogWrite(PWMPIN_DRIBBLER, XBee::rxdata.dribble * 1023UL / 255UL);
  }
  
  // Check if we're in emergency stop mode.
  if (XBee::rxdata.emergency || millis() - lastReceivedMessageTime > TIMEOUT_RECEIVE) {
    // Carefully set lastReceivedMessageTime such that even if millis() wraps we won't have a small time region where we're alive.
    lastReceivedMessageTime = millis() - TIMEOUT_RECEIVE - 1;
    nuke();
    return;
  }
  
  // Check whether to shut off kicker.
  if (kickStartTime && millis() - kickStartTime > KICK_TIME) {
    analogWrite(PWMPIN_KICKER, 0);
    kickStartTime = 0;
  }
    
  // Spin until we reach the next loop time.
  digitalWrite(IOPIN_CPU_BUSY, LOW);
  while (millis() - lastLoopTime < LOOP_TIME);
  lastLoopTime += LOOP_TIME;
  digitalWrite(IOPIN_CPU_BUSY, HIGH);

  // Update drive train.
  updateDriveTrain();
}
