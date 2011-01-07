#include "spi.h"
#include "pins.h"
#include <pic18fregs.h>

void spi_init(void) {
	SSP1STATbits.CKE = 1;
	/*           /-------- No write collision
	 *           |/------- No overflow
	 *           ||/------ Module enabled
	 *           |||/----- Clock idles low
	 *           ||||////- SPI master mode, clock = Fosc/4 */
	SSP1CON1 = 0b00100000;
}

void spi_tristate(void) {
	TRIS_FLASH_CS = 1;
	TRIS_FLASH_CLK = 1;
	TRIS_FLASH_MOSI = 1;
}

void spi_drive(void) {
	TRIS_FLASH_CS = 0;
	TRIS_FLASH_CLK = 0;
	TRIS_FLASH_MOSI = 0;
}

void spi_send(uint8_t ch) __wparam {
	SSP1BUF = (ch);
	while (!PIR1bits.SSP1IF);
	PIR1bits.SSP1IF = 0;
	(void) SSP1BUF;
}

uint8_t spi_receive(void) {
	SSP1BUF = 0;
	while (!PIR1bits.SSP1IF);
	PIR1bits.SSP1IF = 0;
	return SSP1BUF;
}

