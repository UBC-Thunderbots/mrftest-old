#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>

#define BAUD 115200
#define BAUD_DIVISOR (((F_CPU + 4UL * BAUD) / (8UL * BAUD)) - 1UL)

static uint8_t ticks_since_last_send;
static uint8_t last_pin_state;

ISR(TIMER0_COMP_vect, ISR_BLOCK) {
	uint8_t cur_pin_state;

	// Check if pin state has changed.
	cur_pin_state = PINB & _BV(0);
	if (cur_pin_state != last_pin_state || ticks_since_last_send == 50) {
		// Either the switch was changed or else it's been half a second since the last message.
		// Send a message now and reset the tick counter.
		ticks_since_last_send = 1;
		if (cur_pin_state)
			UDR0 = '0';
		else
			UDR0 = '1';
		// We need to keep the system clock running until the transmission is complete.
		set_sleep_mode(SLEEP_MODE_IDLE);
	} else {
		// Less than half a second since last notify and no new data. Don't bother sending.
		ticks_since_last_send++;
	}

	// Remember the current pin state.
	last_pin_state = cur_pin_state;
}

ISR(USART0_TX_vect, ISR_BLOCK) {
	// Transmission complete. Main clock is no longer needed; go into powersave mode.
	set_sleep_mode(SLEEP_MODE_PWR_SAVE);
}

int main(void) __attribute__((__noreturn__));
int main(void) {
	// Disable peripherals we don't need that start out enabled.
	MCUCSR |= _BV(JTD); // JTAG debug interface
	MCUCSR |= _BV(JTD); // (timed sequence, write twice)
	ACSR = _BV(ACD);    // Analog comparator

	// Configure the serial port.
	UBRR0H = BAUD_DIVISOR / 256UL;
	UBRR0L = BAUD_DIVISOR % 256UL;
	UCSR0A = _BV(U2X0);
	UCSR0B = _BV(TXCIE) | _BV(TXEN0);
	UCSR0C = _BV(USBS0) | _BV(UCSZ01) | _BV(UCSZ00);

	// Configure RTC on timer 0 using secondary crystal to expire around every 10ms.
	ASSR = _BV(AS0);
	TCNT0 = 0;
	OCR0 = 41;
	TCCR0 = _BV(WGM01) | _BV(CS01);
	while (ASSR & (_BV(TCN0UB) | _BV(OCR0UB) | _BV(TCR0UB)));
	TIMSK |= _BV(OCIE0);

	// Configure sleep mode.
	set_sleep_mode(SLEEP_MODE_PWR_SAVE);

	// Configure IO pins.
	// PB0 = switch
	// PB1 = positive
	// PB2 = negative
	PORTB = _BV(1);
	DDRB = _BV(1) | _BV(2);

	// Enable interrupts.
	sei();

	// Go to sleep forever. The interrupt handlers will choose the appropriate type of sleep.
	for (;;) {
		// Might need a delay to allow one asynchronous tick to pass before sleeping again.
		_delay_us(50);
		sleep_mode();
	}
}

