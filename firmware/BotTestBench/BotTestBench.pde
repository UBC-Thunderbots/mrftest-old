// UBC Thunderbots
// DATE: Feb 20 2009
// Last Modified: May 20, 2009
// For more information about the code, please contact bahador.mousavi@gmail.com
// Main authors: Bahador Moosavi zadeh, George Stelle, Jon Fraser

#include "./Filter.h"

//By passes the controller and applies the velocity Setpoints to
//the M (Pre-Scaler) matrix allow direct control of motor power.
//#define MANUAL_ACTUATOR


//Time of each interation in milliseconds
#define LoopTime 4

//Maximum motor power 0-1023 cap it for testing
#define MOTOR_CAP 1023

//Scaler Value that converts Motor counts to RPM
#define RPM_SCALER 166.67

#define RADS_PER_BIT 1.0
#define M_PER_BIT 1.0
//Number of ms for the counter to go on bus
#define PIN_WAIT 1

//Attenuation constant for the  motor monitor
#define MOTOR_CONSTANT 5200.0

//To all the pins I've loved before
#define   _pinMotor0Enable 0       // NOTE: PWM
#define   _pinMotor1Enable 1       // NOTE: PWM
#define   _pinMotor2Enable 2       // NOTE: PWM
#define   _pinMotor3Enable 3       // NOTE: PWM
#define   _pinDribblerEnable 4     // NOTE: PWM
#define   _pinKickerEnable 5       // NOTE: PWM

#define   _pinAccel1YInput 0
#define   _pinAccel1XInput 1
#define   _pinAccel2YInput 2
#define   _pinAccel2XInput 3
#define   _pinGyroInput  4
#define   _pinGyroVref  5

#define   _pinMotor0A      16
#define   _pinMotor0B      17
#define   _pinMotor1A      18
#define   _pinMotor1B      19
#define   _pinMotor2A      20
#define   _pinMotor2B      21 
#define   _pinMotor3A      22
#define   _pinMotor3B      23

#define   _greenBatteryMonitor 7
#define   _motorBatteryMonitor 6

//io config macro
#define Select0 4
#define Select1 5
#define Select2 6
#define Select3 7

#define Reset 24

#define    _pinLED  48

#define COUNTER_PORT 1

//80ms for 4ms sample time
#define WheelFilterB0 0.111111
#define WheelFilterB1 0.111111
#define WheelFilterA1 -0.777778

#define WheelControllerB0 1.2369
#define WheelControllerB1 -2.0521
#define WheelControllerB2 0.8397
#define WheelControllerA1 -1.7967
#define WheelControllerA2 0.7967



Filter RPMFilter[4] = {
  Filter(WheelFilterB0,WheelFilterB1,WheelFilterA1),
  Filter(WheelFilterB0,WheelFilterB1,WheelFilterA1),
  Filter(WheelFilterB0,WheelFilterB1,WheelFilterA1),
  Filter(WheelFilterB0,WheelFilterB1,WheelFilterA1)
};

Filter WheelController[4] = {
  Filter(WheelControllerB0,WheelControllerB1,WheelControllerA1,WheelControllerB2,WheelControllerA2),
  Filter(WheelControllerB0,WheelControllerB1,WheelControllerA1,WheelControllerB2,WheelControllerA2),
  Filter(WheelControllerB0,WheelControllerB1,WheelControllerA1,WheelControllerB2,WheelControllerA2),
  Filter(WheelControllerB0,WheelControllerB1,WheelControllerA1,WheelControllerB2,WheelControllerA2)
};


//These are the actual velocity values that the robot thinks its
//travelling
double VelocityTheta;
double AccelerationX;
double AccelerationY;

//Wheel RPMs as reported by the counters
double WheelRPM[4];

//This is to keep track of the loop timing
unsigned long int PrevTime=0;


//This is the time of the last XBee message for heart beat
unsigned long int LastMessage = 0;

//The last time we sent a battery voltage.
unsigned long LastBatteryTime = 0;

unsigned char PrevCount[4] = {
  0,0,0,0};


//Stash the pin constants in arrays for easy processing later
const unsigned char Select[4] = {
  Select0,Select1,Select2,Select3};
  
const unsigned char MotorControl[4][2] = {
  { _pinMotor0A,_pinMotor0B  }
  ,
  { _pinMotor1A,_pinMotor1B  }
  ,
  { _pinMotor2A,_pinMotor2B  }
  ,
  { _pinMotor3A,_pinMotor3B  }
};

const unsigned char MotorEnable[4] = {
  _pinMotor0Enable,_pinMotor1Enable,_pinMotor2Enable,_pinMotor3Enable};


//Emergency stop flag gets set by time out
//can be set and cleared by XBee
int Emergency_Stop=0;

//this is the array of motor percentages from the pre-scaler
//it should have values between -1 and 1
double MotorPercentage[4]={
  0,0,0,0};

double MotorPower[4]={
  0,0,0,0};
  
double XAccelZeroPoint;
double YAccelZeroPoint;
double GyroZeroPoint;

unsigned long int KickTime=0;

int count=0;

double VThetas[256][4];
double AccelXs[256][4];
double AccelYs[256][4];

unsigned char i;
// -------------------------------------------------------------------------------------------- //
// ###################################### INITIAL SETUP####################################### //

//Reads the Counter value, takes in the output enable pin of the counter
inline unsigned char CountPort(int Select);

//succesively reads the counters and converts to RPM Values for each wheel
inline void GetRPM(void);


//Use the Gyro the get the angular Velocity
inline void GetGyro();                         //Gyro and Accelerometer integration

//Use the Accelerometers and the angular_velocity
//to integrate the linear velocity
inline void GetAccel();


//Apply Motor percentages to motors
inline void SetMotors(void);


