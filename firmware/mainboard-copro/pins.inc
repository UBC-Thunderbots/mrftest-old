	; asmsyntax=pic

	; pins.inc
	; ========
	;
	; The compile-time definitions in this file define which pin is used for
	; which purpose.
	;



	; Which port, tris, and pin is connected to the FPGA's "DONE" pin.
PORT_DONE equ PORTD
TRIS_DONE equ TRISD
LAT_DONE  equ LATD
PIN_DONE  equ 4

	; Which port, tris, and pin is connected to the FPGA's "PROG_B" pin.
PORT_PROG_B equ PORTD
TRIS_PROG_B equ TRISD
LAT_PROG_B  equ LATD
PIN_PROG_B  equ 2

	; Which port, tris, and pin is connected to the FPGA's "INIT_B" pin.
PORT_INIT_B equ PORTD
TRIS_INIT_B equ TRISD
LAT_INIT_B  equ LATD
PIN_INIT_B  equ 5

	; Which port, tris, and pin is wired-ORed to the motor controllers' BRAKE lines.
PORT_BRAKE equ PORTC
TRIS_BRAKE equ TRISC
LAT_BRAKE  equ LATC
PIN_BRAKE  equ 1

	; Which port, tris, and pin is the SPI transmit line.
PORT_SPI_TX equ PORTD
TRIS_SPI_TX equ TRISD
LAT_SPI_TX  equ LATD
PIN_SPI_TX  equ 1

	; Which port, tris, and pin is the SPI receive line.
PORT_SPI_RX equ PORTD
TRIS_SPI_RX equ TRISD
LAT_SPI_RX  equ LATD
PIN_SPI_RX  equ 0

	; Which port, tris, and pin is the SPI clock line.
PORT_SPI_CK equ PORTC
TRIS_SPI_CK equ TRISC
LAT_SPI_CK  equ LATC
PIN_SPI_CK  equ 0

	; Which port, tris, and pin is the SPI slave select line to the Flash.
PORT_SPI_SS_FLASH equ PORTD
TRIS_SPI_SS_FLASH equ TRISD
LAT_SPI_SS_FLASH  equ LATD
PIN_SPI_SS_FLASH  equ 6

	; Which port, tris, and pin is the SPI slave select line to the FPGA.
PORT_SPI_SS_FPGA equ PORTD
TRIS_SPI_SS_FPGA equ TRISD
LAT_SPI_SS_FPGA  equ LATD
PIN_SPI_SS_FPGA  equ 3

	; Which port, tris, and pin is the Flash WRITE PROTECT line.
PORT_FLASH_WP equ PORTA
TRIS_FLASH_WP equ TRISA
LAT_FLASH_WP  equ LATA
PIN_FLASH_WP  equ 4

	; Which port, tris, and pin is the transmit line to the XBee.
PORT_XBEE_TX equ PORTC
TRIS_XBEE_TX equ TRISC
LAT_XBEE_TX  equ LATC
PIN_XBEE_TX  equ 6

	; Which port, tris, and pin is the receive line from the XBee.
PORT_XBEE_RX equ PORTC
TRIS_XBEE_RX equ TRISC
LAT_XBEE_RX  equ LATC
PIN_XBEE_RX  equ 7

	; Which port, tris, and pin is the bootload signal line from the XBee.
PORT_XBEE_BL equ PORTD
TRIS_XBEE_BL equ TRISD
LAT_XBEE_BL  equ LATD
PIN_XBEE_BL  equ 7

	; Which port, tris, and pin is the PGD line for ICSP.
PORT_ICSP_PGD equ PORTB
TRIS_ICSP_PGD equ TRISB
LAT_ICSP_PGD  equ LATB
PIN_ICSP_PGD  equ 7

	; Which port, tris, and pin is the PGC line for ICSP.
PORT_ICSP_PGC equ PORTB
TRIS_ICSP_PGC equ TRISB
LAT_ICSP_PGC  equ LATB
PIN_ICSP_PGC  equ 6

	; Which port, tris, and pin is the PGM line for ICSP.
PORT_ICSP_PGM equ PORTB
TRIS_ICSP_PGM equ TRISB
LAT_ICSP_PGM  equ LATB
PIN_ICSP_PGM  equ 5

	; Which port, tris, and pin is the D+ line for USB.
PORT_USB_DP equ PORTC
TRIS_USB_DP equ TRISC
LAT_USB_DP  equ LATC
PIN_USB_DP  equ 5

	; Which port, tris, and pin is the D- line for USB.
PORT_USB_DM equ PORTC
TRIS_USB_DM equ TRISC
LAT_USB_DM  equ LATC
PIN_USB_DM  equ 4

	; Which port, tris, and pin is the RTS line.
PORT_RTS equ PORTC
TRIS_RTS equ TRISC
LAT_RTS  equ LATC
PIN_RTS  equ 2
