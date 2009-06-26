#ifndef CONSTANTS_H
#define CONSTANTS_H

// Indicate CPU speed.
#define F_CPU 16000000UL

// Allows bypassing various controllers and running in open loop.
#define T_CONTROLLER_ENABLED 1
#define F_CONTROLLER_ENABLED 1
#define W_CONTROLLER_ENABLED 1

// The scaling factor applied to VT setpoint when T controller is disabled or gyro is absent.
#define VT_CONTROLLERLESS_SCALE (41.0 / 57.0)

// Allows ignoring wireless data and using a locally-configured test instead.
#define TEST_MODE 0

// Parameters for the debug port.
#define DEBUG_BAUD 115200UL
#define DEBUG_BUFSIZE 128

// The scaling factor applied to the setpoint to convert to a motor power when W_CONTROLLER_ENABLED == 0.
#define MANUAL_ACTUATOR_MOTOR_SCALE 8.0

// Maximum representable values for the setpoints.
#define MAX_SP_VX 200.0
#define MAX_SP_VY 500.0
#define MAX_SP_VT   4.0

// Maximum motor power (0-1023)
#define MOTOR_CAP 1023

// This RPM is considered 100%
#define MOTOR_MAX_RPM 5200.0

// Time of each interation in milliseconds
#define LOOP_TIME 4

// Scaler that converts Motor counts to RPM
#define ENCODER_COUNTS_TO_RPM 166.67

// Conversion factors from ADC reading to battery voltage.
#define GREEN_BATTERY_CONVERSION(x) (((x) / 1024.0 * 5.0) / 10.0 * 32.0)
#define GREEN_BATTERY_LOW 9.0

#define MOTOR_BATTERY_CONVERSION(x) (((x) / 1024.0 * 5.0) / 10.0 * 43.0)
#define MOTOR_BATTERY_LOW 14.4

// Scaler radians/second per ADC unit
#define GYRO_TO_RADS 0.0127768

// Number of samples to take while zeroing the gyro.
#define GYRO_ZERO_SAMPLES 50

// Scaler cm/second^2 per ADC unit
#define ACCELEROMETER_TO_CM 4.785156

// Number of samples to take while zeroing the accelerometers.
#if X_CONTROLLER_ENABLED || Y_CONTROLLER_ENABLED
#define ACCELEROMETER_ZERO_SAMPLES 50
#else
#define ACCELEROMETER_ZERO_SAMPLES 0
#endif

// How long to fire the kicker for (ms)
#define KICK_TIME 200

// XBee configuration.
#define XBEE_BAUD 38400UL
#define XBEE_POWER_LEVEL "4"
#define XBEE_PAN "7495"

// Timeout for receiving a data packet before nuking (ms).
#define TIMEOUT_RECEIVE 500

// Number of microseconds to delay before starting an ADC to allow the channel to settle.
#define ADC_SETTLE_DELAY 3

// PWM pin numbers
#define PWMPIN_MOTOR0   0
#define PWMPIN_MOTOR1   1
#define PWMPIN_MOTOR2   2
#define PWMPIN_MOTOR3   3
#define PWMPIN_DRIBBLER 4
#define PWMPIN_KICKER   5

// ADC pin numbers
#define ADCPIN_MOTOR_BATTERY_HACKED 0
#define ADCPIN_GYRO_DATA            4
#define ADCPIN_GYRO_VREF            5
#define ADCPIN_MOTOR_BATTERY        6
#define ADCPIN_GREEN_BATTERY        7

// IO pin numbers (check for conflicts with IO ports below if changing!)
#define IOPIN_BATHACK_1     0
#define IOPIN_BATHACK_2     1
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

