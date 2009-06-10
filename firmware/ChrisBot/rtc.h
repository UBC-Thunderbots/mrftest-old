#ifndef RTC_H
#define RTC_H

/*
 * Initializes the real-time clock subsystem.
 */
void rtc_init(void);

/*
 * Do not access this variable.
 */
extern volatile unsigned long rtc_counter;

/*
 * Returns the current millisecond timestamp.
 */
static unsigned long rtc_millis(void) __attribute__((__always_inline__, __unused__));
static unsigned long rtc_millis(void) {
	uint8_t oldsreg;
	unsigned long ret;

	oldsreg = SREG;
	cli();
	ret = rtc_counter;
	SREG = oldsreg;
	return ret;
}

#endif

