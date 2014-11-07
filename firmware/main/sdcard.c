#include "sdcard.h"
#include "dma.h"
#include "pins.h"
#include "priority.h"
#include <FreeRTOS.h>
#include <inttypes.h>
#include <rcc.h>
#include <semphr.h>
#include <stddef.h>
#include <stdio.h>
#include <task.h>
#include <registers/dma.h>
#include <registers/sdio.h>

typedef enum {
	GO_IDLE_STATE = 0,
	ALL_SEND_CID = 2,
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

typedef enum {
	COMMAND_FLAG_NO_RESPONSE = 0x01,
	COMMAND_FLAG_IGNORE_CRC = 0x02,
	COMMAND_FLAG_LONG_RESPONSE = 0x04,
	COMMAND_FLAG_IGNORE_RESPCMD = 0x08,
} sd_command_flags_t;

#define STATE_IDLE 0
#define STATE_READY 1
#define STATE_IDENT 2
#define STATE_STBY 3
#define STATE_TRAN 4
#define STATE_DATA 5
#define STATE_RCV 6
#define STATE_PRG 7

#define DATA_TIMEOUT_TIME 65535U
#define DATA_LENGTH 512U
#define CLOCK 48000000U

#define SD_DMA_STREAM 6U
#define SD_DMA_CHANNEL 4U

static SemaphoreHandle_t int_semaphore = NULL;

typedef struct {
	sd_status_t status;
	uint32_t sector_count;
	bool sdhc;
} sd_ctx_t;

static sd_ctx_t card_state = {
	.status = SD_STATUS_UNINITIALIZED,
};

/**
 * \brief Handles SD card interrupts.
 *
 * This function should be registered in the application’s interrupt vector table at position 49.
 */
void sd_isr(void) {
	// clear the mask
	
	SDIO_MASK_t mask_temp = { 0 };
	SDIO.MASK = mask_temp;
	BaseType_t yield = pdFALSE;
	xSemaphoreGiveFromISR(int_semaphore, &yield);
	if (yield) {
		portYIELD_FROM_ISR();
	}
}

static bool card_present(void) {
	// When a card is inserted, it will apply a 50 kΩ pull-up resistor to D3.
	// Because the STM32’s internal pull-ups are also on the order of 50 kΩ,
	// they must be disabled. We will drive the pin low, then completely float
	// it and wait a tick. If a card is present, the resistor should pull the
	// pin high very quickly. If no card is present, pin capacitance should
	// keep the pin low.
	//
	// The connector does have a physical card presence switch, but it
	// occasionally fails. We might as well use D3-based detection instead,
	// since if that fails, the card won’t work anyway.
	gpio_reset(PIN_SD_D3);
	gpio_set_mode(PIN_SD_D3, GPIO_MODE_OUT);
	gpio_set_pupd(PIN_SD_D3, GPIO_PUPD_NONE);
	vTaskDelay(1U);
	gpio_set_mode(PIN_SD_D3, GPIO_MODE_IN);
	vTaskDelay(1U);
	bool present = gpio_get_input(PIN_SD_D3);
	gpio_set_pupd(PIN_SD_D3, GPIO_PUPD_PU);
	gpio_set_mode(PIN_SD_D3, GPIO_MODE_AF);
	return present;
}

static float convert_TAAC ( uint8_t TAAC )
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
		printf ("Error: time value = 0 (reserved)\r\n");
		TAAC_in_nanosecs = 0;
	}
	for ( int i = 0; i<(int)multiplier; i++ )
	{
		TAAC_in_nanosecs *= 10;
	}
	return TAAC_in_nanosecs;
}

static void clear_cpsm_interrupts(void) {
	// Clear the flags.
	SDIO_ICR_t temp = {
		.CMDSENTC = 1,
		.CMDRENDC = 1,
		.CTIMEOUTC = 1,
		.CCRCFAILC = 1,
	};
	SDIO.ICR = temp;

	// A dummy read appears to be necessary to delay the CPU long enough for the SDIO.STA flags to actually show as clear.
	(void) SDIO.ICR;
}

