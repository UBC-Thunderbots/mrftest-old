static inline unsigned char read(unsigned char p) {
	unsigned char x;
	asm volatile("in %0, %1" : "=r"(x) : "i"(p));
	return x;
}

static inline void write(unsigned char p, unsigned char x) {
	asm volatile("out %0, %1" : : "i"(p), "r"(x));
}

static void avr_main(void) __attribute__((noreturn, section(".entry"), used));
static void avr_main(void) {
	for (;;) {
		read(0);
		write(0, read(3) >> 2);
	}
}

