#include "sdcard.h"
#include "syscalls.h"
#include "io.h"
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>

typedef enum {
	GO_IDLE_STATE = 0,
	SEND_RELATIVE_ADDR = 3,
	SWITCH_FUNC = 6,
	SELECT_CARD = 7,
	SEND_IF_COND = 8,
	SEND_CSD = 9,
	SEND_CID = 10,
	STOP_TRANSMISSION = 12,
	SEND_STATUS = 13,
	SET_BLOCKLEN = 16,
	READ_SINGLE_BLOCK = 17,
	READ_MULTIPLE_BLOCK = 18,
	WRITE_BLOCK = 24,        
	WRITE_MULTIPLE_BLOCK = 25,
	PROGRAM_CSD = 27,
	SET_WRITE_PROT = 28,
	CLR_WRITE_PROT = 29,
	SEND_WRITE_PROT = 30,
	ERASE_WR_BLK_START_ADDR = 32,
	ERASE_WR_BLK_END_ADDR = 33,
	ERASE = 38,
	LOCK_UNLOCK = 42,
	APP_CMD = 55,
	GEN_CMD = 56,
} sd_cmd_t;

typedef enum {
	SET_BUS_WIDTH = 6,
	SD_STATUS = 13,
	SEND_NUM_WR_BLOCKS = 22,
	SET_WR_BLK_ERASE_COUNT = 23,
	SD_SEND_OP_COND = 41,
	SET_CLR_CARD_DETECT = 42,
	SEND_SCR = 51,
} sd_acmd_t;

#define STATE_IDLE 0
#define STATE_READY 1
#define STATE_IDENT 2
#define STATE_STBY 3
#define STATE_TRAN 4
#define STATE_DATA 5
#define STATE_RCV 6
#define STATE_PRG 7

#define DATA_TIMEOUT_TIME 65535
#define DATA_LENGTH 512
#define CLOCK 48000000

static SemaphoreHandle_t int_semaphore = NULL;

typedef struct {
	sd_status_t status;
	uint32_t sector_count;
	bool sdhc;
	bool write_multi_active;
} sd_ctx_t;

static sd_ctx_t card_state = {
	.status = SD_STATUS_UNINITIALIZED,
};

//first case: when data write is complete. 
void SDIO_ISR ( void ) {
	// clear the mask
	SDIO_MASK_t mask_temp = { 0 };
	SDIO_MASK = mask_temp;
	BaseType_t yield = pdFALSE;
	xSemaphoreGiveFromISR( int_semaphore, &yield );
	if (yield) {
		portYIELD_FROM_ISR();
	}
}

static bool card_present(void) {
	return IO_SD.csr.present;
}

