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

// The setpoint filters for the linear velocity setpoints.
static const double setpointFilterA[] = {-1.9440, 0.9455};
static const double setpointFilterB[] = {0.1945, 0.0, -0.1945};
static Filter<2> setpointXFilter(setpointFilterA, setpointFilterB);
static Filter<2> setpointYFilter(setpointFilterA, setpointFilterB);

// The linear acceleration controllers.
static const double accelerationControllerA[] = {0.0};
static const double accelerationControllerB[] = {0.000, 0.000};
static Filter<1> accelerationXController(accelerationControllerA, accelerationControllerB);
static Filter<1> accelerationYController(accelerationControllerA, accelerationControllerB);

// The angular velocity controller.
static const double vthetaControllerA[] = {-1.9724, 0.9724};
static const double vthetaControllerB[] = {0.0406, -0.0806, 0.0400};
static Filter<2> vthetaController(vthetaControllerA, vthetaControllerB);

// The feedforward controller.
static const double feedforwardControllerA[] = {0.0};
static const double feedforwardControllerB[] = {3.0/16.0, 0.0};
static Filter<1> feedforwardController(feedforwardControllerA, feedforwardControllerB);

// The acceleration filters for the accelerometers.
static const double accelFilterA[] = {-0.9608};
static const double accelFilterB[] = {0.0196, 0.0196};
static Filter<1> accelXFilter(accelFilterA, accelFilterB);
static Filter<1> accelYFilter(accelFilterA, accelFilterB);

// The wheels.
static const double rpmFilterA[] = {-0.777778};
static const double rpmFilterB[] = {0.111111, 0.111111};
static const double wheelControllerA[] = {-1.7967, 0.7967};
static const double wheelControllerB[] = {1.2369, -2.0521, 0.8397};
static const double m[4][3] = {
  {-0.2588,  0.9659, 1.0},
  {-0.2588, -0.9659, 1.0},
  { 0.7071, -0.7071, 1.0},
  { 0.7071,  0.7071, 1.0}
};
static Wheel wheels[4] = {
  Wheel(rpmFilterA, rpmFilterB, wheelControllerA, wheelControllerB,
    IOPIN_COUNTER0_OE, IOPIN_MOTOR0A, IOPIN_MOTOR0B, PWMPIN_MOTOR0, m[0]),
  Wheel(rpmFilterA, rpmFilterB, wheelControllerA, wheelControllerB,
    IOPIN_COUNTER1_OE, IOPIN_MOTOR1A, IOPIN_MOTOR1B, PWMPIN_MOTOR1, m[1]),
  Wheel(rpmFilterA, rpmFilterB, wheelControllerA, wheelControllerB,
    IOPIN_COUNTER2_OE, IOPIN_MOTOR2A, IOPIN_MOTOR2B, PWMPIN_MOTOR2, m[2]),
  Wheel(rpmFilterA, rpmFilterB, wheelControllerA, wheelControllerB,
    IOPIN_COUNTER3_OE, IOPIN_MOTOR3A, IOPIN_MOTOR3B, PWMPIN_MOTOR3, m[3]),
};

// Print a floating-point number with a fix for the negative-number bug.
static void print(double num) {
  if(num<0) {
    Serial.print('-');
    Serial.print(static_cast<float>(-num));
  } else {
    Serial.print(static_cast<float>(num));
  }
}
static void println(double num) {
  print(num);
  Serial.println();
}







// Desired Velocity points from the Bot from XBee or Other
double VelocityXSetPoint=0.0;
double VelocityYSetPoint=0.0;
double VelocityThetaSetPoint=0.0;
double AccelerationXSetPoint=0.0;
double AccelerationYSetPoint=0.0;

//These are the actual velocity values that the robot thinks its
//travelling
double VelocityTheta;
double AccelerationX;
double AccelerationY;

//This is to keep track of the loop timing
unsigned long PrevTime=0;

//This holds the difference of the setPoints from measured
double AxError;
double AyError;
double VThetaError;

