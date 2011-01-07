#include "crc.h"
#include "leds.h"
#include "params.h"
#include "pins.h"
#include "spi.h"
#include <delay.h>
#include <pic18fregs.h>

#define FLASH_SIZE (2UL * 1024UL * 1024UL)
#define SECTOR_SIZE 4096UL
#define PARAMS_BLOCK_FLASH_ADDRESS (FLASH_SIZE - SECTOR_SIZE)

/**
 * \brief The parameters block to write.
 */
__code static const params_t PARAMS = {
	/* flash_contents */ FLASH_CONTENTS_NONE,
	/* xbee_channels */ { 0x0E, 0x0F },
	/* robot_number */ 15,
};

void main(void) {
	__code const uint8_t *ptr = (__code const uint8_t *) &PARAMS;
	uint8_t len = sizeof(PARAMS);
	uint16_t crc = CRC16_EMPTY;

	/* Configure I/O pins. */
	PINS_INITIALIZE();
	WDTCONbits.ADSHR = 1;
	ANCON0 = 0xF0;
	ANCON1 = 0xFF;
	WDTCONbits.ADSHR = 0;

	/* Disable timer 0 until needed. */
	T0CONbits.TMR0ON = 0;

	/* We want to attach timer 1 to ECCP 1 and timer 3 to ECCP2 so we can have separate periods. */
	T3CONbits.T3CCP2 = 0;
	T3CONbits.T3CCP1 = 1;

	/* Wait for the oscillator to start. */
	while (!OSCCONbits.OSTS);

	/* Enable the PLL and wait for it to lock. This may take up to 2ms. */
	OSCTUNEbits.PLLEN = 1;
	delay1ktcy(2);

	/* Enable the SPI transceiver. */
	spi_init();

	/* Read and check the JEDEC ID. */
	leds_show_number(1);
	LAT_FLASH_CS = 0;
	spi_send(0x9F);
	if (spi_receive() != 0xEF || spi_receive() != 0x40 || spi_receive() != 0x15) {
		LAT_FLASH_CS = 1;
		for (;;) {
			Sleep();
		}
	}
	LAT_FLASH_CS = 1;

	/* Write-enable the flash. */
	LAT_FLASH_CS = 0;
	spi_send(0x06);
	LAT_FLASH_CS = 1;

	/* Erase the chip. */
	leds_show_number(2);
	LAT_FLASH_CS = 0;
	spi_send(0xC7);
	LAT_FLASH_CS = 1;

	/* Wait until the erase is finished. */
	LAT_FLASH_CS = 0;
	spi_send(0x05);
	while (spi_receive() & 0x01);
	LAT_FLASH_CS = 1;

	/* Write-enable the flash. */
	LAT_FLASH_CS = 0;
	spi_send(0x06);
	LAT_FLASH_CS = 1;

	/* Write the data. */
	leds_show_number(3);
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

	/* Wait until the program is finished. */
	LAT_FLASH_CS = 0;
	spi_send(0x05);
	while (spi_receive() & 0x01);
	LAT_FLASH_CS = 1;

	/* Show completion. */
	for (;;) {
		LAT_LED1 = 1;
		LAT_LED2 = 1;
		LAT_LED3 = 1;
		LAT_LED4 = 1;
		delay1mtcy(4);
		LAT_LED1 = 0;
		LAT_LED2 = 0;
		LAT_LED3 = 0;
		LAT_LED4 = 0;
		delay1mtcy(4);
	}
}

