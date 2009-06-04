//Wheel Acceleration test application
//Written By Jonathan Fraser

#define SELECTED_MOTOR 3

#define SELECT_PIN1 4
#define SELECT_PIN2 5
#define SELECT_PIN3 6
#define SELECT_PIN4 7

#define COUNTER_PORT 1

#define MOTOR1_CTRL_PIN1 16
#define MOTOR1_CTRL_PIN2 17
#define MOTOR1_PWM_PIN 0


#define MOTOR2_CTRL_PIN1 18
#define MOTOR2_CTRL_PIN2 19
#define MOTOR2_PWM_PIN 1


#define MOTOR3_CTRL_PIN1 20
#define MOTOR3_CTRL_PIN2 21
#define MOTOR3_PWM_PIN 2


#define MOTOR4_CTRL_PIN1 22
#define MOTOR4_CTRL_PIN2 23
#define MOTOR4_PWM_PIN 3

#define COUNTER_RESET 24

unsigned char Selects[4] = {
  SELECT_PIN1,SELECT_PIN2,SELECT_PIN3,SELECT_PIN4};

unsigned char Ctrl1Pins[4]={
  MOTOR1_CTRL_PIN1,MOTOR2_CTRL_PIN1,MOTOR3_CTRL_PIN1,MOTOR4_CTRL_PIN1};
unsigned char Ctrl2Pins[4]={
  MOTOR1_CTRL_PIN2,MOTOR2_CTRL_PIN2,MOTOR3_CTRL_PIN2,MOTOR4_CTRL_PIN2};
unsigned char PwmPins[4]={
  MOTOR1_PWM_PIN,MOTOR2_PWM_PIN,MOTOR3_PWM_PIN,MOTOR4_PWM_PIN};

unsigned char data[256];
unsigned char dataIndex=0;
unsigned long time;

inline void timer1SetPrescaler(uint8_t DivValue)
{
  TCCR1B=(TCCR1B & ~TIMER_PRESCALE_MASK)|DivValue;
}

inline void timer3SetPrescaler(uint8_t DivValue)
{
  TCCR3B=(TCCR3B & ~TIMER_PRESCALE_MASK)|DivValue;
}


void setup()
{
  Serial.begin(9600);
  for(int i=0;i<4;i++){
    pinMode(Ctrl1Pins[i],OUTPUT);
    pinMode(Ctrl2Pins[i],OUTPUT);
    pinMode(Selects[i],OUTPUT);
    digitalWrite(Selects[i],HIGH);
    digitalWrite(Ctrl1Pins[i],LOW);
    digitalWrite(Ctrl2Pins[i],LOW);
  }
  digitalWrite(Selects[SELECTED_MOTOR],LOW);
  digitalWrite(Ctrl1Pins[SELECTED_MOTOR],HIGH);
  digitalWrite(Ctrl2Pins[SELECTED_MOTOR],LOW);
  portMode(COUNTER_PORT,INPUT);

  portMode(COUNTER_RESET,OUTPUT);
  digitalWrite(COUNTER_RESET,LOW);
  digitalWrite(COUNTER_RESET,HIGH);

  time=millis();
  while(millis()<=time);
  time++;
  analogWrite(PwmPins[SELECTED_MOTOR],0);
  timer1SetPrescaler(TIMER_CLK_DIV8);
  timer3SetPrescaler(TIMER_CLK_DIV8);
  analogWrite(PwmPins[SELECTED_MOTOR],1023);
}

void loop()
{
  dataIndex=0;
  do{
    data[dataIndex++]=portRead(COUNTER_PORT);
    while(millis()<=time);
    time++;
  }
  while(dataIndex);
  analogWrite(PwmPins[SELECTED_MOTOR],0);


  //while(!Serial.available());

  dataIndex=0;
  do{
    Serial.println((int)data[dataIndex++],DEC);  
    delay(1);
  }
  while(dataIndex);

  while(1);  
}

