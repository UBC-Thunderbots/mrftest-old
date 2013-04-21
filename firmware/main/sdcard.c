#include "sdcard.h"
#include "io.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

bool sd_write_busy(void) {
	return !!(SD_CTL&0x01);
}

void sd_write_byte(uint8_t byte) {
	while(sd_write_busy());
	SD_DATA = byte;
}

uint8_t sd_read_byte(void) {
	while(sd_write_busy());
	return SD_DATA;
}

void sd_deassert_cs() {
	SD_CTL = SD_CTL|0x04;
}

void sd_assert_cs() {
	SD_CTL = SD_CTL&(~0x04);
}

bool is_sd_present() {
	return !!(SD_CTL&0x02);
}

void send_nop() {
	sd_write_byte(0xFF);
}

bool line_is_busy() {
	send_nop();
	return (sd_read_byte() == 0x00);
}

uint8_t CRC7(uint8_t *chr, size_t cnt) {
	size_t i,a;
	uint8_t crc,Data;

	crc=0;
	for (a=0;a<cnt;a++) {
		Data=chr[a];
		for (i=0;i<8;i++) {
			crc <<= 1;

			if ((Data & 0x80)^(crc & 0x80)) {
				crc ^=0x09;
			}
			Data <<= 1;	
		}
	}
	crc=(crc<<1)|1;
	return(crc);
}

typedef enum {
	VERSION_UNKNOWN, //reset value card needs to init
	VERSION_1_x, //card is version 1.x
	VERSION_2_later, //card is version 2.00 or later
} sd_version_t;

typedef enum {
	CAPACITY_UNKNOWN,
	CAPACITY_SD,
	CAPACITY_HC,
} sd_capacity_t;

typedef struct {
	sd_version_t version;
	sd_capacity_t capacity;
	size_t block_size;
	uint8_t response;
	uint8_t status;
	uint32_t OCR;
	bool crc_enabled;
	bool enabled;
} sd_ctx_t;

sd_ctx_t card_state;

void sd_reset_state() {
	card_state.version = VERSION_UNKNOWN;
	card_state.capacity = CAPACITY_UNKNOWN;
	card_state.block_size = 512;
	card_state.response = 0;
	card_state.status = 0;
	card_state.OCR = 0;
	card_state.crc_enabled = false;
	card_state.enabled = false;
}

void receive_R1() {
	uint8_t byte;
	do {
		send_nop();
		} while ((byte = sd_read_byte()) == 0xFF);
	card_state.response = byte;
}

void receive_R2() {
	send_nop();
	card_state.status = sd_read_byte();
}

void receive_R3() {
	card_state.OCR=0;
	for(uint8_t i=0;i<4;++i) {
		send_nop();
		card_state.OCR |= ((uint32_t) sd_read_byte())<< (8*(3-i));
	}
}

bool is_ready(void) {
	if(card_state.enabled) {
		if(is_sd_present()) {
			return true;
		}
		card_state.enabled = false;
	}
	return false;
}

bool is_idle(void) {
	return (card_state.response & 0x01);
}

bool is_illegal_cmd(void) {
	return (card_state.response & 0x04);
}

bool CRC7_error(void) {
	return (card_state.response & 0x08);
}

bool any_error(void) {
	return card_state.response;
}

bool voltage_check(void) {
	return (card_state.OCR & ((uint32_t)1 << 20) ) && (card_state.OCR & ((uint32_t)1 << 21));	
}

bool any_error_except_idle(void) {
	return (card_state.response & 0xFE);
}

bool send_command(sd_cmd_t cmd, uint8_t args[4]) {
	sd_assert_cs();
	uint8_t retries = 3;
	uint8_t send_data[6];
	send_data[0] = (uint8_t) cmd;
	send_data[0] = send_data[0] | 0x40;
	for(uint8_t i=0; i<4;++i) {
		send_data[i+1] = args[i];
	}
	send_data[5] = CRC7(send_data,5);
	do {
		for(uint8_t i=0;i<6;++i) {
			sd_write_byte(send_data[i]);
		}
		receive_R1();
	} while(CRC7_error() && --retries);
	return !!retries;
}


bool send_command_checked(sd_cmd_t cmd, uint8_t args[4]) {
	if(!is_ready()) {
		return false;
	}
	return send_command(cmd,args) && !any_error();
}

