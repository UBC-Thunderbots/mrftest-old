#ifndef STRING_H
#define STRING_H

#include "stddef.h"
#include "stdint.h"

void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
void formathex4(char *dest, uint8_t val);
void formathex8(char *dest, uint8_t val);
void formathex16(char *dest, uint16_t val);
void formathex32(char *dest, uint32_t val);

#endif