static void clear_dpsm_interrupts(void) {
	// Clear the flags.
	SDIO_ICR_t temp = {
		.DBCKENDC = 1,
		.STBITERRC = 1,
		.DATAENDC = 1,
		.RXOVERRC = 1,
		.TXUNDERRC = 1,
		.DTIMEOUTC = 1,
		.DCRCFAILC = 1,
	};
	SDIO.ICR = temp;

	// A dummy read appears to be necessary to delay the CPU long enough for the SDIO.STA flags to actually show as clear.
	(void) SDIO.ICR;
}

static bool send_command(uint8_t command, uint32_t argument, unsigned int flags) {
	// All long responses do not contain a response command field.
	if (flags & COMMAND_FLAG_LONG_RESPONSE) {
		flags |= COMMAND_FLAG_IGNORE_RESPCMD;
	}

	// Clear all old interrupts.
	clear_cpsm_interrupts();

	// Enable interrupts based on type of command.
	if (flags & COMMAND_FLAG_NO_RESPONSE) {
		SDIO_MASK_t temp = { .CMDSENTIE = 1 };
		SDIO.MASK = temp;
	} else {
		SDIO_MASK_t temp = { .CTIMEOUTIE = 1, .CMDRENDIE = 1, .CCRCFAILIE = 1 };
		SDIO.MASK = temp;
	}

	// Start up command path state machine.
	SDIO.ARG = argument;
	SDIO_CMD_t cmd_temp = {
		.CPSMEN = 1,
		.CMDINDEX = command,
	};
	if (flags & COMMAND_FLAG_NO_RESPONSE) {
		cmd_temp.WAITRESP = 0b00;
	} else if (flags & COMMAND_FLAG_LONG_RESPONSE) {
		cmd_temp.WAITRESP = 0b11;
	} else {
		cmd_temp.WAITRESP = 0b01;
	}
	SDIO.CMD = cmd_temp;

	// Wait for operation to finish.
	xSemaphoreTake(int_semaphore, portMAX_DELAY);

	// Check what happened.
	if (SDIO.STA.CMDREND) {
		// Response received with correct CRC.
		if ((flags & COMMAND_FLAG_IGNORE_RESPCMD) || SDIO.RESPCMD == command) {
			// Response was for the same command.
			return true;
		} else {
			iprintf("SD: Command %" PRIu8 " had different response command %" PRIu8 ".\r\n", command, (uint8_t) SDIO.RESPCMD);
			card_state.status = SD_STATUS_ILLEGAL_RESPONSE;
			return false;
		}
	} else if ((flags & COMMAND_FLAG_NO_RESPONSE) && SDIO.STA.CMDSENT) {
		// Command sent, and no response expected.
		return true;
	} else if (SDIO.STA.CCRCFAIL) {
		// CRC failed.
		if (flags & COMMAND_FLAG_IGNORE_CRC) {
			return true;
		} else {
			iprintf("SD: Command %" PRIu8 " CRC error.\r\n", command);
			card_state.status = SD_STATUS_CRC_ERROR;
			return false;
		}
	} else if (SDIO.STA.CTIMEOUT) {
		// Timeout waiting for response.
		iprintf("SD: Command %" PRIu8 " response timeout.\r\n", command);
		card_state.status = SD_STATUS_COMMAND_RESPONSE_TIMEOUT;
		return false;
	} else {
		// No idea how we got here.
		iprintf("SD: Command %" PRIu8 " host controller logic error.\r\n", command);
		card_state.status = SD_STATUS_LOGICAL_ERROR;
		return false;
	}
}