void close_transaction() {
	sd_deassert_cs();
	send_nop();
}

bool send_acommand(sd_acmd_t acmd, uint8_t args[4]) {
	uint8_t fake_args[] = {0x00, 0x00, 0x00, 0x00};
	if(send_command(APP_CMD,fake_args)) {
		close_transaction();
		bool retval =!any_error_except_idle();
		return retval && send_command((sd_cmd_t) acmd, args);
	}
	return false;
}

bool goto_idle_state() {
	uint8_t temp_args[]={0x00, 0x00, 0x00,0x00};
	bool retval = send_command(GO_IDLE_STATE,temp_args) && !any_error_except_idle() && is_idle();
	close_transaction();
	return retval;
}

bool get_OCR_register() {
	uint8_t temp_args[]={0x00, 0x00, 0x00, 0x00};
	send_command(SEND_STATUS,temp_args);
	receive_R3();
	close_transaction();
	return !any_error_except_idle();
}


bool write_hello() {
	unsigned char test_string[] = "Hello World\n";
	return false;	
}


bool sd_init_card(bool enable_CRC) {
	sd_reset_state();
	if(!is_sd_present()) {
		printf("Card not detected.\n");
		return false;
	} else {
		printf("Card detected for init.\n");
	}

	sd_deassert_cs();
	for(uint8_t i=0;i<80;i++) {
		send_nop();	
	}
	
	if(!goto_idle_state()) {
		printf("Failed to enter idle state");
		return false;
	} else {
		printf("Successfully returned to idle state.\n");
	}

	uint8_t if_cond_args[]={0x00, 0x00, 0x01, 0x42};
	send_command(SEND_IF_COND,if_cond_args);
	if(is_illegal_cmd()) {
		card_state.version = VERSION_1_x;
		printf("possible VERSION 1_x card detected.\n");
	} else {
		if(any_error_except_idle()) {
			printf("Fatal error getting interface condition\n");
			return false;
		}
		card_state.version = VERSION_2_later;
		printf("possible VERSION 2_later card detected.\n");
		send_nop(); //command version and reserved bits ignore these
		send_nop(); //reserved bits ignore these
		send_nop(); //reserved bits and voltage accepted
		uint8_t voltage = (sd_read_byte() & 0x0F);
		send_nop(); //check pattern
		uint8_t check = sd_read_byte();
		if((check != 0x42) || (voltage != 0x01)) {
			printf("Fatal error getting interface condition\n");
			return false;
		} else {
			printf("Iterface condition checks out\n");
		}
	}
	close_transaction();

	if(!get_OCR_register()) {
		printf("Failed to initialize Card\n");
		return false;
	} else {
		printf("Retrieved OCR register\n");
	}

	if(!voltage_check()) {
		printf("Card voltage not compatible\n");
		return false;
	} else {
		printf("Card voltage appears compatible.\n");	
	}

	if(enable_CRC) {
		card_state.crc_enabled = true;
		uint8_t crc_args[] = {0x00, 0x00, 0x00, 0x01};
		if(!send_command(CRC_ON_OFF,crc_args) || any_error_except_idle()) {
			printf("Failed to enable CRC\n");
			return false;
		}
		close_transaction();
	}

	uint8_t op_cond_args[] = {0x00, 0x00, 0x00, 0x00};
	if(card_state.version == VERSION_2_later) {
		op_cond_args[0] = 0x40;
	}
	printf("Entering init wait.\n");
	do {
		send_acommand(SD_SEND_OP_COND,op_cond_args);
		close_transaction();
	} while(is_idle() && !any_error_except_idle());
	printf("Card no longer idle.\n");

	if(any_error_except_idle()) {
		printf("Failed to initialize Card\n");
		return false;
	}

	do {
		if(!get_OCR_register()) {
			printf("Failed to get OCR register during init\n");
			return false;
		} else {
			printf("read OCR register\n");
		}
	} while(!(card_state.OCR&0x8000));

	if(card_state.OCR&0x4000 && card_state.version == VERSION_2_later) {
		card_state.capacity = CAPACITY_HC;
		printf("Card is high or extended capacity\n");
	} else {
		card_state.capacity = CAPACITY_SD;
		printf("Card is standard capacity");
	}

	card_state.enabled=true;
	
	return true;
}