float convert_TAAC ( uint8_t TAAC )
{
	uint8_t multiplier = TAAC & 0b00000111;
	uint8_t value = TAAC>>3;
	float TAAC_in_nanosecs;
	value &= 0b00001111;
	if ( value == 1)
		TAAC_in_nanosecs = 1.0;
	else if ( value == 2 )
		TAAC_in_nanosecs = 1.2;
	else if ( value == 3)
		TAAC_in_nanosecs = 1.3;
	else if ( value == 4 )
		TAAC_in_nanosecs = 1.5;
	else if ( value == 5 )
		TAAC_in_nanosecs = 2.0;
	else if ( value == 6 )
		TAAC_in_nanosecs = 2.5;
	else if ( value == 7 )
		TAAC_in_nanosecs = 3.0;
	else if ( value == 8 )
		TAAC_in_nanosecs = 3.5;
	else if ( value == 9 )
		TAAC_in_nanosecs = 4.0;
	else if ( value == 10 )
		TAAC_in_nanosecs = 4.5;
	else if ( value == 11 )
		TAAC_in_nanosecs = 5.0;
	else if ( value == 12 )
		TAAC_in_nanosecs = 5.5;
	else if ( value == 13 )
		TAAC_in_nanosecs = 6.0;
	else if ( value == 14 )
		TAAC_in_nanosecs = 7.0;
	else if ( value == 15 )
		TAAC_in_nanosecs = 8.0;
	else 
	{
		printf ("Error: time value = 0 (reserved)\n");
		TAAC_in_nanosecs = 0;
	}
	for ( int i = 0; i<(int)multiplier; i++ )
	{
		TAAC_in_nanosecs *= 10;
	}
	return TAAC_in_nanosecs;
}
/*
static bool read_data_token(void) 
{
	//1. check if the transmit FIFO is empty. Move on when it is empty (Semaphore)
	//2. Set up DMA, do DMA
	//3. Check if DMA is done (Semephore)
	//4. Function finished. 
	SDIO_MASK_t mask_temp = { .RXFIFOEIE = 1 };

	SDIO_MASK = mask_temp;
	
	xSemaphoreTake (int_semaphore, portMAX_DELAY);

	//insert DMA stuff here. 

	mask_temp = { .DATAENDIE = 1, .DCRCFAILIE = 1, .DTIMEOUTIE = 1 };
	SDIO_MASK = mask_temp;

	xSemaphoreTake(int_semaphore, portMAX_DELAY);

	if ( SDIO_STA.DTIMEOUT == 1 ) {
		printf ( "Data timeout" );
		return false;
	}
	else if ( SDIO_STA.DCRCFAIL == 1 ) {
		printff ( "Data CRC fail" );
		return false;
	}

	return true;
	
} */
/*	uint8_t token;
	do {
		token = nop8r();
	} while (token == LINE_IDLE_STATE);
	if (token == START_BLOCK_TOKEN) {
		return true;
	}
	{
		char buffer[48];
		siprintf(buffer, "SD: Illegal or data error token 0x%" PRIX8 "\n", token);
		syscall_debug_puts(buffer);
	}
	if (token & 0xF0) {
		card_state.status = SD_STATUS_ILLEGAL_RESPONSE;
	} else if (token & 0x08) {
		card_state.status = SD_STATUS_LOGICAL_ERROR;
	} else {
		card_state.status = SD_STATUS_CARD_INTERNAL_ERROR;
	}
	return false;

*/


static bool read_block (uint32_t * data_address)
{
	//1. check if the receive FIFO is empty. Move on when it is empty (Semaphore)
	//2. Set up DMA, do DMA
	//3. Check if DMA is done (Semephore)
	//4. Function finished. 

	//insert DMA stuff here. 

	mask_temp = { .RXFIFOEIE = 1, .DCRCFAILIE = 1, .DBCKENDIE = 1, .DTIMEOUTIE = 1 };

	SDIO_DCTRL_t dctrl_temp = { .DTEN = 1, .DTDIR = 1, .DTMODE = 0, .DMAEN = 1, .DBLOCKSIZE = 0x1001 };

	SDIO_MASK = mask_temp;

	SDIO_DCTRL = dctrl_temp;

	xSemaphoreTake(int_semaphore, portMAX_DELAY);

	if ( SDIO_STA.DTIMEOUT == 1 ) {
		printf ( "Data timeout" );
		return false;
	}
	else if ( SDIO_STA.DCRCFAIL == 1 ) {
		printf ( "Data CRC fail" );
		return false;
	}
	else if ( SDIO_STA.DBCKEND == 1 ) {
		printf ( "Start bit error" );
		return false;
	}

	return true;
}

//skeleton for writing. No DMA stuff yet. 
static bool write_block (uint32_t * data_address)
{
	//1. check if the transmit FIFO is empty. Move on when it is empty (Semaphore)
	//2. Set up DMA, do DMA
	//3. Check if DMA is done (Semephore)
	//4. Function finished. 


/*	while ( SDIO_STA.TXFIFOE == 1 );
	SDIO_MASK_t mask_temp = { .TXFIFOEIE = 1 };

	SDIO_MASK = mask_temp;

	xSemaphoreTake (int_semaphore, portMAX_DELAY);
*/
	//insert DMA stuff here. 

	SDIO_DCTRL_t dctrl_temp = { .DTEN = 1, .DTDIR = 0, .DTMODE = 1, .DMAEN = 1, .DBLOCKSIZE = 0x1001 };

	mask_temp = { .DATAENDIE = 1, .DCRCFAILIE = 1, .DTIMEOUTIE = 1 };

	SDIO_DCTRL = dctrl_temp;

	SDIO_MASK = mask_temp;

	xSemaphoreTake(int_semaphore, portMAX_DELAY);

	if ( SDIO_STA.DTIMEOUT == 1 ) {
		printf ( "Data timeout" );
		return false;
	}
	else if ( SDIO_STA.DCRCFAIL == 1 ) {
		printff ( "Data CRC fail" );
		return false;
	}

	return true;
}


