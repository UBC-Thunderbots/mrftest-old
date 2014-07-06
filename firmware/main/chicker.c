/**
 * \defgroup CHICKER Kicker and Chipper Functions
 *
 * These functions provide the ability to kick the ball.
 *
 * @{
 */
#include "chicker.h"
#include "charger.h"
#include "adc.h"
#include "breakbeam.h"
#include <FreeRTOS.h>
#include <minmax.h>
#include <rcc.h>
#include <task.h>
#include <registers/timer.h>

/**
 * \brief The amount of time (in ticks) to delay after firing a device before allowing another fire, to avoid physical collisions.
 */
#define COLLIDE_TIMEOUT (500U / portTICK_PERIOD_MS)

/**
 * \brief Whether or not autokick is armed.
 */
static bool auto_enabled = false;

/**
 * \brief Whether or not autokick has fired since the last successful feedback packet.
 */
static bool auto_fired = false;

/**
 * \brief Which device to fire when autokicking.
 */
static chicker_device_t auto_device;

/**
 * \brief The pulse width to use when autokicking.
 */
static unsigned int auto_width;

/**
 * \brief Whether no device should be fired right now because it will physically collide with the previously fired device.
 */
static bool collide_timeout_active = false;

/**
 * \brief The number of ticks left until the collide timeout expires.
 */
static unsigned int collide_timeout;

/**
 * \brief Initializes the chicker subsystem.
 */
void chicker_init(void) {
	// Configure timers 9 and 11 with a prescaler suitable to count microseconds.
	// The ideal way to handle these timers would be to enable one-pulse PWM mode.
	// Each firing could then set CCR to 1, set ARR to pulse width plus one, and enable the counter.
	// Unfortunately, timer 11 does not support one-pulse mode.
	// So, instead, we use output-compare mode along with some forced levels to get things into the states we want.
	{
		rcc_enable_reset(APB2, TIM9);
		rcc_enable_reset(APB2, TIM11);
		TIM9.PSC = 168000000U / 1000000U;
		TIM11.PSC = 168000000U / 1000000U;
		TIM9_12_EGR_t egr9 = {
			.UG = 1, // Generate an update event (to push PSC into the timer)
		};
		TIM10_14_EGR_t egr11 = {
			.UG = 1, // Generate an update event (to push PSC into the timer)
		};
		TIM9.EGR = egr9;
		TIM11.EGR = egr11;
		TIM9_12_CCMR1_t ccmr1_9 = {
			.O = {
				.CC1S = 0b00, // Channel is an output
				.OC1FE = 0, // No special fast enable circuit
				.OC1PE = 0, // Writes to CCR1 happen immediately (we will be doing all that sort of thing with the timer disabled anyway)
				.OC1M = 0b100, // Force OC1REF to low level
			}
		};
		TIM10_14_CCMR1_t ccmr1_11 = {
			.O = {
				.CC1S = 0b00, // Channel is an output
				.OC1FE = 0, // No special fast enable circuit
				.OC1PE = 0, // Writes to CCR1 happen immediately (we will be doing all that sort of thing with the timer disabled anyway)
				.OC1M = 0b100, // Force OC1REF to low level
			}
		};
		TIM9.CCMR1 = ccmr1_9;
		TIM11.CCMR1 = ccmr1_11;
		TIM9_12_CCER_t ccer9 = {
			.CC1E = 1, // OC1 signal output to pin
			.CC1P = 0, // OC1 active high
			.CC1NP = 0, // Must be cleared for output mode
		};
		TIM10_14_CCER_t ccer11 = {
			.CC1E = 1, // OC1 signal output to pin
			.CC1P = 0, // OC1 active high
			.CC1NP = 0, // Must be cleared for output mode
		};
		TIM9.CCER = ccer9;
		TIM11.CCER = ccer11;
	}
}

/**
 * \brief Shuts down the chicker subsystem.
 *
 * \post The safe discharge process has discharged the capacitors.
 */
void chicker_shutdown(void) {
	chicker_discharge(true);
	unsigned int timeout = 30U; // Give up if we wait three seconds and have not discharged (else ADC failure could burn a lot of power forever).
	while (adc_capacitor() > CHICKER_DISCHARGE_THRESHOLD && timeout--) {
		vTaskDelay(100U / portTICK_PERIOD_MS);
	}
	chicker_discharge(false);
}

/**
 * \brief Enables or disables the safe discharge pulse generator.
 *
 * \warning The safe discharge pulse generator should be disabled once the capacitors are discharged!
 *
 * \param[in] discharge \c true to discharge, or \c false to not discharge
 */
