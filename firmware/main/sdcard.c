#include "sdcard.h"
#include "dma.h"
#include "io.h"
#include <stddef.h>
#include <stdio.h>

typedef enum {
	GO_IDLE_STATE = 0,
	SEND_OP_COND = 1,
	SWITCH_FUNC = 6, 
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
	READ_OCR = 58,
	CRC_ON_OFF = 59,
} sd_cmd_t;

typedef enum {
	SD_STATUS = 13,
	SEND_NUM_WR_BLOCKS = 22,
	SET_WR_BLK_ERASE_COUNT = 23,
	SD_SEND_OP_COND = 41,
	SET_CLR_CARD_DETECT = 42,
	SEND_SCR = 51,
} sd_acmd_t;

#define START_BLOCK_TOKEN 0b11111110
#define START_BLOCK_MULTI_TOKEN 0b11111100
#define STOP_TRAN_TOKEN 0b11111101
#define LINE_IDLE_STATE 0xFF



typedef struct {
	sd_status_t status;
	uint32_t sector_count;
	bool expect_idle;
	bool sdhc;
	bool write_multi_active;
} sd_ctx_t;

static sd_ctx_t card_state = {
	.status = SD_STATUS_UNINITIALIZED,
};



static void spi_assert_cs(void) {
	SD_CTL &= ~4;
}

static void spi_deassert_cs(void) {
	SD_CTL |= 4;
}

static void spi_transmit(uint8_t send) {
	SD_DATA = send;
	while (!!(SD_CTL & 1));
}

static uint8_t spi_transceive(uint8_t send) {
	spi_transmit(send);
	return SD_DATA;
}



static bool card_present(void) {
	return !!(SD_CTL & 2);
}

static void nop8(void) {
	spi_transmit(LINE_IDLE_STATE);
}

static uint8_t nop8r(void) {
	return spi_transceive(LINE_IDLE_STATE);
}

static uint32_t nop32r(void) {
	uint32_t acc = 0;
	uint8_t bits = 4;
	do {
		acc <<= 8;
		acc |= spi_transceive(LINE_IDLE_STATE);
	} while (--bits);
	return acc;
}

static uint8_t crc7(const void *data, size_t len) {
	const uint8_t *pdata = data;
	uint8_t crc = 0;

	do {
		uint8_t byte = *pdata++;
		uint8_t bits = 8;
		do {
			crc <<= 1;
			if ((byte & 0x80) ^ (crc & 0x80)) {
				crc ^= 0x09;
			}
			byte <<= 1;
		} while (--bits);
	} while (--len);

	return crc;
}

static uint16_t crc16(const void *data, size_t len) {
	const uint8_t *pdata = data;
	uint16_t crc = 0;

	do {
		uint8_t s = *pdata++ ^ (crc >> 8);
		uint8_t t = s ^ (s >> 4);
		crc = (crc << 8) ^ t ^ (t << 5) ^ (t << 12);
	} while (--len);

	return crc;
}

static void close_transaction(void) {
	spi_deassert_cs();
	nop8();
}

static bool read_data_token(void) {
	uint8_t token;
	do {
		token = nop8r();
	} while (token == LINE_IDLE_STATE);
	if (token == START_BLOCK_TOKEN) {
		return true;
	}
	printf("SD: Illegal or data error token 0x%" PRIX8 "\n", token);
	if (token & 0xF0) {
		card_state.status = SD_STATUS_ILLEGAL_RESPONSE;
	} else if (token & 0x08) {
		card_state.status = SD_STATUS_LOGICAL_ERROR;
	} else {
		card_state.status = SD_STATUS_CARD_INTERNAL_ERROR;
	}
	return false;
}