//send command in response 1 - no close
static bool send_command_r1(uint8_t command, uint32_t argument, uint8_t state_expected) {
	//clr possible error flags left from last iteration
	//reset CE-ATA: not useful
	//leave everything that doesn't have to do with command alone
	//clear everything that has to do with commands
	SDIO_ICR_t temp = { .CMDSENTC = 1, .CMDRENDC = 1, .CTIMEOUTC = 1, .CCRCFAILC = 1 }; 
	SDIO_ICR = temp;

	// enable command path state machine
	// disabled everything that has to do with CE-ATA and SDIO
	// command completion signal is disabled
	// CPSM doesn't wait for ends of data transfer before sending command
	SDIO_CMD_t tmp = { .CPSMEN = 1, .ATACMD = 0, .nIEN=1, .ENCMDcompl = 0, .SDIOSuspend = 0, .WAITPEND = 0, .WAITINT = 0, .WAITRESP = 1, .CMDINDEX = command };

	SDIO_MASK_t mask_temp = { .CTIMEOUTIE = 1, .CMDRENDIE = 1, .CCRCFAILIE = 1 };
	SDIO_MASK = mask_temp;
	// command completion signal is enabled
	SDIO_ARG = argument;
	SDIO_CMD = tmp;

	xSemaphoreTake ( int_semaphore, portMAX_DELAY );

	//grab the card response gained by sending command	
	uint32_t r1 = SDIO_RESP [0];

	//command received + CRC check passed. Command sent successfully
	if (SDIO_STA.COMDREND == 1) {
		uint8_t current_state = (r1 >> 9)&0x0F;
		r1 &= 0xFFF9E008;//zero everything that doesn't need to be checked. 
		uint8_t response_cmd = SDIO_RESPCMD;
		if ( response_cmd != command ) {
			printf ( "Sent command is not the same as command in response.\n" );
			printf( "Sent command %" PRIu8 "\n", command );
			printf( "Response command %" PRIu8 "\n", response_cmd );
			card_state.status = SD_STATUS_ILLEGAL_RESPONSE;
			return false;
		}
		if ( current_state != state_expected ) {
			printf ( "Incorrect starting state. \n" );
			card_state.status = SD_STATUS_ILLEGAL_STATE;
			return false;
		} 
		
		if (!r1) {	//no error occured on card's side. 
			return true;
		}
		else if ( r1 >> 31 & 0x01 ) {
			card_state.status = SD_STATUS_OUT_OF_RANGE;
		} else if ( r1 >> 30 & 0x01 ) {
			card_state.status = SD_STATUS_ADDRESS_ERROR;
		} else if ( r1 >> 29 & 0x01 )  {
			card_state.status = SD_STATUS_BLOCK_LEN_ERROR;
		} else if ( r1 >> 28 & 0x01 ) {
			card_state.status = SD_STATUS_ERASE_SEQ_ERROR;
		} else if ( r1 >> 27 & 0x01 )  {
			card_state.status = SD_STATUS_ERASE_PARAM;
		} else if ( r1 >> 26 & 0x01 )  {
			card_state.status = SD_STATUS_WP_VIOLATION;
		} else if ( r1 >> 25 & 0x01 )  {
			card_state.status = SD_STATUS_CARD_IS_LOCKED;
		} else if ( r1 >> 24 & 0x01 )  {
			card_state.status = SD_STATUS_LOCK_UNLOCK_FAILED;
		} else if ( r1 >> 23 & 0x01 )  {
			card_state.status = SD_STATUS_COM_CRC_ERROR;
		} else if ( r1 >> 22 & 0x01 )  {
			card_state.status = SD_STATUS_ILLEGAL_COMMAND;
		} else if ( r1 >> 21 & 0x01 )  {
			card_state.status = SD_STATUS_CARD_ECC_FAILED;
		} else if ( r1 >> 20 & 0x01 )  {
			card_state.status = SD_STATUS_CC_ERROR;
		} else if ( r1 >> 19 & 0x01 )  {
			card_state.status = SD_STATUS_ERROR;
		} else if ( r1 >> 16 & 0x01 )  {
			card_state.status = SD_STATUS_CSD_OVERWRITE;
		} else if ( r1 >> 15 & 0x01 )  {
			card_state.status = SD_STATUS_WP_ERASE_SKIP;
		} else if ( r1 >> 14 & 0x01 )  {
			card_state.status = SD_STATUS_CARD_ECC_DISABLED;
		} else if ( r1 >> 13 & 0x01 )  {
			card_state.status = SD_STATUS_ERASE_RESET;
		} else if ( r1 >> 3 & 0x01 )  {
			card_state.status = SD_STATUS_AKE_SEQ_ERROR;
		}
	}

	printf( "SD: Command %" PRIu8 "failed\n", command );

	if ( SDIO_STA.CCRCFAIL == 1 ) {
		card_state.status = SD_STATUS_CRC_ERROR;
		printf ( "SD: CRC_ERROR" );
	} else if ( SDIO_STA.CTIMEOUT == 1 ) {
		card_state.status = SD_STATUS_COMMAND_RESPONSE_TIMEOUT;
		printf ( "SD: COMMAND RESPONSE TIMEOUT." );
	}
	return false;
}



