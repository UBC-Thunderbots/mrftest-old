#ifndef CONSTANTS_H
#define CONSTANTS_H

// Bypass the controller and apply the velocity setpoints to the M (Pre-Scaler) matrix to allow direct control of motor power
#define MANUAL_ACTUATOR 1

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

// Baud rates to run the serial ports at.
#define BAUD_RATE_USB  115200
#define BAUD_RATE_XBEE 9600

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
#define IOPIN_COUNTER0_OE   4
#define IOPIN_COUNTER1_OE   5
#define IOPIN_COUNTER2_OE   6
#define IOPIN_COUNTER3_OE   7
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

// IO port numbers (check for conflict with IO pins above if changing!)
#define IOPORT_COUNTER_DATA 1

#endif