static bool send_command_r1_nc(uint8_t command, uint32_t argument) {
	uint8_t buffer[6];
	buffer[0] = command | 0x40;
	buffer[1] = argument >> 24;
	buffer[2] = argument >> 16;
	buffer[3] = argument >> 8;
	buffer[4] = argument;
	buffer[5] = (crc7(buffer, 5) << 1) | 1;

	spi_assert_cs();
	for (uint8_t i = 0; i < 6; ++i) {
		spi_transmit(buffer[i]);
	}
	uint8_t r1;
	do {
		r1 = nop8r();
	} while (r1 == LINE_IDLE_STATE);
	if (r1 == card_state.expect_idle ? 0x01 : 0x00) {
		return true;
	}

	printf("SD: Command %" PRIu8 ": Bad R1: 0x%" PRIX8 "\n", (uint8_t) command, r1);
	if (r1 & 0x80) {
		card_state.status = SD_STATUS_ILLEGAL_RESPONSE;
	} else if (r1 & 0x08) {
		card_state.status = SD_STATUS_CRC_ERROR;
	} else if (r1 & 0x04) {
		card_state.status = SD_STATUS_ILLEGAL_COMMAND;
	} else if ((r1 & 0x01) != (card_state.expect_idle ? 0x01 : 0x00)) {
		card_state.status = SD_STATUS_ILLEGAL_IDLE;
	} else {
		card_state.status = SD_STATUS_LOGICAL_ERROR;
	}
	return false;
}

static bool send_command_r1(uint8_t command, uint32_t argument) {
	bool ret = send_command_r1_nc(command, argument);
	close_transaction();
	return ret;
}

static bool send_command_r1_x32(uint8_t command, uint32_t argument, uint32_t *extra_response) {
	bool ret = send_command_r1_nc(command, argument);
	if (ret) {
		*extra_response = nop32r();
	}
	close_transaction();
	return ret;
}

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
		printf("SD: RX data CRC error (calc %" PRIX16 ", RX %" PRIX16 ")\n", calculated_crc, received_crc);
		card_state.status = SD_STATUS_CRC_ERROR;
		return false;
	}
	return true;
}



sd_status_t sd_status(void) {
	return card_state.status;
}

