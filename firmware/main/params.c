#include "params.h"
#include "crc.h"
#include "pins.h"
#include "spi.h"
#include "xbee_rxpacket.h"
#include <pic18fregs.h>
#include <string.h>

#define FLASH_SIZE (2UL * 1024UL * 1024UL)
#define SECTOR_SIZE 4096UL
#define PARAMS_BLOCK_FLASH_ADDRESS (FLASH_SIZE - SECTOR_SIZE)

params_t params;

__code static params_t __at(0x1F000) rom_params = { FLASH_CONTENTS_NONE, { 0x0E, 0x0F }, 15, 80, 0 };
__code static uint16_t __at(0x1F000 + sizeof(rom_params)) rom_params_crc = 0x4201;

BOOL params_load(void) {
	memcpypgm2ram(&params, &rom_params, sizeof(params));
	return crc_update_block(CRC16_EMPTY, &params, sizeof(params)) == rom_params_crc;
}

void params_commit(void) {
	static uint16_t crc;
	static const uint8_t params_len = sizeof(params);

	/* Suspend inbound communication so data will not be lost. */
	xbee_rxpacket_suspend();

	/* Do not take interrupts during this time. */
	INTCONbits.GIEH = 0;

	/* Compute a CRC of the current parameter block. */
	crc = crc_update_block(CRC16_EMPTY, &params, sizeof(params));

	__asm
	; Erase the region.
	movlw LOW(_rom_params)
	movwf _TBLPTRL
	movlw HIGH(_rom_params)
	movwf _TBLPTRH
	movlw UPPER(_rom_params)
	movwf _TBLPTRU
	movlw 0x14 ; FREE | WREN
	movwf _EECON1
	movlw 0x55
	movwf _EECON2
	movlw 0xAA
	movwf _EECON2
	bsf _EECON1, 1 ; WR

	; Copy the parameters into the holding registers.
	lfsr 0, _params
	banksel _params_commit_params_len_1_1
	movf _params_commit_params_len_1_1, W
params_commit_copy_to_holding_regs:
	movff _POSTINC0, _TABLAT
	tblwt *+
	addlw -1
	bnz params_commit_copy_to_holding_regs

	; Copy the CRC into the holding registers.
	movff _params_commit_crc_1_1, TABLAT
	tblwt *+
	movff _params_commit_crc_1_1 + 1, TABLAT
	tblwt *

	; Execute the write operation.
	movlw 0x04 ; WREN
	movwf _EECON1
	movlw 0x55
	movwf _EECON2
	movlw 0xAA
	movwf _EECON2
	bsf _EECON1, 1 ; WR

	; Lock out writes.
	clrf _EECON1
	__endasm;

	/* Now accept interrupts. */
	INTCONbits.GIEH = 1;

	/* Resume inbound communication. */
	xbee_rxpacket_resume();
}

