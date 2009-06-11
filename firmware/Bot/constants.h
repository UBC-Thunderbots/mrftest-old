#ifndef CONSTANTS_H
#define CONSTANTS_H

// Indicate CPU speed.
#define F_CPU 16000000UL

// Allows bypassing various controllers and running in open loop.
#define T_CONTROLLER_ENABLED 1
#define X_CONTROLLER_ENABLED 0
#define Y_CONTROLLER_ENABLED 0
#define F_CONTROLLER_ENABLED 0
#define W_CONTROLLER_ENABLED 1

// Parameters for the debug port.
#define DEBUG_BAUD 115200UL
#define DEBUG_BUFSIZE 128

// The maximum motor percentage that can be applied in manual actuator mode.
#define MANUAL_ACTUATOR_MOTOR_MAX 4.0

// Maximum representable values for the setpoints.
#define MAX_SP_VX 500.0
#define MAX_SP_VY  200.0
#define MAX_SP_VT  4.0

// Maximum motor power (0-1023)
#define MOTOR_CAP 1023

// This RPM is considered 100%
#define MOTOR_MAX_RPM 5200.0

// Time of each interation in milliseconds
#define LOOP_TIME 4

// Scaler that converts Motor counts to RPM
#define ENCODER_COUNTS_TO_RPM 166.67

#define GREEN_BATTERY_CONVERSION 0.015577712
#define GREEN_BATTERY_LOW 9.0

#define MOTOR_BATTERY_CONVERSION 0.020932551
#define MOTOR_BATTERY_LOW 14.4

// Scaler radians/second per ADC unit
#define GYRO_TO_RADS 0.0127768

// Number of samples to take while zeroing the gyro.
#define GYRO_ZERO_SAMPLES 50

// Scaler cm/second^2 per ADC unit
#define ACCELEROMETER_TO_CM 4.785156

// Number of samples to take while zeroing the accelerometers.
#define ACCELEROMETER_ZERO_SAMPLES 50

// How long to fire the kicker for (ms)
#define KICK_TIME 200

// XBee configuration.
#define XBEE_BAUD 9600UL
#define XBEE_POWER_LEVEL "0"
#define XBEE_CHANNEL "E"
#define XBEE_PAN "6666"

// Timeout for receiving a data packet before nuking (ms).
#define TIMEOUT_RECEIVE 200

// Interval between sending battery voltage updates.
#define TIMEOUT_BATTERY 2000

// PWM pin numbers
#define PWMPIN_MOTOR0   0
#define PWMPIN_MOTOR1   1
#define PWMPIN_MOTOR2   2
#define PWMPIN_MOTOR3   3
#define PWMPIN_DRIBBLER 4
#define PWMPIN_KICKER   5

// ADC pin numbers
#define ADCPIN_ACCEL1Y       0
#define ADCPIN_ACCEL1X       1
#define ADCPIN_ACCEL2Y       2
#define ADCPIN_ACCEL2X       3
#define ADCPIN_GYRO_DATA     4
#define ADCPIN_GYRO_VREF     5
#define ADCPIN_MOTOR_BATTERY 6
#define ADCPIN_GREEN_BATTERY 7

// IO pin numbers (check for conflicts with IO ports below if changing!)
#define IOPIN_XBEE_RXD      2
#define IOPIN_XBEE_TXD      3
#define IOPIN_COUNTER0_OE   4
#define IOPIN_COUNTER1_OE   5
#define IOPIN_COUNTER2_OE   6
#define IOPIN_COUNTER3_OE   7
#define IOPIN_COUNTER_DATA0 8
#define IOPIN_COUNTER_DATA1 9
#define IOPIN_COUNTER_DATA2 10
#define IOPIN_COUNTER_DATA3 11
#define IOPIN_COUNTER_DATA4 12
#define IOPIN_COUNTER_DATA5 13
#define IOPIN_COUNTER_DATA6 14
#define IOPIN_COUNTER_DATA7 15
#define IOPIN_MOTOR0A       16
#define IOPIN_MOTOR0B       17
#define IOPIN_MOTOR1A       18
#define IOPIN_MOTOR1B       19
#define IOPIN_MOTOR2A       20
#define IOPIN_MOTOR2B       21
#define IOPIN_MOTOR3A       22
#define IOPIN_MOTOR3B       23
#define IOPIN_COUNTER_RESET 24
#define IOPIN_CPU_BUSY      25
#define IOPIN_PWM6          28
#define IOPIN_PWM0          29
#define IOPIN_PWM1          30
#define IOPIN_PWM2          31
#define IOPIN_USB_RXD       32
#define IOPIN_USB_TXD       33
#define IOPIN_PWM3          35
#define IOPIN_PWM4          36
#define IOPIN_PWM5          37
#define IOPIN_ADC0          40
#define IOPIN_ADC1          41
#define IOPIN_ADC2          42
#define IOPIN_ADC3          43
#define IOPIN_ADC4          44
#define IOPIN_ADC5          45
#define IOPIN_ADC6          46
#define IOPIN_ADC7          47
#define IOPIN_LED           48
#define IOPIN_TOSC1         51
#define IOPIN_TOSC2         52

// IO port numbers (check for conflict with IO pins above if changing!)
#define IOPORT_COUNTER_DATA 1

#endif