//reset the timer module for a different PWM frequency
inline void timer1SetPrescaler(uint8_t DivValue)
{
  TCCR1B=(TCCR1B & ~TIMER_PRESCALE_MASK)|DivValue;
}

inline void timer3SetPrescaler(uint8_t DivValue)
{
  TCCR3B=(TCCR3B & ~TIMER_PRESCALE_MASK)|DivValue;
}


//re-write the print function to fix for negative numbers
void print(float num)
{
  if(num<0)
  {
    Serial.print('-');
    Serial.print(-num);
  }
  else
    Serial.print(num);
}

void println(float num)
{
  print(num);
  Serial.println();
}



void setup(){
  //Set all the Motor Control lines to output
  for(int i=0;i<4;i++){
    pinMode(MotorEnable[i],OUTPUT);
    analogWrite(MotorEnable[i],0);
    for(int j=0;j<2;j++)
      pinMode(MotorControl[i][j],OUTPUT);

  }

  //Kicker and dribbler control are outputs
  pinMode(_pinDribblerEnable, OUTPUT);
  analogWrite(_pinDribblerEnable,0);
  pinMode(_pinKickerEnable, OUTPUT);
  analogWrite(_pinKickerEnable,0);

  //this is the port for the counters and must be input
  portMode(COUNTER_PORT, INPUT); 

  //The Output Enable for the counters is an output
  for(int i=0;i<4;i++){
    digitalWrite (Select[i], HIGH); //Give a high output to prevent bus contention
    pinMode(Select[i],OUTPUT);
  }

  //Both serial ports operate at 9600
  Serial.begin(9600);
  Serial1.begin(9600); //we needs a goods bauds rates

  //This resets the counters (maybe make it another function?)
  pinMode(Reset, OUTPUT);
  digitalWrite (Reset, LOW);		//reset all counters
  delay(PIN_WAIT);
  digitalWrite (Reset, HIGH);
  delay(PIN_WAIT);


  Serial.println("Resetting Timer Scalers"); 
  //because a 100Hz PWM isn't fast enough
  //DIV64=120Hz
  //DIV8 = 1kHz
  //DIV1 = 8kHz 
  timer1SetPrescaler(TIMER_CLK_DIV8);
  timer3SetPrescaler(TIMER_CLK_DIV8);


  //Sampled the analog Gyro and Accelerometers 50 times
  //Average the result to get a good zero bias
  for(unsigned char i=0;i<50;i++) {
    XAccelZeroPoint+=analogRead(_pinAccel1YInput)/50.0;
    YAccelZeroPoint+=analogRead(_pinAccel2YInput)/50.0;
    GyroZeroPoint+=(analogRead(_pinGyroInput)-(double)analogRead(_pinGyroVref))/50.0;
  }

  //some feed back
  Serial.println("LOADED...");
  PrevTime=millis();
}

void loop(){
  digitalWrite(25,LOW);
  while(millis()<PrevTime+LoopTime); //This allows us to keep the loop time constant
  PrevTime+=LoopTime;
  digitalWrite(25,HIGH);
  

   GetGyro();  //Compute VelocityTheta from Gyro   
   GetAccel(); //Compute AcclerationX and Y from acclerometers
   
   
   GetRPM();   //This Function gets the wheel rpm and stores it in WheelRPM array
   
   //GetCurrentVelocities();    //Compute X,Y,Theta Velocities from RPM and performs Accel reset etc         

   /*
   Put some code code here for moving the setpoint around
  and recording values   
   
   
   */

  for(i=0;i<4;i++)
        MotorPower[i]=WheelController[i].process(MotorPercentage[i]-WheelRPM[i]/MOTOR_CONSTANT);

  SetMotors();   //Actually sets the motor voltages
}


inline void GetGyro()
{
  VelocityTheta=RADS_PER_BIT*(analogRead(_pinGyroInput)-(double)analogRead(_pinGyroVref)-GyroZeroPoint);
}

inline void GetAccel()
{
  AccelerationX=-1*M_PER_BIT*((signed int)analogRead(_pinAccel1YInput)-XAccelZeroPoint);
  AccelerationY=-1*M_PER_BIT*((signed int)analogRead(_pinAccel2YInput)-YAccelZeroPoint);
}

inline void GetRPM()
{ 

  static unsigned char Cur_Count;
  static  int  motor_count;


  for(int i=0;i<4;i++){  
    Cur_Count = CountPort(Select[i]);		        //Get the current counter state
    WheelRPM[i]=RPMFilter[i].process(RPM_SCALER/LoopTime*(double)(char)(Cur_Count - PrevCount[i]));  //this is a 8 previous state filter
    PrevCount[i] = Cur_Count;                             //store previous count
  }

}


//Takes motor Percentages (-1,1) and applies to motors
inline void SetMotors(void)
{
  static int motor[4]; 

    
  for(i=0;i<4;i++)
  {
    if(Emergency_Stop)
      MotorPower[i]=0;
      
    motor[i] = min(1023*abs(MotorPower[i]), MOTOR_CAP);

    if(MotorPower[i] > 0){
      digitalWrite(MotorControl[i][0],HIGH);
      digitalWrite(MotorControl[i][1],LOW);
    }
    else{
      digitalWrite(MotorControl[i][1],HIGH);
      digitalWrite(MotorControl[i][0],LOW);
    }  
    analogWrite(MotorEnable[i], motor[i]);
  }

}


//simple function to load counter port
inline unsigned char CountPort(int EnablePin){

  static unsigned char retval=0;
  digitalWrite (EnablePin, LOW);		//enable chip 1 outputs (traditionally faster than CE)
  retval = portRead(COUNTER_PORT);  
  digitalWrite (EnablePin, HIGH);
  return retval;
}