void chicker_discharge(bool discharge) {
	if (discharge) {
		// To discharge, we generate a pulse train with 2.5% duty cycle and 1 ms period.

		// Do the kicker.
		{
			// Force output off to avoid spurious activity while reconfiguring.
			TIM10_14_CCMR1_t ccmr1 = {
				.O = {
					.CC1S = 0b00, // Channel is an output
					.OC1FE = 0, // No special fast enable circuit
					.OC1PE = 0, // Writes to CCR1 happen immediately (we will be doing all that sort of thing with the timer disabled anyway)
					.OC1M = 0b100, // Force OC1REF to low level
				}
			};
			TIM11.CCMR1 = ccmr1;

			// Disable the timer.
			TIM10_14_CR1_t cr1 = {
				.CEN = 0,
			};
			TIM11.CR1 = cr1;

			// Reconfigure timing parameters.
			TIM11.CNT = 0U;
			TIM11.ARR = 1000U * 4U;
			TIM11.CCR1 = 25U * 4U;

			// Enable the timer.
			cr1.CEN = 1;
			TIM11.CR1 = cr1;

			// Enable the output.
			ccmr1.O.OC1M = 0b110; // PWM mode 1
			TIM11.CCMR1 = ccmr1;
		}

		// Do the chipper.
		{
			// Force output off to avoid spurious activity while reconfiguring.
			TIM9_12_CCMR1_t ccmr1 = {
				.O = {
					.CC1S = 0b00, // Channel is an output
					.OC1FE = 0, // No special fast enable circuit
					.OC1PE = 0, // Writes to CCR1 happen immediately (we will be doing all that sort of thing with the timer disabled anyway)
					.OC1M = 0b100, // Force OC1REF to low level
				}
			};
			TIM9.CCMR1 = ccmr1;

			// Disable the timer.
			TIM9_12_CR1_t cr1 = {
				.CEN = 0,
			};
			TIM9.CR1 = cr1;

			// Reconfigure timing parameters.
			TIM9.CNT = 0U;
			TIM9.ARR = 1000U * 4U;
			TIM9.CCR1 = 25U * 4U;

			// Enable the timer.
			cr1.CEN = 1;
			TIM9.CR1 = cr1;

			// Enable the output.
			ccmr1.O.OC1M = 0b110; // PWM mode 1
			TIM9.CCMR1 = ccmr1;
		}

		// Because we are discharging, the capacitors cannot be full.
		charger_mark_fired();
	} else {
		// To stop discharging, find each timer that was in PWM mode and switch it to forced-low mode.
		// This will carefully not touch timers that are in output-compare inactive-on-match mode, as such timers are involved in firing, not discharging, operations.

		// Do the kicker.
		{
			TIM10_14_CCMR1_t ccmr1 = TIM11.CCMR1;
			if (ccmr1.O.OC1M == 0b110 /* PWM mode 1 */) {
				ccmr1.O.OC1M = 0b100; // Force OC1REF to low level
				TIM11.CCMR1 = ccmr1;
			}
		}

		// Do the chipper.
		{
			TIM9_12_CCMR1_t ccmr1 = TIM9.CCMR1;
			if (ccmr1.O.OC1M == 0b110 /* PWM mode 1 */) {
				ccmr1.O.OC1M = 0b100; // Force OC1REF to low level
				TIM9.CCMR1 = ccmr1;
			}
		}
	}
}

/**
 * \brief Fires a device.
 *
 * \param[in] device which device to fire
 * \param[in] width the width of the pulse, in kicking units
 */
