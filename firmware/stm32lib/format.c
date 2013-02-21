#include <format.h>

void formathex4(char *dest, uint8_t val) {
	static const char DIGITS[16] = "0123456789ABCDEF";
	*dest = DIGITS[val & 0x0F];
}

void formathex8(char *dest, uint8_t val) {
	formathex4(dest, val >> 4);
	formathex4(dest + 1, val);
}

void formathex16(char *dest, uint16_t val) {
	formathex8(dest, val >> 8);
	formathex8(dest + 2, val);
}

void formathex32(char *dest, uint32_t val) {
	formathex16(dest, val >> 16);
	formathex16(dest + 4, val);
}