//send command in response 1 - no close
static bool send_command_r1(uint8_t command, uint32_t argument, uint8_t state_expected) {
	if (!send_command(command, argument, 0U)) {
		return false;
	}

	//grab the card response gained by sending command	
	uint32_t r1 = SDIO.RESP[0];

	uint8_t current_state = (r1 >> 9)&0x0F;
	r1 &= 0xFFF9E008;//zero everything that doesn't need to be checked. 

	if ( current_state != state_expected ) {
		iprintf("SD: Command %" PRIu8 ": Incorrect starting state %" PRIu8 ", expected %" PRIu8 ".\r\n", command, current_state, state_expected);
		card_state.status = SD_STATUS_ILLEGAL_STATE;
		return false;
	} 

	if (!r1) {	//no error occured on card's side. 
		return true;
	}
	else if ( r1 >> 31 & 0x01 ) {
		card_state.status = SD_STATUS_OUT_OF_RANGE;
	} else if ( r1 >> 30 & 0x01 ) {
		card_state.status = SD_STATUS_ADDRESS_MISALIGN;
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

	printf("SD: Command %" PRIu8 " failed (R1 = 0x%" PRIX32 ").\r\n", command, SDIO.RESP[0U]);

	return false;
}

static bool send_command_r6 (uint8_t command, uint32_t argument,  uint16_t * RCA, uint8_t state_expected )
{
	if (!send_command(command, argument, 0U)) {
		return false;
	}

	//grab the card response  gained by sending command	
	uint32_t r6 = SDIO.RESP[0];
	*RCA = r6 >> 16;

	uint8_t current_state = (r6 >> 9)&0x0F;
	r6 &= 0x0000E008;//zero everything that doesn't need to be checked. 

	if ( current_state != state_expected ) {
		iprintf("SD: Command %" PRIu8 ": Incorrect starting state %" PRIu8 ", expected %" PRIu8 ".\r\n", command, current_state, state_expected);
		card_state.status = SD_STATUS_ILLEGAL_STATE;
		return false;
	}

	if ( !r6 ) {
		return true;  
	} else if ( r6 >> 15 & 0x01 ) {
		card_state.status = SD_STATUS_COM_CRC_ERROR;
	} else if ( r6 >> 14 & 0x01 ) {
		card_state.status = SD_STATUS_ILLEGAL_COMMAND;
	} else if ( r6 >> 13 & 0x01 ) {
		card_state.status = SD_STATUS_ERROR;
	} else if ( r6 >> 3 & 0x01 ) {
		card_state.status = SD_STATUS_AKE_SEQ_ERROR;
	}

	printf("SD: Command %" PRIu8 " failed (R6 = 0x%" PRIX32 ").\r\n", command, SDIO.RESP[0U]);

	return false;
}


/**
 * \brief Returns the current status of the SD card.
 *
 * \return the current status, including the last error if an operation failed
 */
sd_status_t sd_status(void) {
	return card_state.status;
}

/**
 * \brief Initializes the SD card.
 *
 * \retval true initialization succeeded
 * \retval false an error occurred
 */
bool sd_init(void) {

	card_state.status = SD_STATUS_UNINITIALIZED;

	// Check for card.
	if (!card_present()) {
		printf( "SD: No card\r\n" );
		card_state.status = SD_STATUS_NO_CARD;
		return false;
	}

	// Enable the SD host controller.
	rcc_enable_reset(APB2, SDIO);
	
	//Initialize the STM32 SD card registers
	SDIO.POWER.PWRCTRL = 0b11;
	{
		SDIO_CLKCR_t tmp = {
			.HWFC_EN = 0, // Errata: hardware flow control doesn’t work.
			.NEGEDGE = 0, // Clock output pin matches rising edge of internal clock signal.
			.WIDBUS = 0U, // One-bit bus mode.
			.BYPASS = 0, // Do not bypass clock divider.
			.PWRSAV = 0, // Output clock always, not only when accessing card.
			.CLKEN = 1, // Enable clock.
			.CLKDIV = 118, // 48M/400k = 120 = 118 + 2.
		};
		SDIO.CLKCR = tmp;
	}

	// Setting up interrupts and related things.
	int_semaphore = xSemaphoreCreateBinary();

	// Unmask SD card interrupts.
	portENABLE_HW_INTERRUPT(49U, PRIO_EXCEPTION_SD);

	// Unmask DMA interrupts for the channel. These should never actually
	// happen, because the specific interrupt causes we enable for the DMA
	// channel are always error conditions. Thus, there isn’t actually a handler
	// for this interrupt; instead, if it ever happens, it will crash the
	// system.
	portENABLE_HW_INTERRUPT(69U, 0U);

	vTaskDelay(1000U / portTICK_PERIOD_MS);

	// Reset card and enter idle state in SD mode.
	send_command(GO_IDLE_STATE, 0U, COMMAND_FLAG_NO_RESPONSE);

	// Check interface condition and whether card is version 1 or version 2.
	bool v2;
	{
		uint32_t r7;
		bool ret = send_command(SEND_IF_COND, (0b0001 << 8) | 0x5A, 0U);
		r7 = SDIO.RESP[0];
		if (ret) {
			// Command was accepted, so this must be a version 2 card.
			v2 = true;
			// Check the interface condition.
			uint8_t check_pattern = r7 & 0xFF;
			if (check_pattern != 0x5A) {
				printf( "SD: Bad R7: 0x%" PRIX32 ", expected LSB 0x5A\r\n", r7);
				card_state.status = SD_STATUS_ILLEGAL_RESPONSE;
				return false;
			}
			uint8_t voltage_mask = (r7 >> 8) & 0x0F;
			if (voltage_mask != 0x1) {
				printf( "SD: Bad R7: accepted voltage mask=0x%" PRIX8 ", expected 0x1\r\n", voltage_mask);
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
		printf("SD: V%u card, interface OK\r\n", v2 ? 2U : 1U);
	}

	//30th bit is HCS -> set it for v2 cards
	uint32_t arg = v2 ? (UINT32_C(1) << 30 | 0x00300000 ) : 0x00300000; // Only V2 cards are allowed to see an SDHC host.

//SD mode specific ACMD41 routine
//need to be able to time for 1 second. During 1 second, repeatedly send ACMD41 until bit 31 (busy bit) in response to ACMD41 is no longer 0.
	for (;;) // since it's apparently pointless to count 1 second
	{	  	
		if (!send_command_r1(APP_CMD, 0, STATE_IDLE) && card_state.status != SD_STATUS_ILLEGAL_STATE) {
			return false; 
		// when busy bit = 1, initialization complete
		}
		if (!send_command(SD_SEND_OP_COND, arg, COMMAND_FLAG_IGNORE_CRC)) {
			return false;
		} else if (SDIO.RESP[0]>>31) {
			break;
		}
		vTaskDelay(1U);
	}

	if ( !( SDIO.RESP[0] >> 20 & 0x01 || SDIO.RESP[0] >> 21 & 0x01 )) {
		printf ("Unacceptable voltage.");
		return false;
	}

	card_state.status = SD_STATUS_UNINITIALIZED;	
	// Determine card capacity class (SDSC vs SDHC/SDXC).
	if (v2) {
		card_state.sdhc = (SDIO.RESP[0] >> 30) & 0x01;
	} else {
		card_state.sdhc = false;
	}
	{
		printf("SD: SD%cC init OK\r\n", card_state.sdhc ? 'H' : 'S');
	}
	
	//CMD2 and CMD3 to finish in SD mode
	if ( !send_command( ALL_SEND_CID, 0, COMMAND_FLAG_LONG_RESPONSE ) ) return false; 
	uint16_t RCA;
	if ( !send_command_r6( SEND_RELATIVE_ADDR, 0, &RCA, STATE_IDENT ) ) return false;

	{
		uint8_t csd[16];
		if (!send_command(SEND_CSD, RCA << 16U, COMMAND_FLAG_LONG_RESPONSE)) {
			return false;
		}

		for ( int i = 0; i<4 ; i++ )
		{
			for (int k = 0; k<4; k++ )
			{
				csd[i*4+k] = SDIO.RESP[i] >> (4-k-1)*8;
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
			printf("SD: CSD_STRUCTURE=%" PRIu8 " (expected 0 or 1)\r\n", (uint8_t) (csd[0] >> 6));
			card_state.status = SD_STATUS_ILLEGAL_RESPONSE;
			return false;
		}
	
	
		printf("SD: %" PRIu32 " sectors\r\n", card_state.sector_count);
	
	// Compute the timeout times using CSD data
	// TAAC = csd[1], NSAC = csd[2]
	
		float Data_timeout = ( convert_TAAC(csd[1])*10e-9F/(1.0F/CLOCK) + csd[2] * 100 ) * 100 ;
		float Hundredms = 0.1F/(1.0F/CLOCK);
		if ( Data_timeout < Hundredms )
			SDIO.DTIMER = (uint32_t)Data_timeout;
		else SDIO.DTIMER = (uint32_t) Hundredms;
	}

	if (!send_command_r1(SELECT_CARD, RCA << 16U, STATE_STBY)) return false;

	// Set block length to 512 bytes (this is ignored in all the ways we care about for SDHC/SDXC, as they always use a 512 byte block length, but harmless).
	if ( !send_command_r1(SET_BLOCKLEN, 512, STATE_TRAN)) {
		return false;
	}
	
	//TODO: UHS-I specific. 

	// Read the card specific data register and compute the number of sectors on the card.
	SDIO.DLEN = (uint32_t) 512;

	card_state.status = SD_STATUS_OK;

	// Detach pull-up resistor from DATA3.
	if (!send_command_r1(APP_CMD, RCA << 16U, STATE_TRAN)) {
		return false;
	}
	if (!send_command_r1(SET_CLR_CARD_DETECT, 0U, STATE_TRAN)) {
		return false;
	}

	// Set bus width.
	SDIO_CLKCR_t clkcr_tmp = SDIO.CLKCR;
	if (!send_command_r1(APP_CMD, RCA << 16U, STATE_TRAN)) {
		return false;
	}
	if (!send_command_r1(SET_BUS_WIDTH, 2U, STATE_TRAN)) {
		printf("SD: 4-bit wide bus not supported.\r\n");
	} else {
		clkcr_tmp.WIDBUS = 0b01;
	}

	// Increase clock frequency.
	// Note, this and the above WIDBUS change are rolled into one register write because it is not permitted to write to SDIO.CLKCR twice in quick succession.
	clkcr_tmp.CLKDIV = 0;
	SDIO.CLKCR = clkcr_tmp;

	return true;
}

/**
 * \brief Reads a sector from the SD card.
 *
 * \param[in] sector the sector to read
 *
 * \param[out] buffer a 512-byte buffer in which to store the sector data
 *
 * \retval true read completed successfully
 * \retval false an error occurred
 */
bool sd_read(uint32_t sector, void *buffer) {
	// Sanity check.
	assert(dma_check(buffer, 512U));
	assert(!(((uintptr_t) buffer) & 15U));

	// Clear pending DMA interrupts.
	DMA_HIFCR_t temp_hifcr = {
		.CFEIF6 = 1U,
		.CDMEIF6 = 1U,
		.CTEIF6 = 1U,
		.CHTIF6 = 1U,
		.CTCIF6 = 1U,
	};
	DMA2.HIFCR = temp_hifcr;

	// Initialize DMA engine.
	DMA2.streams[SD_DMA_STREAM].M0AR = buffer;
	DMA2.streams[SD_DMA_STREAM].PAR = &SDIO.FIFO;
	DMA_SxFCR_t temp_FCR = {
		.FTH = DMA_FIFO_THRESHOLD_FULL,
		.DMDIS = 1,
		.FEIE = 0,
	};
	DMA2.streams[SD_DMA_STREAM].FCR = temp_FCR;
	DMA_SxCR_t temp_CR = {
		.EN = 1,
		.DMEIE = 1,
		.TEIE = 1,
		.TCIE = 0,
		.PFCTRL = 1,
		.DIR = DMA_DIR_P2M,
		.CIRC = 0,
		.PINC = 0,
		.MINC = 1,
		.PSIZE = DMA_DSIZE_WORD,
		.MSIZE = DMA_DSIZE_WORD,
		.PINCOS = 0,
		.PL = 0, 
		.DBM = 0,
		.CT = 0,
		.PBURST = DMA_BURST_INCR4,
		.MBURST = DMA_BURST_INCR4,
		.CHSEL = SD_DMA_CHANNEL,
	};
	DMA2.streams[SD_DMA_STREAM].CR = temp_CR;

	// Clear old DPSM interrupts.
	clear_dpsm_interrupts();

	// Enable the DPSM before sending the command, because the card may start sending back data at any time.
	SDIO_DCTRL_t dctrl_temp = { .DTEN = 1, .DTDIR = 1, .DTMODE = 0, .DMAEN = 1, .DBLOCKSIZE = 9 };
	SDIO.DCTRL = dctrl_temp;

	// Send the command.
	if (!send_command_r1( READ_SINGLE_BLOCK, card_state.sdhc ? sector : (sector * 512U), STATE_TRAN)) {
		// Disable the DMA stream.
		DMA2.streams[SD_DMA_STREAM].CR.EN = 0;
		while (DMA2.streams[SD_DMA_STREAM].CR.EN) {
			taskYIELD();
		}
		SDIO_DCTRL_t dctrl_temp = { .DTEN = 0 };
		SDIO.DCTRL = dctrl_temp;
		return false; 
	}

	// Now that the CPSM is finished, wait for the DPSM to also finish.
	SDIO_MASK_t mask_temp = { .DBCKENDIE = 1, .DCRCFAILIE = 1, .DTIMEOUTIE = 1 };
	SDIO.MASK = mask_temp;
	xSemaphoreTake(int_semaphore, portMAX_DELAY);

	// SD error handling.
	if (SDIO.STA.DTIMEOUT) {
		// Disable the DMA stream.
		DMA2.streams[SD_DMA_STREAM].CR.EN = 0;
		while (DMA2.streams[SD_DMA_STREAM].CR.EN) {
			taskYIELD();
		}
		fputs("SD: Data timeout\r\n", stdout);
		return false;
	} else if (SDIO.STA.DCRCFAIL) {
		// Disable the DMA stream.
		DMA2.streams[SD_DMA_STREAM].CR.EN = 0;
		while (DMA2.streams[SD_DMA_STREAM].CR.EN) {
			taskYIELD();
		}
		fputs("SD: Data CRC failure\r\n", stdout);
		return false;
	}

	// Data block ended successfully; wait for the DMA controller to shut down.
	while (DMA2.streams[SD_DMA_STREAM].CR.EN);

	return true;
}

/**
 * \brief Writes a sector to the SD card.
 *
 * \param[in] sector the sector to write
 *
 * \param[in] data the data to write
 *
 * \retval true write completed successfully
 * \retval false an error occurred
 */
bool sd_write(uint32_t sector, const void *data) {
	// Sanity check.
	assert(dma_check(data, 512U));
	assert(!(((uintptr_t) data) & 15U));

	// Clear pending DMA interrupts.
	DMA_HIFCR_t temp_hifcr = {
		.CFEIF6 = 1U,
		.CDMEIF6 = 1U,
		.CTEIF6 = 1U,
		.CHTIF6 = 1U,
		.CTCIF6 = 1U,
	};
	DMA2.HIFCR = temp_hifcr;

	// Initialize the DMA engine.
	DMA2.streams[SD_DMA_STREAM].M0AR = (void *) data;
	DMA2.streams[SD_DMA_STREAM].PAR = &SDIO.FIFO;
	DMA_SxFCR_t temp_FCR = {
		.FTH = DMA_FIFO_THRESHOLD_FULL,
		.DMDIS = 1,
		.FEIE = 0,
	};
	DMA2.streams[SD_DMA_STREAM].FCR = temp_FCR;
	DMA_SxCR_t temp_CR = {
		.EN = 1,
		.DMEIE = 1,
		.TEIE = 1,
		.TCIE = 0,
		.PFCTRL = 1,
		.DIR = DMA_DIR_M2P,
		.CIRC = 0,
		.PINC = 0,
		.MINC = 1,
		.PSIZE = DMA_DSIZE_WORD,
		.MSIZE = DMA_DSIZE_WORD,
		.PINCOS = 0,
		.PL = 0, 
		.DBM = 0,
		.CT = 0,
		.PBURST = DMA_BURST_INCR4,
		.MBURST = DMA_BURST_INCR4,
		.CHSEL = SD_DMA_CHANNEL,
	};
	DMA2.streams[SD_DMA_STREAM].CR = temp_CR;

	// Clear old DPSM interrupts.
	clear_dpsm_interrupts();

	// Send the command.
	if (!send_command_r1(WRITE_BLOCK, card_state.sdhc ? sector : (sector * 512U), STATE_TRAN)) {
		// Disable the DMA stream.
		DMA2.streams[SD_DMA_STREAM].CR.EN = 0;
		while (DMA2.streams[SD_DMA_STREAM].CR.EN) {
			taskYIELD();
		}
		return false; 
	}

	// Enable the DPSM and transfer the data.
	SDIO_DCTRL_t dctrl_temp = { .DTEN = 1, .DTDIR = 0, .DTMODE = 0, .DMAEN = 1, .DBLOCKSIZE = 9 };
	SDIO.DCTRL = dctrl_temp;
	SDIO_MASK_t mask_temp = { .DBCKENDIE = 1, .DCRCFAILIE = 1, .DTIMEOUTIE = 1 };
	SDIO.MASK = mask_temp;

	// Wait for SD controller to interrupt with error/transfer complete.
	xSemaphoreTake(int_semaphore, portMAX_DELAY);

	// SD error handling.
	if (SDIO.STA.DTIMEOUT) {
		// Disable the DMA stream.
		DMA2.streams[SD_DMA_STREAM].CR.EN = 0;
		while (DMA2.streams[SD_DMA_STREAM].CR.EN) {
			taskYIELD();
		}
		fputs("SD: Data timeout\r\n", stdout);
		return false;
	} else if (SDIO.STA.DCRCFAIL) {
		// Disable the DMA stream.
		DMA2.streams[SD_DMA_STREAM].CR.EN = 0;
		while (DMA2.streams[SD_DMA_STREAM].CR.EN) {
			taskYIELD();
		}
		fputs("SD: Data CRC failure\r\n", stdout);
		return false;
	}

	// Data block ended successfully; wait for the DMA controller to shut down.
	while (DMA2.streams[SD_DMA_STREAM].CR.EN);

	// Wait for DPSM to disable.
	// This is where we wait until SDIO_D0 goes high, indicating the card is no longer busy.
	// Unfortunately there is no way to do this with an interrupt.
	// TXACT, TXFIFOE, and TXFIFOHE all go from 1 to 0 when the DPSM goes idle.
	// However, the host controller only generates interrupts when a status flag goes high, not low.
	// So, busy-wait instead.
	while (SDIO.STA.TXACT) {
		taskYIELD();
	}

	return true;
}

/**
 * \brief Erases a sequence of sectors on the SD card.
 *
 * \param[in] sector the address of the first sector to erase
 * \param[in] count the number of sectors to erase
 * \retval true erase completed successfully
 * \retval false an error occurred
 */
bool sd_erase(uint32_t sector, size_t count) {
	// Sanity check.
	assert(count != 0U);
	assert(sector + count <= sd_sector_count());

	// Send the commands.
	if (!send_command_r1(ERASE_WR_BLK_START_ADDR, card_state.sdhc ? sector : (sector * 512U), STATE_TRAN)) {
		return false;
	}
	if (!send_command_r1(ERASE_WR_BLK_END_ADDR, (card_state.sdhc ? (sector + count) : ((sector + count) * 512U)) - 1U, STATE_TRAN)) {
		return false;
	}
	if (!send_command_r1(ERASE, 0U, STATE_TRAN)) {
		return false;
	}

	// Wait until the card is no longer busy.
	while (!gpio_get_input(PIN_SD_D0));

	return true;
}

/**
 * \brief Returns the number of sectors on the SD card.
 *
 * \return the number of sectors, or 0 on failure
 */
uint32_t sd_sector_count(void) {
	return card_state.status == SD_STATUS_OK ? card_state.sector_count : UINT32_C(0);
}