void chicker_fire(chicker_device_t device, unsigned int width) {
	// If the collide timeout is active, we are not allowed to fire right now.
	if (__atomic_load_n(&collide_timeout_active, __ATOMIC_RELAXED)) {
		return;
	}

	if (device == CHICKER_KICK) {
		// Force output off in case it was already firing.
		TIM10_14_CCMR1_t ccmr1 = {
			.O = {
				.CC1S = 0b00, // Channel is an output
				.OC1FE = 0, // No special fast enable circuit
				.OC1PE = 0, // Writes to CCR1 happen immediately (we will be doing all that sort of thing with the timer disabled anyway)
				.OC1M = 0b100, // Force OC1REF to low level
			}
		};
		TIM11.CCMR1 = ccmr1;

		// Disable the timer.
		TIM10_14_CR1_t cr1 = {
			.CEN = 0,
		};
		TIM11.CR1 = cr1;

		// Load the pulse width, clamping at maximum.
		TIM11.ARR = MIN(width, 65535U);
		TIM11.CCR1 = MIN(width, 65535U);

		// Clear the counter.
		TIM11.CNT = 0U;

		// The next steps are timing-critical.
		portDISABLE_INTERRUPTS();

		// Begin the pulse by forcing the output high.
		ccmr1.O.OC1M = 0b101;
		TIM11.CCMR1 = ccmr1;

		// Switch the timer to output-compare inactive-on-match mode.
		ccmr1.O.OC1M = 0b010U;
		TIM11.CCMR1 = ccmr1;

		// Start the counter.
		cr1.CEN = 1;
		TIM11.CR1 = cr1;

		// End of timing-critical steps.
		portENABLE_INTERRUPTS();
	} else {
		// Force output off in case it was already firing.
		TIM9_12_CCMR1_t ccmr1 = {
			.O = {
				.CC1S = 0b00, // Channel is an output
				.OC1FE = 0, // No special fast enable circuit
				.OC1PE = 0, // Writes to CCR1 happen immediately (we will be doing all that sort of thing with the timer disabled anyway)
				.OC1M = 0b100, // Force OC1REF to low level
			}
		};
		TIM9.CCMR1 = ccmr1;

		// Disable the timer.
		TIM9_12_CR1_t cr1 = {
			.CEN = 0,
		};
		TIM9.CR1 = cr1;

		// Load the pulse width, clamping at maximum.
		TIM9.ARR = MIN(width, 65535U);
		TIM9.CCR1 = MIN(width, 65535U);

		// Clear the counter.
		TIM9.CNT = 0U;

		// The next steps are timing-critical.
		portDISABLE_INTERRUPTS();

		// Begin the pulse by forcing the output high.
		ccmr1.O.OC1M = 0b101;
		TIM9.CCMR1 = ccmr1;

		// Switch the timer to output-compare inactive-on-match mode.
		ccmr1.O.OC1M = 0b010U;
		TIM9.CCMR1 = ccmr1;

		// Start the counter.
		cr1.CEN = 1;
		TIM9.CR1 = cr1;

		// End of timing-critical steps.
		portENABLE_INTERRUPTS();
	}

	// Because we are discharging, the capacitors cannot be full.
	charger_mark_fired();

	// Set a timeout to avoid colliding with the hardware.
	taskENTER_CRITICAL();
	collide_timeout = COLLIDE_TIMEOUT;
	collide_timeout_active = true;
	taskEXIT_CRITICAL();
}

/**
 * \brief Arms auto-fire mode.
 *
 * In auto-fire mode, the device is fired as soon as the break beam is interrupted.
 *
 * \param[in] device which device to fire
 * \param[in] width the width of the pulse, in kicking units
 */
void chicker_auto_arm(chicker_device_t device, unsigned int width) {
	// The auto_device and auto_width variables are only read from an ISR, and only if auto_enabled is true.
	// Thus, auto_enabled being false can itself protect writes to auto_device and auto_width, because ISRs are themselves implicitly atomic.
	__atomic_store_n(&auto_enabled, false, __ATOMIC_RELAXED);
	__atomic_signal_fence(__ATOMIC_ACQUIRE);
	auto_device = device;
	auto_width = width;
	__atomic_signal_fence(__ATOMIC_RELEASE);
	__atomic_store_n(&auto_enabled, true, __ATOMIC_RELAXED);
}

/**
 * \brief Disarms auto-fire mode.
 *
 * Auto-fire mode is also automatically disarmed after it fires.
 */
void chicker_auto_disarm(void) {
	__atomic_store_n(&auto_enabled, false, __ATOMIC_RELAXED);
}

/**
 * \brief Checks whether auto-fire mode is armed.
 *
 * \retval true auto-fire mode is armed
 * \retval false auto-fire mode is disarmed
 */
bool chicker_auto_armed(void) {
	return __atomic_load_n(&auto_enabled, __ATOMIC_RELAXED);
}

/**
 * \brief Checks whether auto-fire has fired.
 *
 * \retval true auto-fire has fired since the last call to this function
 * \retval false auto-fire has not fired
 */
bool chicker_auto_fired_test_clear(void) {
	const bool NEW_VALUE = false;
	bool ret;
	__atomic_exchange(&auto_fired, &NEW_VALUE, &ret, __ATOMIC_RELAXED);
	return ret;
}

/**
 * \brief Counts down the timeout between consecutive fires.
 *
 * This function runs in the normal-speed tick task.
 */
void chicker_tick(void) {
	if (!__atomic_sub_fetch(&collide_timeout, 1U, __ATOMIC_RELAXED)) {
		__atomic_store_n(&collide_timeout_active, false, __ATOMIC_RELAXED);
	}
}

/**
 * \brief Checks for auto-fire eligibility.
 *
 * This function runs in the fast ISR.
 */
void chicker_tick_fast(void) {
	// Check if we should fire now.
	if (auto_enabled && breakbeam_interrupted() && charger_full() && !collide_timeout_active) {
		chicker_fire(auto_device, auto_width);
		auto_enabled = false;
		auto_fired = true;
	}
}

/**
 * @}
 */

