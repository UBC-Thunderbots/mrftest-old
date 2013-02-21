#include <exti.h>
#include <assert.h>
#include <registers.h>

static exti_handler_t handlers[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

void exti_dispatcher_0(void) {
	handlers[0]();
}

void exti_dispatcher_1(void) {
	handlers[1]();
}

void exti_dispatcher_2(void) {
	handlers[2]();
}

void exti_dispatcher_3(void) {
	handlers[3]();
}

void exti_dispatcher_4(void) {
	handlers[4]();
}

void exti_dispatcher_9_5(void) {
	for (unsigned int i = 5; i <= 9; ++i) {
		if (EXTI_PR & (1 << i)) {
			handlers[i]();
			break;
		}
	}
}

void exti_dispatcher_15_10(void) {
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
	SYSCFG_EXTICR[cr] = (SYSCFG_EXTICR[cr] & ~(0xF << shift)) | (port << shift);
}

void exti_set_handler(unsigned int line, exti_handler_t handler) {
	assert(line <= 15);
	handlers[line] = handler;
}