//r2, r3, r7
static bool send_command_general (uint8_t command, uint32_t argument) {
	//clr possible error flags left from last iteration
	//reset CE-ATA: not useful
	//leave everything that doesn't have to do with command alone
	//clear everything that has to do with commands
	SDIO_ICR_t temp = { .CMDSENTC = 1, .CMDRENDC = 1, .CTIMEOUTC = 1, .CCRCFAILC = 1 }; 
	SDIO_ICR = temp;

	// enable command path state machine
	// disabled everything that has to do with CE-ATA and SDIO
	// command completion signal is disabled
	// CPSM doesn't wait for ends of data transfer before sending command 
	SDIO_CMD_t tmp = { .CPSMEN = 1, .ATACMD = 0, .nIEN=1, .ENCMDcompl = 0, .SDIOSuspend = 0, .WAITPEND = 0, .WAITINT = 0, .WAITRESP = 1, .CMDINDEX = command };
	SDIO_MASK_t mask_temp = { .CTIMEOUTIE = 1, .CMDRENDIE = 1, .CCRCFAILIE = 1 };
	SDIO_MASK = mask_temp;
	// command completion signal is enabled
	SDIO_ARG = argument;
	SDIO_CMD = tmp; 
	
	xSemaphoreTake ( int_semaphore, portMAX_DELAY );

	//command received + CRC check passed. Command sent successfully
	if (SDIO_STA.COMDREND == 1) {
		uint8_t response_cmd = SDIO_RESPCMD;
		if ( response_cmd != command ) {
			printf ( "Sent command is not the same as command in response.\n" );
			printf( "Sent command %" PRIu8 "\n", command );
			printf( "Response command %" PRIu8 "\n", response_cmd );
			card_state.status = SD_STATUS_ILLEGAL_RESPONSE;
			return false;
		}

		return true;
	}
	
	printf( "SD: Command %" PRIu8 "failed\n", command );

	if ( SDIO_STA.CCRCFAIL == 1 ) {
		card_state.status = SD_STATUS_CRC_ERROR;
		printf ( "SD: CRC_ERROR" );
	} else if ( SDIO_STA.CTIMEOUT == 1 ) {
		card_state.status = SD_COMMAND_RESPONSE_TIMEOUT;
		printf ( "SD: COMMAND RESPONSE TIMEOUT." );
	}
}

