#include "params.h"
#include "crc.h"
#include "pins.h"
#include "spi.h"
#include <pic18fregs.h>

#define FLASH_SIZE (2UL * 1024UL * 1024UL)
#define SECTOR_SIZE 4096UL
#define PARAMS_BLOCK_FLASH_ADDRESS (FLASH_SIZE - SECTOR_SIZE)

params_t params;

BOOL params_load(void) {
	uint8_t len = sizeof(params);
	__data uint8_t *ptr = (uint8_t *) &params;
	uint8_t ch;
	uint16_t crc = CRC16_EMPTY;

	/* Check the flash's JEDEC ID.
	 * It should be 0xEF, 0x40, 0x15. */
	LAT_FLASH_CS = 0;
	spi_send(0x9F);
	if (spi_receive() != 0xEF || spi_receive() != 0x40 || spi_receive() != 0x15) {
		LAT_FLASH_CS = 1;
		return false;
	}
	LAT_FLASH_CS = 1;

	/* Read the data and verify the checksum. */
	LAT_FLASH_CS = 0;
	spi_send(0x03);
	spi_send((PARAMS_BLOCK_FLASH_ADDRESS >> 16) & 0xFF);
	spi_send((PARAMS_BLOCK_FLASH_ADDRESS >> 8) & 0xFF);
	spi_send(PARAMS_BLOCK_FLASH_ADDRESS & 0xFF);
	while (len--) {
		ch = spi_receive();
		crc = crc_update(crc, ch);
		*ptr++ = ch;
	}
	if (spi_receive() != (crc & 0xFF) || spi_receive() != (crc >> 8)) {
		LAT_FLASH_CS = 1;
		return false;
	}
	LAT_FLASH_CS = 1;
	return true;
}

void params_commit(BOOL erase) {
	__data const uint8_t *ptr = (__data const uint8_t *) &params;
	uint8_t len = sizeof(params);
	uint16_t crc = CRC16_EMPTY;

	if (erase) {
		/* Set write enable latch. */
		LAT_FLASH_CS = 0;
		spi_send(0x06);
		LAT_FLASH_CS = 1;

		/* Send the sector erase instruction. */
		LAT_FLASH_CS = 0;
		spi_send(0x20);
		spi_send(PARAMS_BLOCK_FLASH_ADDRESS >> 16);
		spi_send(PARAMS_BLOCK_FLASH_ADDRESS >> 8);
		spi_send(PARAMS_BLOCK_FLASH_ADDRESS);
		LAT_FLASH_CS = 1;

		/* Wait until complete. */
		LAT_FLASH_CS = 0;
		spi_send(0x05);
		while (spi_receive() & 0x01);
		LAT_FLASH_CS = 1;
	}

	/* Set write enable latch. */
	LAT_FLASH_CS = 0;
	spi_send(0x06);
	LAT_FLASH_CS = 1;

	/* Send the page program instruction. */
	LAT_FLASH_CS = 0;
	spi_send(0x02);
	spi_send(PARAMS_BLOCK_FLASH_ADDRESS >> 16);
	spi_send(PARAMS_BLOCK_FLASH_ADDRESS >> 8);
	spi_send(PARAMS_BLOCK_FLASH_ADDRESS);
	while (len--) {
		crc = crc_update(crc, *ptr);
		spi_send(*ptr++);
	}
	spi_send(crc);
	spi_send(crc >> 8);
	LAT_FLASH_CS = 1;

	/* Wait until complete. */
	LAT_FLASH_CS = 0;
	spi_send(0x05);
	while (spi_receive() & 0x01);
	LAT_FLASH_CS = 1;
}

