#include <avr/io.h>
#include <avr/sleep.h>
#include <util/delay.h>

#include "constants.h"
#include "debug.h"
#include "iopins.h"
#include "pwmpins.h"
#include "adcpins.h"
#include "rtc.h"
#include "filter.h"
#include "gyro.h"
#include "xbee.h"
#include "wheel.h"
#include "accelerometer.h"

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
static struct wheel wheels[4];

// The accelerometers.
static const double accelerometer_filter_a[3] = {1.0, -1.8229, 0.8374};
static const double accelerometer_filter_b[3] = {0.0036, 0.0072, 0.0036};
static struct accelerometer accelerometer_x, accelerometer_y;

// Record the time the last kick was started.
static unsigned long kick_time;

// Record the time the loop last ran.
static unsigned long last_loop_time;

// Record the time we last transmitted battery data.
static unsigned long last_battery_time;

// Whether the battery voltage is low.
static uint8_t low_battery;

// Whether to estop.
static uint8_t estop;

/*
 * Zeroes everything.
 */
static void nuke(void) {
	uint8_t i;

	pwm_write(PWMPIN_KICKER, 0);
	pwm_write(PWMPIN_DRIBBLER, 0);
	pwm_write(PWMPIN_MOTOR0, 0);
	pwm_write(PWMPIN_MOTOR1, 0);
	pwm_write(PWMPIN_MOTOR2, 0);
	pwm_write(PWMPIN_MOTOR3, 0);

	for (i = 0; i < 4; i++)
		wheel_clear(&wheels[i]);

	filter_clear(&setpoint_x_filter);
	filter_clear(&setpoint_y_filter);
	filter_clear(&vx_controller);
	filter_clear(&vy_controller);
	filter_clear(&vt_controller);
	filter_clear(&ff_controller);

	accelerometer_clear(&accelerometer_x);
	accelerometer_clear(&accelerometer_y);

	iopin_write(IOPIN_COUNTER_RESET, 0);
	_delay_us(100);
	iopin_write(IOPIN_COUNTER_RESET, 1);
}

/*
 * All the stuff that needs critical timing.
 */
static void loop_timed(void) {
	double vt, vx, vy;
	double vt_setpoint, vx_setpoint, vy_setpoint;
	double vx_filtered_setpoint, vy_filtered_setpoint;
	double vx_actuator, vy_actuator, vt_actuator;

	// Check for estop case.
	if (estop)
		return;

	// Perform ADC sampling.
	adc_sample();

	// Read data from sensors.
	vt = gyro_read();
	vx = accelerometer_update(&accelerometer_x, vt);
	vy = accelerometer_update(&accelerometer_y, vt);
	wheel_update_rpm(&wheels[0]);
	wheel_update_rpm(&wheels[1]);
	wheel_update_rpm(&wheels[2]);
	wheel_update_rpm(&wheels[3]);

	// Extract setpoints from most-recently-received packet.
	vx_setpoint = xbee_rxdata.vx / 127.0 * MAX_SP_VX;
	vy_setpoint = xbee_rxdata.vy / 127.0 * MAX_SP_VY;
	vt_setpoint = xbee_rxdata.vt / 127.0 * MAX_SP_VT;

	// Reset the accelerometer integrator to the measured camera velocity if available.
	if (xbee_rxdata.vx_measured != -128 && xbee_rxdata.vy_measured != -128 && rtc_millis() - xbee_rxtimestamp < LOOP_TIME) {
		accelerometer_set(&accelerometer_x, xbee_rxdata.vx_measured / 127.0 * MAX_SP_VX);
		accelerometer_set(&accelerometer_y, xbee_rxdata.vy_measured / 127.0 * MAX_SP_VY);
	}

	// Filter the linear setpoints.
	vx_filtered_setpoint = filter_process(&setpoint_x_filter, vx_setpoint);
	vy_filtered_setpoint = filter_process(&setpoint_y_filter, vy_setpoint);

	// Process errors through controllers to generate actuator levels.
	// If controller is disabled pass setpoint through to actuators.
#if X_CONTROLLER_ENABLED
	vx_actuator = filter_process(&vx_controller, vx_filtered_setpoint - vx);
#else
	vx_actuator = vx_filtered_setpoint;
#endif

#if Y_CONTROLLER_ENABLED
	vy_actuator = filter_process(&vy_controller, vy_filtered_setpoint - vy);
#else
	vy_actuator = vy_filtered_setpoint;
#endif

#if T_CONTROLLER_ENABLED
	vt_actuator = filter_process(&vt_controller, vt_setpoint - vt);
#else
	vt_actuator = vt_setpoint;
#endif

	// Feed forward corrects theta actuator and uses X actuator as input.
	// Therefore this can be shut off entirely if not used.
#if F_CONTROLLER_ENABLED
	vt_actuator += filter_process(&ff_controller, vx_actuator);
#endif

	// Drive wheels. Prescaling matrix is stored internally in wheel class.
	wheel_update_drive(&wheels[0], vx_actuator, vy_actuator, vt_actuator);
	wheel_update_drive(&wheels[1], vx_actuator, vy_actuator, vt_actuator);
	wheel_update_drive(&wheels[2], vx_actuator, vy_actuator, vt_actuator);
	wheel_update_drive(&wheels[3], vx_actuator, vy_actuator, vt_actuator);
}