bool sd_init(void) {
	// Reset status.
	card_state.status = SD_STATUS_UNINITIALIZED;

	// Check for card.
	if (!card_present()) {
		puts("SD: No card");
		card_state.status = SD_STATUS_NO_CARD;
		return false;
	}

	// Reset bus.
	{
		spi_deassert_cs();
		uint8_t count = 80;
		do {
			nop8();
		} while (--count);
	}

	// Reset card and enter idle state in SPI mode.
	{
		card_state.expect_idle = true;
		if (!send_command_r1(GO_IDLE_STATE, 0)) {
			return false;
		}
	}

	// Check interface condition and whether card is version 1 or version 2.
	bool v2;
	{
		uint32_t r7;
		bool ret = send_command_r1_x32(SEND_IF_COND, (0b0001 << 8) | 0x5A, &r7);
		if (ret) {
			// Command was accepted, so this must be a version 2 card.
			v2 = true;
			// Check the interface condition.
			uint8_t check_pattern = r7 & 0xFF;
			if (check_pattern != 0x5A) {
				printf("SD: Bad R7: 0x%" PRIX32 ", expected LSB 0x5A\n", r7);
				card_state.status = SD_STATUS_ILLEGAL_RESPONSE;
				return false;
			}
			uint8_t voltage_mask = (r7 >> 8) & 0x0F;
			if (voltage_mask != 0x1) {
				printf("SD: Bad R7: accepted voltage mask=0x%" PRIX8 ", expected 0x1\n", voltage_mask);
				card_state.status = SD_STATUS_INCOMPATIBLE_CARD;
				return false;
			}
		} else {
			if (card_state.status == SD_STATUS_ILLEGAL_COMMAND) {
				// Illegal command indicates this is a version 1 card, and is not actually an error.
				v2 = false;
			} else {
				// Command failed for some other reason which should not happen.
				return false;
			}
		}
	}
	printf("SD: V%" PRIu8 " card, interface OK\n", v2 ? (uint8_t) 2 : (uint8_t) 1);

	// Enable CRC protection of transactions.
	if (!send_command_r1(CRC_ON_OFF, 1)) {
		return false;
	}

	// Check OCR for compatible voltage range.
	{
		uint32_t ocr;
		if (!send_command_r1_x32(READ_OCR, 0, &ocr)) {
			return false;
		}
		if (!(ocr & (UINT32_C(3) << 20))) {
			printf("SD: OCR=0x%" PRIX32 ", 3.2 to 3.4 volts not supported\n", ocr);
			card_state.status = SD_STATUS_INCOMPATIBLE_CARD;
			return false;
		}
	}

	// Initialize the card.
	{
		card_state.expect_idle = false;
		uint32_t arg = v2 ? (UINT32_C(1) << 30) : 0; // Only V2 cards are allowed to see an SDHC host.
		for (;;) {
			if (!send_command_r1(APP_CMD, 0) && card_state.status != SD_STATUS_ILLEGAL_IDLE) {
				return false;
			}
			if (send_command_r1(SD_SEND_OP_COND, arg)) {
				break;
			} else if (card_state.status != SD_STATUS_ILLEGAL_IDLE) {
				return false;
			}
		}
		card_state.status = SD_STATUS_UNINITIALIZED;
	}

	// Determine card capacity class (SDSC vs SDHC/SDXC).
	if (v2) {
		uint32_t ocr;
		if (!send_command_r1_x32(READ_OCR, 0, &ocr)) {
			return false;
		}
		if (!(ocr & (UINT32_C(1) << 31))) {
			printf("SD: OCR=0x%" PRIX32 ", power up not done\n", ocr);
			card_state.status = SD_STATUS_ILLEGAL_RESPONSE;
			return false;
		}
		card_state.sdhc = !!(ocr & (UINT32_C(1) << 30));
	} else {
		card_state.sdhc = false;
	}
	printf("SD: SD%cC init OK\n", card_state.sdhc ? 'H' : 'S');

	// Set block length to 512 bytes (this is ignored in all the ways we care about for SDHC/SDXC, as they always use a 512 byte block length, but harmless).
	if (!send_command_r1(SET_BLOCKLEN, 512)) {
		return false;
	}

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
		if (!send_command_r1_data_read(SEND_CSD, 0, csd, sizeof(csd))) {
			return false;
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
	printf("SD: %" PRIu32 " sectors\n", card_state.sector_count);

	card_state.status = SD_STATUS_OK;
	card_state.write_multi_active = false;
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
		puts("SD: read during write multi");
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
		puts("SD: write multi start during write multi");
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
	return dma_running(DMA_READ_CHANNEL_SD);
}

bool sd_write_multi_sector(const void *data) {
	// Sanity check.
	if (card_state.status != SD_STATUS_OK) {
		return false;
	}
	if (!card_state.write_multi_active) {
		card_state.status = SD_STATUS_LOGICAL_ERROR;
		puts("SD: write multi sector outside write multi");
		return false;
	}
	if (sd_write_multi_busy()) {
		card_state.status = SD_STATUS_LOGICAL_ERROR;
		puts("SD: write multi sector while busy");
		return false;
	}
	uint8_t ctl = SD_CTL & 0b00111000;
	if (ctl) {
		if (ctl & 0b00100000) {
			card_state.status = SD_STATUS_ILLEGAL_RESPONSE;
			puts("SD: write multi saw malformed DRT");
		} else if (ctl & 0b00010000) {
			card_state.status = SD_STATUS_CARD_INTERNAL_ERROR;
			puts("SD: write multi saw I/O error in DRT");
		} else if (ctl & 0b00001000) {
			card_state.status = SD_STATUS_CRC_ERROR;
			puts("SD: write multi saw CRC error in DRT");
		}
		return false;
	}

	// Start the DMA transfer.
	dma_read_start(DMA_READ_CHANNEL_SD, data, 512);
	return true;
}

bool sd_write_multi_end(void) {
	// Sanity check.
	if (card_state.status != SD_STATUS_OK) {
		return false;
	}
	if (!card_state.write_multi_active) {
		card_state.status = SD_STATUS_LOGICAL_ERROR;
		puts("SD: write multi end outside write multi");
		return false;
	}

	// Wait for non-busy status.
	while (sd_write_multi_busy());
	while (SD_CTL & 1);

	// Check for any errors in the final sector.
	bool ret = true;
	uint8_t ctl = SD_CTL & 0b00111000;
	if (ctl) {
		if (ctl & 0b00100000) {
			card_state.status = SD_STATUS_ILLEGAL_RESPONSE;
			puts("SD: write multi saw malformed DRT");
		} else if (ctl & 0b00010000) {
			card_state.status = SD_STATUS_CARD_INTERNAL_ERROR;
			puts("SD: write multi saw I/O error in DRT");
		} else if (ctl & 0b00001000) {
			card_state.status = SD_STATUS_CRC_ERROR;
			puts("SD: write multi saw CRC error in DRT");
		}
		ret = false;
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

