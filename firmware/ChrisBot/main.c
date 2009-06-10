#include <avr/io.h>
#include <util/delay.h>

#include "constants.h"
#include "debug.h"
#include "iopins.h"
#include "pwmpins.h"
#include "adcpins.h"
#include "rtc.h"
#include "filter.h"
#include "led.h"

// The setpoint filters for the linear velocity setpoints.
static const double setpoint_filter_a[3] = {1.0, 0.0, 0.0};
static const double setpoint_filter_b[3] = {1.0, 0.0, 0.0};
static struct filter setpoint_x_filter, setpoint_y_filter;

// The linear acceleration controllers.
static const double vx_controller_a[3] = {1.0, 0.0, 0.0};
static const double vx_controller_b[3] = {0.000, 0.000, 0.000};
static struct filter vx_controller;

static const double vy_controller_a[3] = {1.0, 0.0, 0.0};
static const double vy_controller_b[3] = {0.000, 0.000, 0.000};
static struct filter vy_controller;

// The angular velocity controller.
static const double vt_controller_a[3] = {1.0, -1.9724, 0.9724};
static const double vt_controller_b[3] = {6.2131, -12.3272, 6.1145};
static struct filter vt_controller;

// The feedforward controller.
static const double ff_controller_a[3] = {1.0, 0.0, 0.0};
static const double ff_controller_b[3] = {3.0/16.0, 0.0, 0.0};
static struct filter ff_controller;

// The wheels.
static const double rpm_filter_a[3] = {1.0, -0.777778, 0.0};
static const double rpm_filter_b[3] = {0.111111, 0.111111, 0.0};
static const double wheel_controller_a[3] = {1.0, -1.7967, 0.7967};
static const double wheel_controller_b[3] = {1.2369, -2.0521, 0.8397};
static const double m[4][3] = {
  {-0.0028,  0.0007, 0.0065},
  {-0.0028, -0.0007, 0.0065},
  { 0.0010, -0.0010, 0.0065},
  { 0.0010,  0.0010, 0.0065}
};
static struct filter rpm_filters[4];
static struct filter wheel_controllers[4];

// The accelerometers.
static const double accelerometer_filter_a[3] = {1.0, -1.8229, 0.8374};
static const double accelerometer_filter_b[3] = {0.0036, 0.0072, 0.0036};
static struct filter accelerometer_x_filter, accelerometer_y_filter;

// The gyro's zero point.
static double gyro_zero;

// Record the time of the last received message.
static unsigned long last_message_time;

// Record the time the last kick was started.
static unsigned long kick_time;

// Record the time the loop last ran.
static unsigned long last_loop_time;

// Record the time we last transmitted battery data.
static unsigned long last_battery_time;

// Whether the battery voltage is low.
static uint8_t low_battery;

/*
 * Zeroes the gyroscope.
 */
static void init_gyro(void) {
	int accumulator = 0;
	uint8_t i;

	i = GYRO_ZERO_SAMPLES;
	do {
		_delay_ms(2);
		accumulator += adc_results[ADCPIN_GYRO_DATA] - adc_results[ADCPIN_GYRO_VREF];
	} while (--i);
	gyro_zero = (double) accumulator / GYRO_ZERO_SAMPLES;
}

/*
 * Reads the gyroscope.
 */
static double read_gyro(void) {
	return adc_results[ADCPIN_GYRO_DATA] - adc_results[ADCPIN_GYRO_VREF] - gyro_zero;
}

/*
 * Application entry point.
 */
int main(void) __attribute__((__noreturn__));
int main(void) {
	// Turn on interrupts.
	sei();

	// Initialize modules.
	led_init();
	led_on();
	debug_init();
	rtc_init();
	adc_init();
	pwm_init();

	// Display a message.
	debug_puts("Bot: Initializing...\n");

	// Configure IO pins.
	iopin_write(IOPIN_CPU_BUSY, 1);
	iopin_configure_output(IOPIN_CPU_BUSY);

	iopin_write(IOPIN_COUNTER0_OE, 1);
	iopin_write(IOPIN_COUNTER1_OE, 1);
	iopin_write(IOPIN_COUNTER2_OE, 1);
	iopin_write(IOPIN_COUNTER3_OE, 1);
	iopin_configure_output(IOPIN_COUNTER0_OE);
	iopin_configure_output(IOPIN_COUNTER1_OE);
	iopin_configure_output(IOPIN_COUNTER2_OE);
	iopin_configure_output(IOPIN_COUNTER3_OE);

	iopin_write(IOPIN_MOTOR0A, 1);
	iopin_write(IOPIN_MOTOR1A, 1);
	iopin_write(IOPIN_MOTOR2A, 1);
	iopin_write(IOPIN_MOTOR3A, 1);
	iopin_configure_output(IOPIN_MOTOR0A);
	iopin_configure_output(IOPIN_MOTOR0B);
	iopin_configure_output(IOPIN_MOTOR1A);
	iopin_configure_output(IOPIN_MOTOR1B);
	iopin_configure_output(IOPIN_MOTOR2A);
	iopin_configure_output(IOPIN_MOTOR2B);
	iopin_configure_output(IOPIN_MOTOR3A);
	iopin_configure_output(IOPIN_MOTOR3B);

	iopin_configure_output(IOPIN_COUNTER_RESET);
	_delay_us(100);
	iopin_write(IOPIN_COUNTER_RESET, 1);

	// Initialize the filters and controllers with their coefficients.
	filter_init(&setpoint_x_filter, setpoint_filter_a, setpoint_filter_b);
	filter_init(&setpoint_y_filter, setpoint_filter_a, setpoint_filter_b);
	filter_init(&vx_controller, vx_controller_a, vx_controller_b);
	filter_init(&vy_controller, vy_controller_a, vy_controller_b);
	filter_init(&vt_controller, vt_controller_a, vt_controller_b);
	filter_init(&ff_controller, ff_controller_a, ff_controller_b);
	filter_init(&rpm_filters[0], rpm_filter_a, rpm_filter_b);
	filter_init(&rpm_filters[1], rpm_filter_a, rpm_filter_b);
	filter_init(&rpm_filters[2], rpm_filter_a, rpm_filter_b);
	filter_init(&rpm_filters[3], rpm_filter_a, rpm_filter_b);
	filter_init(&wheel_controllers[0], wheel_controller_a, wheel_controller_b);
	filter_init(&wheel_controllers[1], wheel_controller_a, wheel_controller_b);
	filter_init(&wheel_controllers[2], wheel_controller_a, wheel_controller_b);
	filter_init(&wheel_controllers[3], wheel_controller_a, wheel_controller_b);
	filter_init(&accelerometer_x_filter, accelerometer_filter_a, accelerometer_filter_b);
	filter_init(&accelerometer_y_filter, accelerometer_filter_a, accelerometer_filter_b);

	// Initialize the gyroscope.
	init_gyro();

	// Initialization complete.
	debug_puts("Bot: Initialized.\n");
	iopin_write(IOPIN_CPU_BUSY, 0);
	led_off();

	// Begin iterating.
	for (;;) {
	}
}