//These are the values produced by the controller and feed into 
//the prescalling matrix
double AxActuator=0;
double AyActuator=0;
double VThetaActuator=0;


//This is the time of the last XBee message for heart beat
unsigned long LastMessage = 0;

//The last time we sent a battery voltage.
unsigned long LastBatteryTime = 0;

double xAccelZeroPoint;
double yAccelZeroPoint;
double gyroZeroPoint;

unsigned long KickTime=0;

// -------------------------------------------------------------------------------------------- //
// ###################################### INITIAL SETUP####################################### //

//Converts RPMs to Velocity Approximations
inline void GetCurrentVelocities(void);

//Use the Gyro the get the angular Velocity
inline void GetGyro();                         //Gyro and Accelerometer integration

//Use the Accelerometers and the angular_velocity
//to integrate the linear velocity
inline void GetAccel();

//This is the function to trigger the kicker (its busted)
inline void kick(int kickSpeed);

//Control the dribbler speed
inline void dribbler(int dribble);

//Subtracts Velocitys from setpoints
inline void ComputeError(void);

//tests to see how long a kick has been on for and shuts if off if too long
inline void clearKick();

// Configures the microcontroller.
void setup() {
  // Initialize the per-wheel ports.
  for (byte i = 0; i < 4; i++)
    wheels[i].init();

  // Initialize the kicker and dribbler.
  analogWrite(PWMPIN_DRIBBLER, 0);
  analogWrite(PWMPIN_KICKER, 0);

  // Configure the IO port where counter values are read from as an input.
  portMode(IOPORT_COUNTER_DATA, INPUT); 

  // Reset the counters.
  pinMode(IOPIN_COUNTER_RESET, OUTPUT);
  digitalWrite(IOPIN_COUNTER_RESET, LOW);
  digitalWrite(IOPIN_COUNTER_RESET, HIGH);

  // Initialize the serial ports.
  Serial.begin(9600);
  Serial1.begin(9600);

  // 100Hz PWM is too slow. Increase frequency to 1kHz by tweaking timer prescalers.
  // DIV64 = 120Hz
  // DIV8  = 1kHz
  // DIV1  = 8kHz 
  TCCR1B = (TCCR1B & ~TIMER_PRESCALE_MASK) | TIMER_CLK_DIV8;
  TCCR3B = (TCCR3B & ~TIMER_PRESCALE_MASK) | TIMER_CLK_DIV8;

  // Sample the analog Gyro and Accelerometers 50 times.
  // Average the result to get a good zero bias.
  for (byte i = 0; i < 50; i++) {
    xAccelZeroPoint += analogRead(ADCPIN_ACCEL1Y) / 50.0;
    yAccelZeroPoint += analogRead(ADCPIN_ACCEL2Y) / 50.0;
    gyroZeroPoint += (analogRead(ADCPIN_GYRO_DATA) - analogRead(ADCPIN_GYRO_VREF)) / 50.0;
  }

  // Configure the XBee module.
  XBee::init();

  // Tell the user we're good.
  Serial.println("Bot: initialized.");
  
  // Record the current time as the last loop time.
  PrevTime = millis();
}

// Zeroes everything.
static void nuke() {
  VelocityXSetPoint = 0;
  VelocityYSetPoint = 0;
  VelocityThetaSetPoint = 0;
  kick(0);
  dribbler(0);
  for (byte i = 0; i < 4; i++)
    wheels[i].nuke();
  setpointXFilter.nuke();
  setpointYFilter.nuke();
  accelerationXController.nuke();
  accelerationYController.nuke();
  vthetaController.nuke();
  feedforwardController.nuke();
  accelXFilter.nuke();
  accelYFilter.nuke();
}