static bool send_command_r6 (uint8_t command, uint32_t argument,  uint16_t * RCA, uint8_t expected_state)
{
	//clr possible error flags left from last iteration
	//reset CE-ATA: not useful
	//leave everything that doesn't have to do with command alone
	//clear everything that has to do with commands
	SDIO_ICR_t temp = { .CMDSENTC = 1, .CMDRENDC = 1, .CTIMEOUTC = 1, .CCRCFAILC = 1 }; 
	SDIO_ICR = temp;

	// enable command path state machine
	// disabled everything that has to do with CE-ATA and SDIO
	// command completion signal is disabled
	// CPSM doesn't wait for ends of data transfer before sending command 
	SDIO_CMD_t tmp = { .CPSMEN = 1, .ATACMD = 0, .nIEN=1, .ENCMDcompl = 0, .SDIOSuspend = 0, .WAITPEND = 0, .WAITINT = 0, .WAITRESP = 1, .CMDINDEX = command };
	SDIO_MASK_t mask_temp = { .CTIMEOUTIE = 1, .CMDRENDIE = 1, .CCRCFAILIE = 1 };
	SDIO_MASK = mask_temp;

	// command completion signal is enabled
	SDIO_ARG = argument;
	SDIO_CMD = tmp; 

	xSemaphoreTake (int_semaphore, portMAX_DELAY);

/*	// wait until command transfer is finished
	while (SDIO_STA.CMDACT == 1);
	// wait until command response is received or command response timeout
	while ( SDIO_STA.CMDREND == 0 && SDIO_STA.CCRCFAIL == 0 && 
			SDIO_STA.CTIMEOUT == 0 );
	*/
	//grab the card response  gained by sending command	
	uint32_t r6 = SDIO_RESP [0];
	RCA = r6 >> 16;

	//command received + CRC check passed. Command sent successfully
	if (SDIO_STA.COMDREND == 1) {
		uint8_t current_state = (r6 >> 9)&0x0F;
		r6 &= 0x0000FE08;//zero everything that doesn't need to be checked. 
		uint8_t response_cmd = SDIO_RESPCMD;
		if ( response_cmd != command ) {
			printf ( "Sent command is not the same as command in response.\n" );
			printf( "Sent command %" PRIu8 "\n", command );
			printf( "Response command %" PRIu8 "\n", response_cmd );
			card_state.status = SD_STATUS_ILLEGAL_RESPONSE;
			return false;
		}
		if ( current_state != state_expected ) {
			printf ( "Incorrect starting state. \n" );
			card_state.status = SD_STATUS_ILLEGAL_STATE;
			return false;
		}

		if ( !r6 ) {
			return true;  
		} else if ( r6 >> 15 & 0x01 ) {
			card_state.status = SD_COM_CRC_ERROR;
		} else if ( r6 >> 14 & 0x01 ) {
			card_state.status = SD_ILLEGAL_COMMAND;
		} else if ( r6 >> 13 & 0x01 ) {
			card_state.status = SD_ERROR;
		} else if ( r6 >> 3 & 0x01 ) {
			card_state.status = SD_AKE_SEQ_ERROR;
		}
	}
	
	printf( "SD: Command %" PRIu8 "failed\n", command );

	if ( SDIO_STA.CCRCFAIL == 1 ) {
		card_state.status = SD_STATUS_CRC_ERROR;
		printf ( "SD: CRC_ERROR" );
	} else if ( SDIO_STA.CTIMEOUT == 1 ) {
		card_state.status = SD_COMMAND_RESPONSE_TIMEOUT;
		printf ( "SD: COMMAND RESPONSE TIMEOUT." );
	}
}

/*
static bool send_command_r1_data_read(uint8_t command, uint32_t argument, void *buffer, size_t length) {
	if (!send_command_r1_nc(command, argument)) {
		close_transaction();
		return false;
	}
	if (!read_data_token()) {
		close_transaction();
		return false;
	}
	uint8_t *ptr = buffer;
	size_t bytes_left = length;
	do {
		*ptr++ = nop8r();
	} while (--bytes_left);
	uint16_t received_crc = 0;
	received_crc |= nop8r();
	received_crc <<= 8;
	received_crc |= nop8r();
	close_transaction();
	uint16_t calculated_crc = crc16(buffer, length);
	if (received_crc != calculated_crc) {
		char buffer[48];
		siprintf(buffer, "SD: RX data CRC error (calc %" PRIX16 ", RX %" PRIX16 ")\n", calculated_crc, received_crc);
		syscall_debug_puts(buffer);
		card_state.status = SD_STATUS_CRC_ERROR;
		return false;
	}
	return true;
}
*/


