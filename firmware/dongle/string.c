#include "string.h"

void *memcpy(void *dest, const void *src, size_t n) {
	unsigned char *pd = dest;
	const unsigned char *ps = src;
	while (n--) {
		*pd++ = *ps++;
	}
	return dest;
}

void *memset(void *s, int c, size_t n) {
	unsigned char *pdest = s;
	while (n--) {
		*pdest++ = c;
	}
	return s;
}

