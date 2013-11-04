#ifndef STM32LIB_REGISTERS_FLASH_H
#define STM32LIB_REGISTERS_FLASH_H

/**
 * \file
 *
 * \brief Defines the embedded Flash memory interface registers.
 */

#include <stdint.h>

#define FLASH_BASE 0x40023C00

typedef struct {
	unsigned LATENCY : 3;
	unsigned : 5;
	unsigned PRFTEN : 1;
	unsigned ICEN : 1;
	unsigned DCEN : 1;
	unsigned ICRST : 1;
	unsigned DCRST : 1;
	unsigned : 19;
} FLASH_ACR_t;
#define FLASH_ACR (*(volatile FLASH_ACR_t *) (FLASH_BASE + 0x00))

#define FLASH_KEYR (*(volatile uint32_t *) (FLASH_BASE + 0x04))
#define FLASH_OPTKEYR (*(volatile uint32_t *) (FLASH_BASE + 0x08))

typedef struct {
	unsigned EOP : 1;
	unsigned OPERR : 1;
	unsigned : 2;
	unsigned WRPERR : 1;
	unsigned PGAERR : 1;
	unsigned PGPERR : 1;
	unsigned PGSERR : 1;
	unsigned : 8;
	unsigned BSY : 1;
	unsigned : 15;
} FLASH_SR_t;
#define FLASH_SR (*(volatile FLASH_SR_t *) (FLASH_BASE + 0x0C))

typedef struct {
	unsigned PG : 1;
	unsigned SER : 1;
	unsigned MER : 1;
	unsigned SNB : 5;
	unsigned PSIZE : 2;
	unsigned : 5;
	unsigned MER1 : 1;
	unsigned STRT : 1;
	unsigned : 7;
	unsigned EOPIE : 1;
	unsigned ERRIE : 1;
	unsigned : 5;
	unsigned LOCK : 1;
} FLASH_CR_t;
#define FLASH_CR (*(volatile FLASH_CR_t *) (FLASH_BASE + 0x10))

typedef struct {
	unsigned OPTLOCK : 1;
	unsigned OPTSTRT : 1;
	unsigned BOR_LEV : 2;
	unsigned : 1;
	unsigned WDG_SW : 1;
	unsigned nRST_STOP : 1;
	unsigned nRST_STDBY : 1;
	unsigned RDP : 8;
	unsigned nWRP : 12;
	unsigned : 4;
} FLASH_OPTCR_t;
#define FLASH_OPTCR (*(volatile FLASH_OPTCR_t *) (FLASH_BASE + 0x14))

typedef struct {
	unsigned : 16;
	unsigned nWRP : 12;
	unsigned : 4;
} FLASH_OPTCR1_t;
#define FLASH_OPTCR1 (*(volatile FLASH_OPTCR1_t *) (FLASH_BASE + 0x18))

#endif

