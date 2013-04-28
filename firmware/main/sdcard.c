#include "sdcard.h"
#include "io.h"
#include <stddef.h>
#include <stdio.h>

#define START_BLOCK 0b11111110

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

#define LINE_IDLE_STATE 0xFF
uint8_t sd_receive_byte() {
	send_nop();
	return sd_read_byte();
}

uint8_t receive_DRT() {
	uint8_t byte;
	while ((byte = sd_receive_byte()) == LINE_IDLE_STATE);
	return byte;
}

void receive_R1() {
	uint8_t byte;
	while ((byte = sd_receive_byte()) == LINE_IDLE_STATE);
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
	return !!card_state.response;
}

bool voltage_check(void) {
	return (card_state.OCR & ((uint32_t)1 << 20) ) && (card_state.OCR & ((uint32_t)1 << 21));	
}

bool any_error_except_idle(void) {
	return (card_state.response & 0xFE);
}

bool send_command(sd_cmd_t cmd, uint32_t addr) {
	sd_assert_cs();
	uint8_t retries = 3;
	uint8_t send_data[6];
	send_data[0] = (uint8_t) cmd;
	send_data[0] = send_data[0] | 0x40;
	for(uint8_t i=0; i<4;++i) {
		send_data[i+1] = (addr >> (8*(3-i)))&0x000000FF;
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


bool send_command_checked(sd_cmd_t cmd, uint32_t addr) {
	if(!is_ready()) {
		return false;
	}
	return send_command(cmd,addr) && !any_error();
}

void close_transaction() {
	sd_deassert_cs();
	send_nop();
}

bool send_acommand(sd_acmd_t acmd, uint32_t addr) {
	if(send_command(APP_CMD,0)) {
		close_transaction();
		bool retval =!any_error_except_idle();
		return retval && send_command((sd_cmd_t) acmd, addr);
	}
	return false;
}

bool goto_idle_state() {
	bool retval = send_command(GO_IDLE_STATE,0) && !any_error_except_idle() && is_idle();
	close_transaction();
	return retval;
}

bool get_OCR_register() {
	send_command(SEND_STATUS,0);
	receive_R3();
	close_transaction();
	return !any_error_except_idle();
}


bool write_hello() {
	unsigned char test_string[] = "Hello World\n";
	
	//open transaction to write block to address zero and read R1	
	if(send_command_checked(WRITE_BLOCK, 0)) {

		sd_write_byte(START_BLOCK);
		for(uint16_t i=0;i<512;i++) {
			if(i < sizeof(test_string)) {
				sd_write_byte(test_string[i]);
			} else {
				sd_write_byte(0xFF);
			}
		}
		sd_write_byte(0x00); //CRC16 byte 1
		sd_write_byte(0x00); //CRC16 byte 2
	} else {
		puts("Send WRITE_SINGLE_BLOCK failed");
		return false;
	}
	uint8_t DRT;
	if(((DRT = receive_DRT())&0x0E) != 0x04) {
		printf("Received bad data response token: %02x\n",DRT);
		return false;
	}
	while(line_is_busy());
	close_transaction();
	return true;	
}

typedef enum {
	SEND_START,
	WRITE_BYTE,
	WRITE_CRC,
	RECEIVE_DRT,
	WAIT_BUSY,
	ERROR,
} sd_poll_state_t;

#define BUFFER_SIZE 1024
#define BLOCK_SIZE 512
#define NUM_HEADS 2

typedef struct {
	uint8_t data[BUFFER_SIZE];
	uint16_t head;
	uint16_t tail;
	uint8_t head_mask;
	uint16_t heads[2];
} sd_buffer_t;

sd_buffer_t multiwrite_buffer;

sd_poll_state_t poll_state;

void initCRC16() {
	//nop for now
}

void appendCRC16(uint8_t byte) {
	//add to crc 16
	//nop for now
}

void sendCRC16() {
	sd_write_byte(0x00);
	sd_write_byte(0x00);
}

bool send_next_byte() {
	if(multiwrite_buffer.head != multiwrite_buffer.tail || multiwrite_buffer.head_mask) {
		uint8_t shift=1;
		for(uint8_t i=0;i<NUM_HEADS;i++) {
			if(multiwrite_buffer.heads[i] == multiwrite_buffer.head && (multiwrite_buffer.head_mask & shift)) {
				multiwrite_buffer.head_mask &= ~shift;
			}
		}
		sd_write_byte(multiwrite_buffer.data[multiwrite_buffer.head]);
		appendCRC16(multiwrite_buffer.data[multiwrite_buffer.head]);
		multiwrite_buffer.head += 1;
		multiwrite_buffer.head = (multiwrite_buffer.head)%BUFFER_SIZE;
		return true;
	}
	return false;
}

sd_poll_error_t sd_poll() {
	uint8_t data;
	switch(poll_state) {
		case SEND_START:
			sd_write_byte(START_BLOCK);
			initCRC16();
			poll_state = WRITE_BYTE;
			break;
		case WRITE_BYTE:
			if(send_next_byte()) {
				poll_state = WRITE_CRC;
			}
			break;
		case WRITE_CRC:
				sendCRC16();
				poll_state = RECEIVE_DRT; 
			break;
		case RECEIVE_DRT:
			data = sd_receive_byte();
			if(data != LINE_IDLE_STATE) {
				switch(data&0x1F) {
					case 0x05:
						poll_state = WAIT_BUSY;
						return SD_POLL_SUCCESS;
					case 0x0B:
						poll_state = ERROR;
						return SD_CRC_ERROR;
					case 0x0D:
						poll_state = ERROR;
						return SD_WRITE_ERROR;
				}
			}
			break;
		case WAIT_BUSY:
			if(!line_is_busy()) {
				poll_state = SEND_START;
			}
			break;
		case ERROR:
			return SD_PREVIOUS_ERROR;
		default:
			return SD_POLL_UNKNOWN;
	}
	return SD_POLL_SUCCESS;
}


uint16_t sd_mulitwrite_available_buffer_space() {
	uint16_t length = BUFFER_SIZE - ((multiwrite_buffer.tail-multiwrite_buffer.head)%BUFFER_SIZE);

	if(length == BUFFER_SIZE && (multiwrite_buffer.head_mask)) {
		return 0;
	}

	return length;
}


void buffer_push(uint8_t *data, uint16_t length) {
	for(uint16_t i=0;i<length; ++i) {
		multiwrite_buffer.data[multiwrite_buffer.tail]=data[i];
		if(!((multiwrite_buffer.tail - multiwrite_buffer.head)%BLOCK_SIZE)) {
			multiwrite_buffer.head_mask <<= 1;
			for(uint8_t j=(NUM_HEADS-1);j!=0;--j) {
				multiwrite_buffer.heads[j] = multiwrite_buffer.heads[j-1];
			}
			multiwrite_buffer.head_mask |= 0x01;
			multiwrite_buffer.heads[0] = multiwrite_buffer.tail;
		}
		multiwrite_buffer.tail += 1;
		multiwrite_buffer.tail = (multiwrite_buffer.tail)%BUFFER_SIZE;
	}
}

void collapse_buffer() {
	if(multiwrite_buffer.heads[NUM_HEADS-1] != multiwrite_buffer.head) {
		//need to copy data from tail block to tailblock -1;
		for(uint16_t i=0;multiwrite_buffer.heads[0]+i < multiwrite_buffer.tail; ++i) {
			multiwrite_buffer.data[multiwrite_buffer.heads[1]+i] =multiwrite_buffer.data[multiwrite_buffer.heads[0]+i]; 
		}
	}
	
	multiwrite_buffer.tail = multiwrite_buffer.heads[0];
	for(uint8_t i=0;i < (NUM_HEADS -1); ++i) {
		multiwrite_buffer.heads[i] = multiwrite_buffer.heads[i+1];
	}
	
	multiwrite_buffer.head_mask >>=1;
}

bool sd_mulitwrite_push_data(uint8_t *data, uint16_t length) {
	uint16_t available_space = sd_multiwrite_available_buffer_space();
	if(length > available_space) {
		buffer_push(data,available_space);
		data = data + available_space;
		length = length - available_space;
	} else {
		buffer_push(data,length);
		data = data + length;
		length = length - length;
	}

	if(length) {
		collapse_buffer();
		sd_multiwrite_push_data(data,length);
		return false;
	}
	return true;
}

void sd_multiwrite_finalize() {
	uint8_t stuff_byte = 0x00;
	while(poll_state != SEND_START) {
		sd_multiwrite_push_data(&stuff_byte,1);
		sd_poll();
	};
	close_transaction();
}

bool sd_multiwrite_open(uint32_t addr) {
	//if standard card shift address here;
	bool retval = send_command_checked(WRITE_MULTIPLE_BLOCK,addr);
	poll_state = SEND_START;
	multiwrite_buffer.head_mask=0x00;
	multiwrite_buffer.head=0;
	multiwrite_buffer.tail=0;
	multiwrite_buffer.heads[0]=0;
	multiwrite_buffer.heads[1]=0;
	return retval;
}

bool sd_init_card(bool enable_CRC) {
	sd_reset_state();
	if(!is_sd_present()) {
		puts("Card not detected.");
		return false;
	} else {
		puts("Card detected for init.");
	}

	sd_deassert_cs();
	for(uint8_t i=0;i<80;i++) {
		send_nop();	
	}
	
	if(!goto_idle_state()) {
		puts("Failed to enter idle state");
		return false;
	} else {
		puts("Successfully returned to idle state.");
	}

	uint32_t if_cond_args = 0x00000142;
	send_command(SEND_IF_COND,if_cond_args);
	if(is_illegal_cmd()) {
		card_state.version = VERSION_1_x;
		puts("possible VERSION 1_x card detected.");
	} else {
		if(any_error_except_idle()) {
			puts("Fatal error getting interface condition.");
			return false;
		}
		card_state.version = VERSION_2_later;
		puts("possible VERSION 2_later card detected.");
		send_nop(); //command version and reserved bits ignore these
		send_nop(); //reserved bits ignore these
		send_nop(); //reserved bits and voltage accepted
		uint8_t voltage = (sd_read_byte() & 0x0F);
		send_nop(); //check pattern
		uint8_t check = sd_read_byte();
		if((check != 0x42) || (voltage != 0x01)) {
			puts("Fatal error getting interface condition.");
			return false;
		} else {
			puts("Iterface condition checks out.");
		}
	}
	close_transaction();

	if(!get_OCR_register()) {
		puts("Failed to initialize Card.");
		return false;
	} else {
		puts("Retrieved OCR register.");
	}

	if(!voltage_check()) {
		puts("Card voltage not compatible.");
		return false;
	} else {
		puts("Card voltage appears compatible.");	
	}

	if(enable_CRC) {
		card_state.crc_enabled = true;
		uint32_t crc_args = 0x00000001;
		if(!send_command(CRC_ON_OFF,crc_args) || any_error_except_idle()) {
			puts("Failed to enable CRC.");
			return false;
		}
		close_transaction();
	}

	uint32_t op_cond_args =0;
	if(card_state.version == VERSION_2_later) {
		op_cond_args |= 0x00000040;
	}
	puts("Entering init wait.");
	do {
		send_acommand(SD_SEND_OP_COND,op_cond_args);
		close_transaction();
	} while(is_idle() && !any_error_except_idle());
	puts("Card no longer idle.");

	if(any_error_except_idle()) {
		puts("Failed to initialize Card.");
		return false;
	}

	do {
		if(!get_OCR_register()) {
			puts("Failed to get OCR register during init.");
			return false;
		} else {
			puts("read OCR register.");
		}
	} while(!(card_state.OCR&0x8000));

	if(card_state.OCR&0x4000 && card_state.version == VERSION_2_later) {
		card_state.capacity = CAPACITY_HC;
		puts("Card is high or extended capacity.");
	} else {
		card_state.capacity = CAPACITY_SD;
		puts("Card is standard capacity");
	}

	card_state.enabled=true;
	
	return true;
}
