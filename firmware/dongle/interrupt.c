#include "interrupt.h"
#include "registers.h"

void (*interrupt_exti12_handler)(void) = 0;

void interrupt_dispatcher_exti15_10(void) {
	if (EXTI_PR & (1 << 12)) {
		interrupt_exti12_handler();
	}
}

