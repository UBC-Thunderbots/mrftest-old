#include <pic18fregs.h>
#include <stdint.h>
#include "dongle_status.h"
#include "estop.h"
#include "pins.h"

/**
 * \brief Converts a voltage to an ADC reading.
 */
#define VOLTS_TO_ADC(x) ((uint16_t) ((x) / 3.3 * 1023))

#define VOLTS_STOP 1.086501
#define VOLTS_RUN 2.2422859
#define ADC_STOP_MIN VOLTS_TO_ADC(VOLTS_STOP / 2)
#define ADC_RUN_MIN VOLTS_TO_ADC((VOLTS_STOP + VOLTS_RUN) / 2)
#define ADC_RUN_MAX VOLTS_TO_ADC((VOLTS_RUN + 3.3) / 2)

#define RUN_DEBOUNCE_MAX 25
static uint8_t run_debounce;

void estop_init(void) {
	/* Clear the debounce counter. */
	run_debounce = RUN_DEBOUNCE_MAX;

	/* Send power to the estop switch. */
	LAT_ESTOP_OUT1 = 1;

	/* Configure the ADC. */

	/*         /-------- Use VSS as VREF-
	 *         |/------- Use VDD as VREF+
	 *         ||////--- Select channel 0
	 *         ||||||/-- Do not start acquisition
	 *         |||||||/- Turn on ADC */
	ADCON0 = 0b00000001;

	/*         /-------- Right-justify conversion results
	 *         |/------- Execute calibration on next acquisition
	 *         ||///---- Set acquisition time 4TAD = 5⅓µs
	 *         |||||///- Set conversion clock TAD = Fosc/64 = 1⅓µs */
	ADCON1 = 0b11010110;

	/* Begin a conversion; since ADCAL=1 this will be a calibration and not a real conversion. */
	ADCON0bits.GO = 1;
	while (ADCON0bits.GO);

	/* Enable ADC interrupts and start an acquisition. */
	IPR1bits.ADIP = 0;
	PIR1bits.ADIF = 0;
	PIE1bits.ADIE = 1;
	ADCON1bits.ADCAL = 0;
	ADCON0bits.GO = 1;

	/* Enable timer 3 with a 1:8 prescale and internal clock. */
	TMR3H = 0;
	TMR3L = 0;
	T3CONbits.T3CKPS0 = 1;
	T3CONbits.T3CKPS1 = 1;
	T3CONbits.TMR3ON = 1;

	/* Configure ECCP2 to compare to 15000 (1/12000000 (clock period) * 8 (prescale) * 15000 = 10ms) and start an ADC conversion and reset the timer. */
	/*          ////----- Unused in compare mode
	 *          ||||////- Compare mode with special event trigger on match */
	CCP2CON = 0b00001011;
	CCPR2H = 15000 >> 8;
	CCPR2L = 15000 & 0xFF;
}

void estop_deinit(void) {
	/* Disable timer 3 and ECCP2 to prevent further conversions from starting. */
	T3CONbits.TMR3ON = 0;
	TMR3H = 0;
	TMR3L = 0;
	CCP2CON = 0;

	/* Wait for any current acquisition to complete. */
	while (ADCON0bits.GO);

	/* Flush any pending ADC interrupts. */
	PIE1bits.ADIE = 0;
	PIR1bits.ADIF = 0;

	/* Turn off the ADC. */
	ADCON0bits.ADON = 0;

	/* Cut power to the estop switch. */
	LAT_ESTOP_OUT1 = 0;

	/* Update the dongle status block to indicate that the emergency stop is uninitialized. */
	dongle_status.estop = ESTOP_STATE_UNINITIALIZED;
	dongle_status_dirty();
}

SIGHANDLER(estop_adif) {
	uint16_t value;

	/* Read the ADC result. */
	value = (ADRESH << 8) | ADRESL;

	/* Check what its value is. */
	if (value < ADC_STOP_MIN) {
		dongle_status.estop = ESTOP_STATE_DISCONNECTED;
		run_debounce = RUN_DEBOUNCE_MAX;
	} else if (value < ADC_RUN_MIN) {
		dongle_status.estop = ESTOP_STATE_STOP;
		run_debounce = RUN_DEBOUNCE_MAX;
	} else if (value < ADC_RUN_MAX) {
		if (!run_debounce) {
			dongle_status.estop = ESTOP_STATE_RUN;
		} else {
			run_debounce--;
		}
	} else {
		dongle_status.estop = ESTOP_STATE_DISCONNECTED;
		run_debounce = RUN_DEBOUNCE_MAX;
	}
	dongle_status_dirty();

	/* Clear interrupt. */
	PIR1bits.ADIF = 0;
}