/*
 * All the stuff that does NOT need critical timing.
 */
static void loop_untimed(void) {
	uint16_t battery_level;

	// Send battery voltage.
	if (rtc_millis() - last_battery_time > TIMEOUT_BATTERY) {
		battery_level = adc_read(ADCPIN_GREEN_BATTERY);
		if (battery_level < GREEN_BATTERY_LOW / GREEN_BATTERY_CONVERSION)
			low_battery = 1;
		xbee_txdata.v_green[0] = battery_level / 256;
		xbee_txdata.v_green[1] = battery_level % 256;

		battery_level = adc_read(ADCPIN_MOTOR_BATTERY);
		if (battery_level < MOTOR_BATTERY_LOW / MOTOR_BATTERY_CONVERSION)
			low_battery = 1;
		xbee_txdata.v_motor[0] = battery_level / 256;
		xbee_txdata.v_motor[1] = battery_level % 256;

		xbee_send();
		last_battery_time += TIMEOUT_BATTERY;
	}

	// Try receiving data from the XBee.
	xbee_receive();

	// Check if we're asked to reboot.
	if (xbee_rxdata.reboot) {
		debug_puts("Bot: Reboot\n");
		nuke();
		WDTCR = _BV(WDE) | _BV(WDP2);
		for (;;);
	}

	// Check if we're in ESTOP mode.
	low_battery = 0;
	estop = xbee_rxdata.emergency || rtc_millis() - xbee_rxtimestamp > TIMEOUT_RECEIVE || !xbee_rxtimestamp || low_battery;
	if (estop) {
		nuke();
		return;
	}

	// Start a kick, if requested.
	if (xbee_rxdata.kick && !kick_time) {
		pwm_write(PWMPIN_KICKER, xbee_rxdata.kick * 1023UL / 255UL);
		kick_time = rtc_millis();
	}

	// Finish a kick, if appropriate.
	if (kick_time && rtc_millis() - kick_time > KICK_TIME) {
		pwm_write(PWMPIN_KICKER, 0);
		kick_time = 0;
	}

	// Drive the dribbler.
	pwm_write(PWMPIN_DRIBBLER, xbee_rxdata.dribble * 1023UL / 255UL);
}

/*
 * Application entry point.
 */
