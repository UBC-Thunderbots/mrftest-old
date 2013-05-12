#include "sdcard.h"
#include "io.h"
#include <stddef.h>
#include <stdio.h>

#define START_BLOCK 0b11111110
#define START_BLOCK_MULTI 0b11111100
#define STOP_TRAN 0b11111101

bool sd_write_busy(void) {
	return !!(SD_CTL&0x01);
}

uint8_t sd_read_byte(void) {
	while(sd_write_busy());
	return SD_DATA;
}

void sd_write_byte(uint8_t byte) {
	while(sd_write_busy());
	SD_DATA = byte;
}


#define LINE_IDLE_STATE 0xFF

void send_nop() {
	sd_write_byte(LINE_IDLE_STATE);
}

uint8_t sd_receive_byte() {
	send_nop();
	return sd_read_byte();
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


//The states of the multiblock write poll
typedef enum {
	SEND_START, //send the start block token
	WRITE_BYTE, //write a byte of data
	WRITE_CRC, //send the two CRC bytes
	RECEIVE_DRT, //Receive the block Data Response Token
	WAIT_BUSY, //Wait for the card to be non-busy
	ERROR, //stalls here if any error occurs
} sd_poll_state_t;


//we should have a buffer at least two blocks long
#define BUFFER_SIZE 1024
#define BLOCK_SIZE 512

//we need some pointers to track the block beginnings ceil(BUFFER_SIZE/BLOCK_SIZE)
#define NUM_HEADS 2


//The data buffer singlton
typedef struct {
	uint8_t data[BUFFER_SIZE];
	uint16_t head;
	uint16_t tail;
	uint8_t head_mask;
	uint16_t heads[2];
	uint16_t byte_index;
} sd_buffer_t;

sd_buffer_t multiwrite_buffer;

sd_poll_state_t poll_state;

void initCRC16() {
	//do what is needed to start a crc16
	//nop for now
}

void appendCRC16(uint8_t byte) {
	//add byte to crc 16
	//nop for now
}

void sendCRC16() {
	//retrieve the computed crc16 and send
	//send garbage for now
	sd_write_byte(0x00);
	sd_write_byte(0x00);
}

bool send_next_byte() {
	//if there is data to be sent
	if(multiwrite_buffer.head != multiwrite_buffer.tail || multiwrite_buffer.head_mask) {
		//walk the heads array and remove any that match the current head
		uint8_t shift=1;
		for(uint8_t i=0;i<NUM_HEADS;i++) {
			if(multiwrite_buffer.heads[i] == multiwrite_buffer.head && (multiwrite_buffer.head_mask & shift)) {
				multiwrite_buffer.head_mask &= ~shift;
			}
			shift <<= 1;
		}

		//send the data to the card and increment head
		sd_write_byte(multiwrite_buffer.data[multiwrite_buffer.head]);
		multiwrite_buffer.byte_index += 1;
		multiwrite_buffer.byte_index = (multiwrite_buffer.byte_index)%BLOCK_SIZE;

		//use the byte in the crc16 
		appendCRC16(multiwrite_buffer.data[multiwrite_buffer.head]);

		//increment head with wrap around
		multiwrite_buffer.head += 1;
		multiwrite_buffer.head = (multiwrite_buffer.head)%BUFFER_SIZE;

		//return if this is the end of a block
		return !!(multiwrite_buffer.byte_index == 0);
	}
	return false;
}

sd_poll_error_t sd_poll(void) {
	uint8_t data;
	switch(poll_state) {
		case SEND_START:
			//send the start token and init the crc
			send_nop();
			sd_write_byte(START_BLOCK_MULTI);
			initCRC16();
			poll_state = WRITE_BYTE;
			break;
		case WRITE_BYTE:
			//we sit here until a block is written
			if(send_next_byte()) {
				poll_state = WRITE_CRC;
			}
			break;
		case WRITE_CRC:
				//send both bytes of the crc
				sendCRC16();
				poll_state = RECEIVE_DRT; 
			break;
		case RECEIVE_DRT:
			data = sd_receive_byte();
			if(data != LINE_IDLE_STATE) {
				switch(data&0x1F) {
					case 0x05:
						// good reponse so carry on
						poll_state = WAIT_BUSY;
						return SD_POLL_SUCCESS;
						//return the various error codes and lock in an error
					case 0x0B:
						//There was a block CRC error
						poll_state = ERROR;
						return SD_CRC_ERROR;
					case 0x0D:
						//There was a genric write error, should check card status and number of well written blocks
						poll_state = ERROR;
						return SD_WRITE_ERROR;
					default:
						//Something dun got fucked up good
						poll_state = ERROR;
						return SD_DRT_ERROR;
				}
			}
			break;
		case WAIT_BUSY:
			//stay here while the card writes the block
			if(!line_is_busy()) {
				poll_state = SEND_START;
			}
			break;
		case ERROR:
			return SD_PREVIOUS_ERROR;
		default:
			//this should never happen but we've seen stranger bugs
			return SD_POLL_UNKNOWN;
	}
	return SD_POLL_SUCCESS;
}

//I want the available buffer space for data
uint16_t sd_multiwrite_available_buffer_space() {
	uint16_t length = BUFFER_SIZE - ((multiwrite_buffer.tail-multiwrite_buffer.head)%BUFFER_SIZE);

	if(length == BUFFER_SIZE && (multiwrite_buffer.head_mask)) {
		return 0;
	}

	return length;
}


//just write data into the buffer taking care of head flagging
//overflows are handled elsewhere
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

//if a write will overflow the buffer we need to destroy data
//so take the last block written to the buffer but not to the card and clobber it
//write in it's place the partial data currently in the buffer this ensures only whole blocks are written
void collapse_buffer() {
	//the head does not align with the edge of the buffer, so copy the partial data
	//this expoits the fact that the buffer is a even multiple of blocks and no copy is needed if the first head aligns with the buffer head
	//heads should always be sequential with earliest head at greatest index and later head (closer to tail) at index 0;
	if(multiwrite_buffer.heads[NUM_HEADS-1] != multiwrite_buffer.head) {
		//need to copy data from tail block to tailblock -1;
		for(uint16_t i=0;multiwrite_buffer.heads[0]+i < multiwrite_buffer.tail; ++i) {
			multiwrite_buffer.data[multiwrite_buffer.heads[1]+i] =multiwrite_buffer.data[multiwrite_buffer.heads[0]+i]; 
		}
	}
	
	// reset the tail clobber the block but include the copied data
	multiwrite_buffer.tail = multiwrite_buffer.heads[1] + multiwrite_buffer.tail - multiwrite_buffer.heads[0];
	
	//we removed a head so handle it
	for(uint8_t i=0;i < (NUM_HEADS -1); ++i) {
		multiwrite_buffer.heads[i] = multiwrite_buffer.heads[i+1];
	}
	
	multiwrite_buffer.head_mask >>=1;
}

bool sd_multiwrite_push_data(uint8_t *data, uint16_t length) {
	uint16_t available_space = sd_multiwrite_available_buffer_space();
	if(length > available_space) {

		//if there isn't enough space, push what you can then clober the buffer and recurse on remaining data
		buffer_push(data,available_space);
		data = data + available_space;
		length = length - available_space;
		collapse_buffer();
		sd_multiwrite_push_data(data,length);
		//things were clobbered so flag it
		return false;
	} else {

		//just a normal push because there is room
		buffer_push(data,length);
		data = data + length;
		length = 0;
		return true;
	}
}

bool sd_multiwrite_finalize() {
	uint8_t stuff_byte = 0x42;
	//until we are ready to send another start token
	//push a null byte into the buffer and run poll.
	do {
		//If the buffer is empty push a stuffing byte
		if(sd_multiwrite_available_buffer_space() == BUFFER_SIZE) {
			sd_multiwrite_push_data(&stuff_byte,1);
		}

		sd_poll_error_t error_code;
		if((error_code = sd_poll()) != SD_POLL_SUCCESS) {
			printf("SD error code is: %d\n",(int) error_code);
		}
	} while(poll_state != SEND_START && poll_state != ERROR);

	//Send the Stoptoken insteand of block start token
	sd_write_byte(STOP_TRAN);
	send_nop(); //WARNING: There is one byte of delay between the stop transmission and the asertion of busy
	while(line_is_busy()); //Let the write finish
	
	//deassert CS and clock out 8 bits to finish things
	close_transaction();
	return true;
}


//Init some variables and open a multiblock write in the card
bool sd_multiwrite_open(uint32_t addr) {
	//if standard card shift address here;
#warning making the assumption that addr is the block addr
	bool retval = send_command_checked(WRITE_MULTIPLE_BLOCK,addr);
	poll_state = SEND_START;
	multiwrite_buffer.head_mask=0x00;
	multiwrite_buffer.head=0;
	multiwrite_buffer.tail=0;
	multiwrite_buffer.heads[0]=0;
	multiwrite_buffer.heads[1]=0;
	multiwrite_buffer.byte_index=0;
	for(uint16_t i=0;i<BUFFER_SIZE; ++i) {
		multiwrite_buffer.data[i] = 0x24;
	}
	return retval;
}

//Initializes the SD Card
bool sd_init_card(bool enable_CRC) {
	//clear the current sdcard state context
	sd_reset_state();

	//First check the present line is active, if not do nothing
	if(!is_sd_present()) {
		puts("SD: Card not detected.");
		return false;
	} else {
		puts("SD: Card detected for init.");
	}

	//send a bunch of clocks to insure the card is booted up 
	sd_deassert_cs();
	for(uint8_t i=0;i<80;i++) {
		send_nop();	
	}
	
	//Send the idle state command
	if(!goto_idle_state()) {
		puts("SD: Failed to enter idle state");
		return false;
	} else {
		puts("SD: Successfully returned to idle state.");
	}


	//Follow the starup procedure of checking voltages and such
	uint32_t if_cond_args = 0x00000142;
	send_command(SEND_IF_COND,if_cond_args);
	if(is_illegal_cmd()) {
		card_state.version = VERSION_1_x;
		puts("SD: possible VERSION 1_x card detected.");
	} else {
		if(any_error_except_idle()) {
			puts("SD: Fatal error getting interface condition.");
			return false;
		}
		card_state.version = VERSION_2_later;
		puts("SD: possible VERSION 2_later card detected.");
		send_nop(); //command version and reserved bits ignore these
		send_nop(); //reserved bits ignore these
		send_nop(); //reserved bits and voltage accepted
		uint8_t voltage = (sd_read_byte() & 0x0F);
		send_nop(); //check pattern
		uint8_t check = sd_read_byte();
		if((check != 0x42) || (voltage != 0x01)) {
			puts("SD: Fatal error getting interface condition.");
			return false;
		} else {
			puts("SD: Iterface condition checks out.");
		}
	}
	close_transaction();

	//retrieve the OCR register, has useful status about capacity and such
	if(!get_OCR_register()) {
		puts("SD: Failed to initialize Card.");
		return false;
	} else {
		puts("SD: Retrieved OCR register.");
	}

	if(!voltage_check()) {
		puts("SD: Card voltage not compatible.");
		return false;
	} else {
		puts("SD: Card voltage appears compatible.");	
	}

	//Turn on CRC checking if we want it
	if(enable_CRC) {
		card_state.crc_enabled = true;
		uint32_t crc_args = 0x00000001;
		if(!send_command(CRC_ON_OFF,crc_args) || any_error_except_idle()) {
			puts("Failed to enable CRC.");
			return false;
		}
		close_transaction();
	}

	//If card supports V2 then send that we support SDHC
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

	//get the OCR and wait on power up status busy
	do {
		if(!get_OCR_register()) {
			puts("Failed to get OCR register during init.");
			return false;
		} else {
			puts("read OCR register.");
		}
	} while(!(card_state.OCR&0x8000));

	//Check final card capacity of OCR
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
