#include "exti.h"
#include <assert.h>
#include <rcc.h>
#include <registers/exti.h>
#include <registers/syscfg.h>

static exti_handler_t handlers[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

void exti0_isr(void) {
	handlers[0]();
}

void exti1_isr(void) {
	handlers[1]();
}

void exti2_isr(void) {
	handlers[2]();
}

void exti3_isr(void) {
	handlers[3]();
}

void exti4_isr(void) {
	handlers[4]();
}

void exti5_9_isr(void) {
	for (unsigned int i = 5; i <= 9; ++i) {
		if (EXTI_PR & (1 << i)) {
			handlers[i]();
			break;
		}
	}
}

void exti10_15_isr(void) {
	for (unsigned int i = 10; i <= 15; ++i) {
		if (EXTI_PR & (1 << i)) {
			handlers[i]();
			break;
		}
	}
}

void exti_map(unsigned int line, unsigned int port) {
	assert(line <= 15);
	assert(port <= 8);
	unsigned int cr = line / 4;
	unsigned int shift = (line % 4) * 4;
	rcc_enable(APB2, SYSCFG);
	SYSCFG_EXTICR[cr] = (SYSCFG_EXTICR[cr] & ~(0xF << shift)) | (port << shift);
	rcc_disable(APB2, SYSCFG);
}

void exti_set_handler(unsigned int line, exti_handler_t handler) {
	assert(line <= 15);
	handlers[line] = handler;
}