sd_status_t sd_status(void) {
	return card_state.status;
}

bool sd_init(void) {

	card_state.status = SD_STATUS_UNINITIALIZED;

	// Check for card.
	if (!card_present()) {
		printf( "SD: No card\n" );
		card_state.status = SD_STATUS_NO_CARD;
		return false;
	}

	// Setting up interrupts and related things.
	int_semaphore = xSemaphoreCreateBinary();

	// Reset card and enter idle state in SD mode.
	{
		if ( !send_command_r1(GO_IDLE_STATE, 0, STATE_IDLE) ) {
			return false;
		}
	}

	// Check interface condition and whether card is version 1 or version 2.
	bool v2;
	{
		uint32_t r7;
		bool ret = send_command_general(SEND_IF_COND, (0b0001 << 8) | 0x5A);
		r7 = SDIO_RESP[0];
		if (ret) {
			// Command was accepted, so this must be a version 2 card.
			v2 = true;
			// Check the interface condition.
			uint8_t check_pattern = r7 & 0xFF;
			if (check_pattern != 0x5A) {
				printf( "SD: Bad R7: 0x%" PRIX32 ", expected LSB 0x5A\n", r7);
				card_state.status = SD_STATUS_ILLEGAL_RESPONSE;
				return false;
			}
			uint8_t voltage_mask = (r7 >> 8) & 0x0F;
			if (voltage_mask != 0x1) {
				printf( "SD: Bad R7: accepted voltage mask=0x%" PRIX8 ", expected 0x1\n", voltage_mask);
				card_state.status = SD_STATUS_INCOMPATIBLE_CARD;
				return false;
			}
		} else {
			if (card_state.status == SD_STATUS_COMMAND_RESPONSE_TIMEOUT) {
				// Command response timeout indicates this is a version 1 card, and is not actually an error.
				v2 = false;  
			} else {
				// Command failed for some other reason which should not happen.
				return false;
 			}
		}
	}
	{
		printf("SD: V%u card, interface OK\n", v2 ? 2U : 1U);
	}

	// Initialize the card.
	{
		//30th bit is HCS -> set it for v2 cards
		uint32_t arg = v2 ? (UINT32_C(1) << 30 | 0x00300000 ) : 0x00300000; // Only V2 cards are allowed to see an SDHC host.
	}

//SD mode specific ACMD41 routine
//need to be able to time for 1 second. During 1 second, repeatedly send ACMD41 until bit 31 (busy bit) in response to ACMD41 is no longer 0.
	for (;;) // since it's apparently pointless to count 1 second
	{	  	
	
		if (!send_command_r1(APP_CMD, 0, IDLE) && card_state.status != SD_STATUS_ILLEGAL_STATE) {
			return false; 
		// when busy bit = 1, initialization complete
		{
		if (!send_command_general(SD_SEND_OP_COND, arg )) return false;
		else {
			if (SDIO_RESP[0]>>31) break;
		}
	}

	if ( !( SDIO_RESP[0] >> 20 & 0x01 || SDIO_RESP[0] >> 21 & 0x01 )) {
		printf ("Unacceptable voltage.");
		return false;
	}

	card_state.status = SD_STATUS_UNINITIALIZED	
	// Determine card capacity class (SDSC vs SDHC/SDXC).
	if (v2) {
		card_state.sdhc =  ((SDIO_RESP[0] >> 30) & 0x02) ? true : false;   
	else {
		card_state.sdhc = false;
	}
	{
		printf("SD: SD%cC init OK\n", card_state.sdhc ? 'H' : 'S');
	}
	
	//CMD2 and CMD3 to finish in SD mode
	if ( !send_command_general( ALL_SEND_CID, 0 ) ) return false; 
	uint16_t RCA;
	if ( !send_command_r6( SEND_RELATIVE_ADDR, 0, &RCA, STATE_STBY ) ) return false;
	
	// Set block length to 512 bytes (this is ignored in all the ways we care about for SDHC/SDXC, as they always use a 512 byte block length, but harmless).
	if ( !send_command_r1(SET_BLOCKLEN, 512)) {
		return false;
	}
	
	//TODO: UHS-I specific. 

	// Set multiblock write erase length.
	if (!send_command_r1(APP_CMD, 0)) {
		return false;
	}
	if (!send_command_r1(SET_WR_BLK_ERASE_COUNT, 256)) {
		return false;
	}

	// Read the card specific data register and compute the number of sectors on the card.
	{
		uint8_t csd[16];
		if (!send_command_general(SEND_CSD, RCA) {
			return false;
		}

		for ( int i = 0; i<4 ; i++ )
		{
			for (int k = 0; k<4; k++ )
			{
				csd[i*4+k] = SDIO_RESP[i] >> (4-k-1)*8;
			}
		}	
			
		
		uint8_t csd_structure = csd[0] >> 6;
		if (csd_structure == 0) {
			// CSD structure version 1.0
			uint8_t read_bl_len = csd[5] & 0x0F;
			uint16_t c_size = (((uint16_t) (csd[6] & 0x03)) << 10) | (((uint16_t) csd[7]) << 2) | (csd[8] >> 6);
			uint8_t c_size_mult = ((csd[9] & 0x03) << 1) | (csd[10] >> 7);
			uint16_t block_len = 1 << read_bl_len;
			uint16_t mult = 1 << (c_size_mult + 2);
			uint32_t blocknr = ((uint32_t) (c_size + 1)) * mult;
			uint32_t bytes = blocknr * block_len;
			card_state.sector_count = bytes / 512;
		} else if (csd_structure == 1) {
			// CSD structure version 2.0
			uint32_t c_size = (((uint32_t) (csd[7] & 0x3F)) << 16) | (((uint32_t) csd[8]) << 8) | csd[9];
			card_state.sector_count = (c_size + 1) * 1024; // bytes = (c_size + 1) * 512 kB
		} else {
			printf("SD: CSD_STRUCTURE=%" PRIu8 " (expected 0 or 1)\n", (uint8_t) (csd[0] >> 6));
			card_state.status = SD_STATUS_ILLEGAL_RESPONSE;
			return false;
		}
	}
	{
		printf("SD: %" PRIu32 " sectors\n", card_state.sector_count);
	}
	// Compute the timeout times using CSD data
	// TAAC = csd[1], NSAC = csd[2]
	{
		float Data_timeout = (( convert_TAAC(csd[1])*10e-9/(1.0/CLOCK) + csd[2] * 100 ) * 100) ;
		float Hundredms = 0.1/(1/CLOCK);
		if ( Data_timeout < Hundredms )
			SDIO_DTIMER = (uint32_t)Data_timeout;
		else SDIO_DTIMER = (uint32_t) Hundredms;
	}
	SDIO_DLEN = (uint32_t) 512;
		

	card_state.status = SD_STATUS_OK;
	card_state.write_multi_active = false;
	
	//selects the only card and set bus width.	
	if (!send_command_r1(SELECT_CARD, RCA)) return false;
	
	if (!send_command_r1(APP_CMD, 0)) return false;

	argument = 2;

	if (!send_command_r1(SET_BUS_WIDTH, argument)) return false;


	return true;
}

uint32_t sd_sector_count(void) {
	return card_state.status == SD_STATUS_OK ? card_state.sector_count : UINT32_C(0);
}

bool sd_read(uint32_t sector, void *buffer) {
	// Sanity check.
	if (card_state.status != SD_STATUS_OK) {
		return false;
	}
	if (card_state.write_multi_active) {
		card_state.status = SD_STATUS_LOGICAL_ERROR;
		printf( "SD: read during write multi\n" );
		return false;
	}

	// Execute operation.
	return send_command_r1_data_read(READ_SINGLE_BLOCK, card_state.sdhc ? sector : sector * 512U, buffer, 512);
}

bool sd_write_multi_active(void) {
	return card_state.status == SD_STATUS_OK && card_state.write_multi_active;
}

bool sd_write_multi_start(uint32_t sector) {
	// Sanity check.
	if (card_state.status != SD_STATUS_OK) {
		return false;
	}
	if (card_state.write_multi_active) {
		card_state.status = SD_STATUS_LOGICAL_ERROR;
		printf( "SD: write multi start during write multi" );
		return false;
	}

	// Send the command to start the operation.
	if (!send_command_r1_nc(WRITE_MULTIPLE_BLOCK, card_state.sdhc ? sector : sector * 512U)) {
		return false;
	}

	card_state.write_multi_active = true;
	return true;
}

bool sd_write_multi_busy(void) {
	return IO_SD.csr.dma_enable;
}

bool sd_write_multi_sector(const void *data) {
	// Sanity check.
	if (card_state.status != SD_STATUS_OK) {
		return false;
	}
	if (!card_state.write_multi_active) {
		card_state.status = SD_STATUS_LOGICAL_ERROR;
		printf("SD: write multi sector outside write multi\n");
		return false;
	}
	if (sd_write_multi_busy()) {
		card_state.status = SD_STATUS_LOGICAL_ERROR;
		printf("SD: write multi sector while busy\n");
		return false;
	}
	{
		io_sd_csr_t csr = IO_SD.csr;
		if (csr.malformed_drt) {
			card_state.status = SD_STATUS_ILLEGAL_RESPONSE;
			syscall_debug_puts(MALFORMED_DRT_MESSAGE);
			return false;
		} else if (csr.internal_error) {
			card_state.status = SD_STATUS_CARD_INTERNAL_ERROR;
			syscall_debug_puts(INTERNAL_ERROR_MESSAGE);
			return false;
		} else if (csr.crc_error) {
			card_state.status = SD_STATUS_CRC_ERROR;
			syscall_debug_puts(CRC_ERROR_MESSAGE);
			return false;
		} else if (csr.ahb_error) {
			card_state.status = SD_STATUS_LOGICAL_ERROR;
			syscall_debug_puts(AHB_ERROR_MESSAGE);
			return false;
		}
	}

	// Start the DMA transfer.
	IO_SD.dma_address = data;
	IO_SD.dma_length = 512;
	__sync_synchronize();
	IO_SD.csr.dma_enable = 1;
	return true;
}

bool sd_write_multi_end(void) {
	// Sanity check.
	if (card_state.status != SD_STATUS_OK) {
		return false;
	}
	if (!card_state.write_multi_active) {
		card_state.status = SD_STATUS_LOGICAL_ERROR;
		static char MESSAGE[] = "SD: write multi end outside write multi\n";
		syscall_debug_puts(MESSAGE);
		return false;
	}

	// Wait for non-busy status.
	while (sd_write_multi_busy());
	while (IO_SD.csr.busy);

	// Check for any errors in the final sector.
	bool ret = true;
	{
		io_sd_csr_t csr = IO_SD.csr;
		if (csr.malformed_drt) {
			card_state.status = SD_STATUS_ILLEGAL_RESPONSE;
			syscall_debug_puts(MALFORMED_DRT_MESSAGE);
			ret = false;
		} else if (csr.internal_error) {
			card_state.status = SD_STATUS_CARD_INTERNAL_ERROR;
			syscall_debug_puts(INTERNAL_ERROR_MESSAGE);
			ret = false;
		} else if (csr.crc_error) {
			card_state.status = SD_STATUS_CRC_ERROR;
			syscall_debug_puts(CRC_ERROR_MESSAGE);
			ret = false;
		} else if (csr.ahb_error) {
			card_state.status = SD_STATUS_LOGICAL_ERROR;
			syscall_debug_puts(AHB_ERROR_MESSAGE);
			ret = false;
		}
	}

	// Shut down the multi-sector write.
	nop8();
	spi_transmit(STOP_TRAN_TOKEN);
	nop8();
	while (!nop8r());
	close_transaction();
	card_state.write_multi_active = false;

	return ret;
}

