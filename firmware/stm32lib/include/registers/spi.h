#ifndef STM32LIB_REGISTERS_SPI_H
#define STM32LIB_REGISTERS_SPI_H

/**
 * \file
 *
 * \brief Defines the serial peripheral interface registers.
 */

#define SPI1_BASE 0x40013000
#define SPI2_BASE 0x40003800
#define SPI3_BASE 0x40003C00

typedef struct {
	unsigned CPHA : 1;
	unsigned CPOL : 1;
	unsigned MSTR : 1;
	unsigned BR : 3;
	unsigned SPE : 1;
	unsigned LSBFIRST : 1;
	unsigned SSI : 1;
	unsigned SSM : 1;
	unsigned RXONLY : 1;
	unsigned DFF : 1;
	unsigned CRCNEXT : 1;
	unsigned CRCEN : 1;
	unsigned BIDIOE : 1;
	unsigned BIDIMODE : 1;
	unsigned : 16;
} SPI_CR1_t;

typedef struct {
	unsigned RXDMAEN : 1;
	unsigned TXDMAEN : 1;
	unsigned SSOE : 1;
	unsigned : 1;
	unsigned FRF : 1;
	unsigned ERRIE : 1;
	unsigned RXNEIE : 1;
	unsigned TXEIE : 1;
	unsigned : 24;
} SPI_CR2_t;

typedef struct {
	unsigned RXNE : 1;
	unsigned TXE : 1;
	unsigned CHSIDE : 1;
	unsigned UDR : 1;
	unsigned CRCERR : 1;
	unsigned MODF : 1;
	unsigned OVR : 1;
	unsigned BSY : 1;
	unsigned FRE : 1;
	unsigned : 23;
} SPI_SR_t;

typedef struct {
	unsigned CHLEN : 1;
	unsigned DATLEN : 2;
	unsigned CKPOL : 1;
	unsigned I2SSTD : 2;
	unsigned : 1;
	unsigned PCMSYNC : 1;
	unsigned I2SCFG : 2;
	unsigned I2SE : 1;
	unsigned I2SMOD : 1;
	unsigned : 20;
} SPI_I2SCFGR_t;

typedef struct {
	unsigned I2SDIV : 8;
	unsigned ODD : 1;
	unsigned MCKOE : 1;
	unsigned : 22;
} SPI_I2SPR_t;

typedef struct {
	SPI_CR1_t CR1;
	SPI_CR2_t CR2;
	SPI_SR_t SR;
	uint32_t DR;
	uint32_t CRCPR;
	uint32_t RXCRCR;
	uint32_t TXCRCR;
	SPI_I2SCFGR_t I2SCFGR;
	SPI_I2SPR_t I2SPR;
} SPI_t;

#define SPI1 (*(volatile SPI_t *) SPI1_BASE)
#define SPI2 (*(volatile SPI_t *) SPI2_BASE)
#define SPI3 (*(volatile SPI_t *) SPI3_BASE)

#endif

