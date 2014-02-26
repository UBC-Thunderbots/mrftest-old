#ifndef STM32LIB_REGISTERS_SDIO_H
#define STM32LIB_REGISTERS_SDIO_H

/**
 * \file
 *
 * \brief Defines the secure digital I/O interface registers.
 */

#include <stdint.h>

#define SDIO_BASE 0x40012C00

typedef struct {
	unsigned PWRCTRL : 2;
	unsigned : 30;
} SDIO_POWER_t;
_Static_assert(sizeof(SDIO_POWER_t) == 4U, "SDIO_POWER_t is wrong size");
#define SDIO_POWER (*(volatile SDIO_POWER_t *) (SDIO_BASE + 0x00))

typedef struct {
	unsigned CLKDIV : 8;
	unsigned CLKEN : 1;
	unsigned PWRSAV : 1;
	unsigned BYPASS : 1;
	unsigned WIDBUS : 2;
	unsigned NEGEDGE : 1;
	unsigned HWFC_EN : 1;
	unsigned : 17;
} SDIO_CLKCR_t;
_Static_assert(sizeof(SDIO_CLKCR_t) == 4U, "SDIO_CLKCR_t is wrong size");
#define SDIO_CLKCR (*(volatile SDIO_CLKCR_t *) (SDIO_BASE + 0x04))

#define SDIO_ARG (*(volatile uint32_t *) (SDIO_BASE + 0x08))

typedef struct {
	unsigned CMDINDEX : 6;
	unsigned WAITRESP : 2;
	unsigned WAITINT : 1;
	unsigned WAITPEND : 1;
	unsigned CPSMEN : 1;
	unsigned SDIOSuspend : 1;
	unsigned ENCMDcompl : 1;
	unsigned nIEN : 1;
	unsigned ATACMD : 1;
	unsigned : 17;
} SDIO_CMD_t;
_Static_assert(sizeof(SDIO_CMD_t) == 4U, "SDIO_CMD_t is wrong size");
#define SDIO_CMD (*(volatile SDIO_CMD_t *) (SDIO_BASE + 0x0C))

#define SDIO_RESPCMD (*(const volatile uint32_t *) (SDIO_BASE + 0x10))

#define SDIO_RESP ((const volatile uint32_t *) (SDIO_BASE + 0x14))

#define SDIO_DTIMER (*(volatile uint32_t *) (SDIO_BASE + 0x24))

#define SDIO_DLEN (*(volatile uint32_t *) (SDIO_BASE + 0x28))

typedef struct {
	unsigned DTEN : 1;
	unsigned DTDIR : 1;
	unsigned DTMODE: 1;
	unsigned DMAEN : 1;
	unsigned DBLOCKSIZE : 4;
	unsigned RWSTART : 1;
	unsigned RWSTOP : 1;
	unsigned RWMOD : 1;
	unsigned SDIOEN : 1;
	unsigned : 20;
} SDIO_DCTRL_t;
_Static_assert(sizeof(SDIO_DCTRL_t) == 4U, "SDIO_DCTRL_t is wrong size");
#define SDIO_DCTRL (*(volatile SDIO_DCTRL_t *) (SDIO_BASE + 0x2C))

#define SDIO_DCOUNT (*(const volatile uint32_t *) (SDIO_BASE + 0x30))

typedef struct {
	unsigned CCRCFAIL : 1;
	unsigned DCRCFAIL : 1;
	unsigned CTIMEOUT : 1;
	unsigned DTIMEOUT : 1;
	unsigned TXUNDERR : 1;
	unsigned RXOVERR : 1;
	unsigned CMDREND : 1;
	unsigned CMDSENT : 1;
	unsigned DATAEND : 1;
	unsigned STBITERR : 1;
	unsigned DBCKEND : 1;
	unsigned CMDACT : 1;
	unsigned TXACT : 1;
	unsigned RXACT : 1;
	unsigned TXFIFOHE : 1;
	unsigned RXFIFOHF : 1;
	unsigned TXFIFOF : 1;
	unsigned RXFIFOF : 1;
	unsigned TXFIFOE : 1;
	unsigned RXFIFOE : 1;
	unsigned TXDAVL : 1;
	unsigned RXDAVL : 1;
	unsigned SDIOIT : 1;
	unsigned CEATAEND : 1;
	unsigned : 8;
} SDIO_STA_t;
_Static_assert(sizeof(SDIO_STA_t) == 4U, "SDIO_STA_t is wrong size");
#define SDIO_STA (*(const volatile SDIO_STA_t *) (SDIO_BASE + 0x34))

typedef struct {
	unsigned CCRCFAILC : 1;
	unsigned DCRCFAILC : 1;
	unsigned CTIMEOUTC : 1;
	unsigned DTIMEOUTC : 1;
	unsigned TXUNDERRC : 1;
	unsigned RXOVERRC : 1;
	unsigned CMDRENDC : 1;
	unsigned CMDSENTC : 1;
	unsigned DATAENDC : 1;
	unsigned STBITERRC : 1;
	unsigned DBCKENDC : 1;
	unsigned : 11;
	unsigned SDIOITC : 1;
	unsigned CEATAENDC : 1;
	unsigned : 8;
} SDIO_ICR_t;
_Static_assert(sizeof(SDIO_ICR_t) == 4U, "SDIO_ICR_t is wrong size");
#define SDIO_ICR (*(volatile SDIO_ICR_t *) (SDIO_BASE + 0x38))

typedef struct {
	unsigned CCRCFAILIE : 1;
	unsigned DCRCFAILIE : 1;
	unsigned CTIMEOUTIE : 1;
	unsigned DTIMEOUTIE : 1;
	unsigned TXUNDERRIE : 1;
	unsigned RXOVERRIE : 1;
	unsigned CMDRENDIE : 1;
	unsigned CMDSENTIE : 1;
	unsigned DATAENDIE : 1;
	unsigned STBITERRIE : 1;
	unsigned DBCKENDIE : 1;
	unsigned CMDACTIE : 1;
	unsigned TXACTIE : 1;
	unsigned RXACTIE : 1;
	unsigned TXFIFOHEIE : 1;
	unsigned RXFIFOHFIE : 1;
	unsigned TXFIFOFIE : 1;
	unsigned RXFIFOFIE : 1;
	unsigned TXFIFOEIE : 1;
	unsigned RXFIFOEIE : 1;
	unsigned TXDAVLIE : 1;
	unsigned RXDAVLIE : 1;
	unsigned SDIOITIE : 1;
	unsigned CEATAENDIE : 1;
	unsigned : 8;
} SDIO_MASK_t;
_Static_assert(sizeof(SDIO_MASK_t) == 4U, "SDIO_MASK_t is wrong size");
#define SDIO_MASK (*(volatile SDIO_MASK_t *) (SDIO_BASE + 0x3C))

#define SDIO_FIFOCNT (*(const volatile uint32_t *) (SDIO_BASE + 0x48))

#define SDIO_FIFO (*(volatile uint32_t *) (SDIO_BASE + 0x80))

#endif