int main(void) __attribute__((__noreturn__));
int main(void) {
	// Turn on interrupts.
	sei();

	// Disable peripherals we don't need that start out enabled.
	MCUCSR |= _BV(JTD); // JTAG debug interface
	MCUCSR |= _BV(JTD); // (timed sequence, write twice)
	ACSR = _BV(ACD);    // Analog comparator

	// Initialize modules.
	debug_init();
	rtc_init();
	adc_init();
	pwm_init();

	// Display a message.
	debug_puts("Bot: Initializing...\n");

	// Configure IO pins.
	iopin_write(IOPIN_LED, 1);
	iopin_configure_output(IOPIN_LED);

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

	ioport_configure_input(IOPORT_COUNTER_DATA);

	// Initialize the filters and controllers with their coefficients.
	filter_init(&setpoint_x_filter, setpoint_filter_a, setpoint_filter_b);
	filter_init(&setpoint_y_filter, setpoint_filter_a, setpoint_filter_b);
	filter_init(&vx_controller, vx_controller_a, vx_controller_b);
	filter_init(&vy_controller, vy_controller_a, vy_controller_b);
	filter_init(&vt_controller, vt_controller_a, vt_controller_b);
	filter_init(&ff_controller, ff_controller_a, ff_controller_b);
	wheel_init(&wheels[0], IOPIN_COUNTER0_OE, IOPIN_MOTOR0A, IOPIN_MOTOR0B, PWMPIN_MOTOR0, m[0], rpm_filter_a, rpm_filter_b, wheel_controller_a, wheel_controller_b);
	wheel_init(&wheels[1], IOPIN_COUNTER1_OE, IOPIN_MOTOR1A, IOPIN_MOTOR1B, PWMPIN_MOTOR1, m[1], rpm_filter_a, rpm_filter_b, wheel_controller_a, wheel_controller_b);
	wheel_init(&wheels[2], IOPIN_COUNTER2_OE, IOPIN_MOTOR2A, IOPIN_MOTOR2B, PWMPIN_MOTOR2, m[2], rpm_filter_a, rpm_filter_b, wheel_controller_a, wheel_controller_b);
	wheel_init(&wheels[3], IOPIN_COUNTER3_OE, IOPIN_MOTOR3A, IOPIN_MOTOR3B, PWMPIN_MOTOR3, m[3], rpm_filter_a, rpm_filter_b, wheel_controller_a, wheel_controller_b);
	accelerometer_init(&accelerometer_x, accelerometer_filter_a, accelerometer_filter_b, ADCPIN_ACCEL1Y,  6.5);
	accelerometer_init(&accelerometer_y, accelerometer_filter_a, accelerometer_filter_b, ADCPIN_ACCEL2Y, -4.5);

	// Initialize the XBee.
	xbee_init();

	// Initialize the gyroscope.
	gyro_init();

	// Initialize CPU sleep mode.
	set_sleep_mode(SLEEP_MODE_IDLE);

	// Initialize timestamps.
	kick_time = last_loop_time = last_battery_time = rtc_millis();

	// Initialization complete.
	debug_puts("Bot: Initialized.\n");

	// Begin iterating.
	for (;;) {
		// Indicate that the CPU is idle.
		iopin_write(IOPIN_CPU_BUSY, 0);

		// Wait for the next time point.
		while (rtc_millis() - last_loop_time < LOOP_TIME) {
			// VERY CAREFULLY go to sleep while avoiding race conditions with interrupts!
			cli();
			if (rtc_millis() - last_loop_time < LOOP_TIME) {
				sleep_enable();
				sei();       // These are taken as two consecutive machine instructions.
				sleep_cpu(); // SEI will not allow interrupts until SLEEP has executed.
				sleep_disable();
			}
			sei();
		}

		// Indicate that the CPU is busy.
		iopin_write(IOPIN_CPU_BUSY, 1);

		// Mark the advancement of time for the loop.
		last_loop_time += LOOP_TIME;

		// Execute those operations that need very precise timing.
		loop_timed();

		// Execute the operations whose timing requirements are loose.
		loop_untimed();

		// Blink the LED at 2Hz.
		iopin_write(IOPIN_LED, rtc_millis() / 250 % 2);
	}
}

