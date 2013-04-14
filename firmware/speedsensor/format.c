#include "format.h"

void formatuint1(char *dest, uint8_t val){
	static const char DEC_DIGITS[10] = "0123456789";
	*dest = DEC_DIGITS[val%10];
	//*dest = val%10 + 48;
}

void formatuint2(char *dest, uint8_t val){
	formatuint1(dest, val/10);
	formatuint1(dest + 1, val);
}

void formatuint4(char *dest, uint16_t val){
	formatuint2(dest, val/100);
	formatuint2(dest + 2, val%100);
}

void formatuint8(char *dest, uint32_t val){
	formatuint4(dest, val/10000);
	formatuint4(dest + 4, val%10000);
}

void formatuint16(char *dest, uint32_t val){
	formatuint8(dest, val/100000000);
	formatuint8(dest + 8, val);
}

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