// Runs repeatedly to perform control.
void loop() {
  // See if there's data to receive on the XBee.
  if (XBee::receive()) {
    VelocityXSetPoint = XBee::rxdata.vx/127.0*10.0; //copy the packet data to the setpoints
    VelocityYSetPoint = XBee::rxdata.vy/127.0*2.0; 
    VelocityThetaSetPoint = XBee::rxdata.vtheta/127.0*4.0;
    kick(XBee::rxdata.kick); //did we want a kick
    dribbler(XBee::rxdata.dribble); //start the dribbler
    LastMessage = millis();
  }
  
  // Check if we're in emergency stop mode.
  if (XBee::rxdata.emergency || millis() - LastMessage > 200) {
    nuke();
    return;
  }

  clearKick(); //Kick timer function to shut the kicker off after KICK_TIME ms
  digitalWrite(25,LOW);
  while(millis()<PrevTime+LOOP_TIME); //This allows us to keep the loop time constant
  PrevTime+=LOOP_TIME;
  digitalWrite(25,HIGH);

  AccelerationXSetPoint = setpointXFilter.process(VelocityXSetPoint);
  AccelerationYSetPoint = setpointYFilter.process(VelocityYSetPoint);

  GetGyro();  //Compute VelocityTheta from Gyro

    GetAccel(); //Compute AcclerationX and Y from acclerometers

  //GetCurrentVelocities();    //Compute X,Y,Theta Velocities from RPM and performs Accel reset etc         

  ComputeError();            //Subtract Actual Velocity from set points

  AxActuator = accelerationXController.process(AxError);
  AyActuator = accelerationYController.process(AyError);
  VThetaActuator = vthetaController.process(VThetaError);  

#if MANUAL_ACTUATOR              //Use this flag if you want to give constant voltages to the motors and bypass the controllers
  AxActuator=VelocityXSetPoint * 0.015;
  AyActuator=VelocityYSetPoint * 0.075;
  VThetaActuator=VelocityThetaSetPoint * 0.05357;
#endif

  VThetaActuator += feedforwardController.process(AxActuator);

  for (byte i = 0; i < 4; i++)
    wheels[i].update(AxActuator, AyActuator, VThetaActuator);

  // Send battery voltage every 2000 milliseconds.
  if (millis() - LastBatteryTime >= 2000) {
    unsigned int val = analogRead(ADCPIN_GREEN_BATTERY);
    XBee::txdata.vGreenHigh = val / 256;
    XBee::txdata.vGreenLow = val % 256;
    val = analogRead(ADCPIN_MOTOR_BATTERY);
    XBee::txdata.vMotorHigh = val / 256;
    XBee::txdata.vMotorLow = val % 256;
    XBee::send();
    LastBatteryTime = millis();
  }
}


inline void GetGyro()
{
  VelocityTheta = GYRO_TO_RADS * (analogRead(ADCPIN_GYRO_DATA) - analogRead(ADCPIN_GYRO_VREF) - gyroZeroPoint);
}

inline void GetAccel()
{
  static double angular_squared; 

  //angular acceleration due to rotation
  angular_squared=VelocityTheta*VelocityTheta;

  //Get Acceleration
  AccelerationX = accelXFilter.process(-1*ACCELEROMETER_TO_CM*(analogRead(ADCPIN_ACCEL1Y)-xAccelZeroPoint)+angular_squared*ACCEL_1_RADIUS);
  AccelerationY = accelYFilter.process(-1*ACCELEROMETER_TO_CM*(analogRead(ADCPIN_ACCEL2Y)-yAccelZeroPoint)-angular_squared*ACCEL_2_RADIUS);
}

inline void ComputeError(void){
  //Compute the Error off the Velocity Setpoints
  AxError=AccelerationXSetPoint-AccelerationX;
  AyError=AccelerationYSetPoint-AccelerationY;
  VThetaError=VelocityThetaSetPoint-VelocityTheta;

}

inline void kick(int kickSpeed){
  if(kickSpeed)
  {
    analogWrite(PWMPIN_KICKER, kickSpeed*1023.0/255.0);
    KickTime=millis();
  }
}

inline void clearKick()
{
  if((millis()-KickTime)>KICK_TIME)
    analogWrite(PWMPIN_KICKER,0);
}

//dribbler function
inline void dribbler(int dribble){
  analogWrite(PWMPIN_DRIBBLER, dribble*1023.0/255.0);
}
