#ifndef SDCARD_H_
#define SDCARD_H_

#include <stdbool.h>
#include <stdint.h>

//For all addresses, SDSC uses byte addresses, SDHC or SDXC use block addresses
typedef enum {
	GO_IDLE_STATE=0, //(no params return R1) resets the SD memory card
	SEND_OP_COND=1, //(30: HCS host supports HC(1) return R1) set host capacity support and init card (only valid after soft reset)
	SWITCH_FUNC=6,  // (see docs return R1)checks switchable function and switches card function
	SEND_IF_COND=8, // (11:8 supply voltage 7:0 check pattern return R7) sends card interface condition
	SEND_CSD=9,     //( no params return R1) asks the card to send its card-specific data (CSD)
	SEND_CID=10,    //(no params return R1) asks the card to send its card identification (CID)
	STOP_TRANSMISSION=12, //(no params return R1b) forces the card to stop multiblock read
	SEND_STATUS=13, //(no params return R2)
	SET_BLOCKLEN=16, //(31:0 block length return R1) length of lock_unlock, for SDSC sets block length (others block length is fixed 512)
	READ_SINGLE_BLOCK=17, //(31:0 data address return R1) reads a block of block length
	READ_MULTIPLE_BLOCK=18, //(31:0 data address return R1) continuously transfers data blocks until stopped
	WRITE_BLOCK=24,         //(31:0 data address return R1) write a block of length block length
	WRITE_MULTIPLE_BLOCK=25, //(31:0 data address return R1) continuously write blocks until stopped
	PROGRAM_CSD=27, //(no params return R1) programming the bits of CSD
	SET_WRITE_PROT=28, //(31:0 data address return R1b) write protect on for WP_GRP_SIZE fo CSD data, not available on SDHC or SDXC (is optional for SDSC)
	CLR_WRITE_PROT=29, //(31:0 data address return R1b) clears write protect of above, not available on SDHC or SDXC (is optional for SDSC)
	SEND_WRITE_PROT=30, //(31:0 write protect data address return R1) if the card has write protect features returns the value not available on SDHC or SDXC (is optional for SDSC)
	ERASE_WR_BLK_START_ADDR=32, //(31:0 data address return R1) sets the address of the first block ot be erased
	ERASE_WR_BLK_END_ADDR=33, //(31:0 data address return R1) sets the address of the last block to be erased
	ERASE=38, //(no params returns R1b) Erases all previously selected blocks
	LOCK_UNLOCK=42, //(reserved set to 0, return R1) set/reset password or lock/unlock card refer to docs
	APP_CMD=55, //(no params return R1) next command will be application specific instead of standard
	GEN_CMD=56, //(0 : read(0) write(1)) used to transfer or retrieve a block for commands, size is block length.
	READ_OCR=58, // (no params return R3) reads the OCR register (CCS is in this register)
	CRC_ON_OFF=59, //(0 : CRC on (1)) turns the CRC on or off defaults off
} sd_cmd_t;

typedef enum {
	SD_STATUS=13, // (no params) Send the SD status
	UNKNOWN1=18, // reserved for security
	SEND_NUM_WR_BLOCKS=22, //(no params) Send the number of well written (no error) blocks
	SET_WR_BLK_ERASE_COUNT=23, //(22:0 num of blocks) set the number of write blocks to be pre erased when writing
	UNKNOWN2=25, // reserved for security
	UNKNOWN3=26, // reserved for security
	UNKNOWN4=38, // reserved for security
	SD_SEND_OP_COND=41, //(30 : HCS host supports HC(1) bit) sends host capacity support
	SET_CLR_CARD_DETECT=42, //(0 set_cd) connect(1)/disconnect(0) the 50k pull up on CS.
	SEND_SCR=51, // (no params) read the sd configuration register (SCR)
} sd_acmd_t;

typedef enum {
	SD_POLL_SUCCESS,
	SD_POLL_UNKNOWN,
	SD_PREVIOUS_ERROR,
	SD_CRC_ERROR,
	SD_DRT_ERROR,
	SD_WRITE_ERROR,
} sd_poll_error_t;

//allows multi block writes to keep chugging along.
sd_poll_error_t sd_poll();

//Initialize the card for operations. 
//Performs and version 2 init so we can support SDHC
bool sd_init_card(bool enable_CRC);

//Start a multiblock write at addr.
//Does not do address conversion when dealing with SDSC VS SDHC (byte vs block)
bool sd_multiwrite_open(uint32_t addr);

//finish writing the last block with 0x42
bool sd_multiwrite_finalize();


//add data to the write queue.
bool sd_multiwrite_push_data(uint8_t *data, uint16_t length);


//gets space left in write buffer
uint16_t sd_multiwrite_available_buffer_space();
#endif
