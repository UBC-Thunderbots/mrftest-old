#include "global.h"
#include "pins.h"
#include "usb.h"
#include "xbee_rxpacket.h"
#include "xbee_txpacket.h"
#include <delay.h>
#include <pic18fregs.h>

volatile BOOL should_start_up = false;

volatile BOOL should_shut_down = false;

volatile uint8_t requested_channels[2] = { 0, 0 };

uint16_t xbee_versions[2];

void check_idle(void) {
	uint8_t saved_stuff;

	/* Double-checked locking paradigm:
	 * There might be a race where we were idle but become active right after the check.
	 * The interrupt would be taken and clear usb_is_idle, but we'd put the MCU to sleep because we were already in the if.
	 * This would be bad, because we'd put the MCU to sleep while the SIE was not suspended!
	 * Instead, if we think we're idle, disable interrupts and check again.
	 * Only if that second check also passes do we actually go to sleep.
	 * Note that we never re-enable GIEH during the body of the if.
	 * Therefore, bus activity will cause ACVTIF to become set but no interrupt will be taken immediately.
	 * The SLEEP instruction does not go to sleep if any interrupt is pending, or wakes up if any interrupt becomes pending.
	 * That is the "atomic" instruction in this system: it guarantees to sleep only if, and as long as, no interrupt is pending.
	 * Once ACTVIF is pending, we wake up, bring up the hardware, and re-enable GIEH.
	 * Only then is the interrupt taken. */
	if (usb_is_idle) {
		INTCONbits.GIEH = 0;
		if (usb_is_idle) {
			/* Flush and suspend communication. */
			xbee_txpacket_suspend();
			xbee_rxpacket_suspend();

			/* Save the states of and turn off all LEDs and the XBees. */
			saved_stuff = 0;
			if (LAT_LED1) {
				saved_stuff |= 1;
			}
			if (LAT_LED2) {
				saved_stuff |= 2;
			}
			if (LAT_LED3) {
				saved_stuff |= 4;
			}
			if (!LAT_XBEE0_SLEEP) {
				saved_stuff |= 8;
			}
			if (!LAT_XBEE1_SLEEP) {
				saved_stuff |= 16;
			}
			LAT_LED1 = 0;
			LAT_LED2 = 0;
			LAT_LED3 = 0;
			LAT_XBEE0_SLEEP = 1;
			LAT_XBEE1_SLEEP = 1;

			/* Disable the PLL. */
			OSCTUNEbits.PLLEN = 0;

			/* Go to sleep. */
			Sleep();

			/* Wait for the crystal oscillator to start back up. */
			while (!OSCCONbits.OSTS);

			/* Enable the PLL and wait for it to lock. This may take up to 2ms. */
			OSCTUNEbits.PLLEN = 1;
			delay1ktcy(2);

			/* Turn back on all the hardware we turned off.
			 * As described above, this will not race with things like SET_CONFIGURATION callbacks.
			 * Reason: WE HAVE NOT YET TAKEN THE INTERRUPT THAT WOKE US UP.
			 * The entire body of this if block is a critical section! */
			if (saved_stuff & 1) {
				LAT_LED1 = 1;
			}
			if (saved_stuff & 2) {
				LAT_LED2 = 1;
			}
			if (saved_stuff & 4) {
				LAT_LED3 = 1;
			}
			if (saved_stuff & 8) {
				LAT_XBEE0_SLEEP = 0;
			}
			if (saved_stuff & 16) {
				LAT_XBEE1_SLEEP = 0;
			}

			/* Restart communication. */
			xbee_txpacket_resume();
			xbee_rxpacket_resume();
		}
		INTCONbits.GIEH = 1;
	}
}

