	; asmsyntax=pic

	; spi.inc
	; =======
	;
	; This file declares functions related to the SPI bus.
	;

	extern spi_drive
	extern spi_tristate
	extern spi_send    ; Transmission is at 400,000 bits per second.
	extern spi_receive ; Reception is at 1,000,000 bits per second.
