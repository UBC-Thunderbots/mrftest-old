#ifndef SDCARD_H
#define SDCARD_H

#include <stdbool.h>
#include <stdint.h>

/**
 * \brief The possible results of taking an action against the card.
 */
typedef enum {
	/**
	 * \brief The action completed successfully.
	 */
	SD_STATUS_OK,

	/**
	 * \brief The action failed because the card has not been initialized.
	 */
	SD_STATUS_UNINITIALIZED,

	/**
	 * \brief The action failed because no card is plugged in.
	 */
	SD_STATUS_NO_CARD,

	/**
	 * \brief The action failed because the attached card is working properly but is incompatible with this system.
	 */
	SD_STATUS_INCOMPATIBLE_CARD,

	/**
	 * \brief The action failed because the card sent an illegal response.
	 */
	SD_STATUS_ILLEGAL_RESPONSE,

	/**
	 * \brief The action failed because a logical error occurred (e.g. invalid parameter or address value or command sequence error).
	 */
	SD_STATUS_LOGICAL_ERROR,

	/**
	 * \brief The action failed because a CRC failed.
	 */
	SD_STATUS_CRC_ERROR,

	/**
	 * \brief The action failed because a command was sent that the card did not recognize.
	 */
	SD_STATUS_ILLEGAL_COMMAND,

	/**
	 * \brief The card was in an unexpected state after the command is run (?)
	 */
	SD_STATUS_ILLEGAL_STATE,

	/**
	 * \brief The card encountered an internal error in its controller or uncorrectable corruption in its memory array.
	 */
	SD_STATUS_CARD_INTERNAL_ERROR,

	/** NEW for STM32
	 * \brief Command response timeout
	 */
	SD_STATUS_COMMAND_RESPONSE_TIMEOUT, 

	/** NEW for STM32
	 * \brief The command's argument was out of the allowed range for this card. 
	 */
	SD_STATUS_OUT_OF_RANGE,

	/**
	 * \brief The commands address argument positions the first data block misaligned to the card physical blocks. 
	 */
	SD_STATUS_ADDRESS_MISALIGN,

	/* 
	 * brief Either the argument of a SET_BLOCKLEN command exceeds the maximum value allowed for the card, or the previously defined blck length is illegal for the current coommand
	 */
	SD_STATUS_BLOCK_LEN_ERROR, 

	/*
	 * brief An error in the sequence of erase commands occurred
	 */
	SD_STATUS_ERASE_SEQ_ERROR,

	/*
	 * brief An invalid selectioin of erase groups for erase occurred
	 */
	SD_STATUS_ERASE_PARAM, 

	/*
	 * brief Attempt to program a write-protected block
	 */
	SD_STATUS_WP_VIOLATION,

	/*
	 * brief When set, signals that the card is locked by the host
	 */
	SD_STATUS_CARD_IS_LOCKED,

	/*
	 * brief Set when a sequence or password error has been detected in lock/unlock card command
	 */
	SD_STATUS_LOCK_UNLOCK_FAILED,

	/*
	 * brief The CRC check of the previous command failed
	 */
	SD_STATUS_COM_CRC_ERROR,

	/*
	 * brief Command not legal for the card state
	 */
	SD_STATUS_ILLEGAL_COMMAND, 

	/*
	 * brief Card internal ECC was applied but failed to correct the data
	 */
	SD_STATUS_CARD_ECC_FAILED, 

	/*
	 * brief A card error occurred, which is not related to the host command
	 */
	SD_STATUS_CC_ERROR, 

	/*
	 * brief Generic card error related to the ( and detected during) execution of the last host command.
	 */
	SD_STATUS_ERROR, 

	/*
	 * brief CSD write error
	 */
	SD_STATUS_CSD_OVERWRITE, 

	/*
	 * brief Set when only partial address space was erased due to existing write
	 */
	SD_STATUS_WP_ERASE_SKIP,

	/* 
	 * brief Command has been executed without using the internal ECC
	 */
	SD_STATUS_CARD_ECC_DISABLED, 

	/*
	 * brief Erase sequence was cleared before executing because an out of erase sequence command was received.
	 */
	SD_STATUS_ERASE_RESET, 

	/*
	 * brief Corresponds to buffer empty signalling on the bus
	 */
	SD_STATUS_READY_FOR_DATA, 

	/* 
	 * brief Error in the sequence of the authentication process
	 */
	SD_STATUS_AKE_SEQ_ERROR

} sd_status_t;

/**
 * \brief Returns the current status of the SD card.
 *
 * \return the current status, including the last error if an operation failed
 */
sd_status_t sd_status(void);

/**
 * \brief Initializes the SD card.
 *
 * \return \c true on success, or \c false on failure
 */
bool sd_init(void);

/**
 * \brief Returns the number of sectors on the SD card.
 *
 * \return the number of sectors, or 0 on failure
 */
uint32_t sd_sector_count(void);


/**
 * \brief Reads a block from the SD card. 
 * 
 * \param Unclear yet. 
 *
 * \return \c true on success, or \c false on failure
 */
bool read_block (uint32_t * );

/**
 * \brief Writes a block to the SD card. 
 * 
 * \param Unclear yet. 
 * 
 * \return \c true on success, or \c false on failure
 */ 
bool write_block (uint32_t *);

/**
 * \brief Reads a sector from the SD card.
 *
 * \param sector the sector to read
 *
 * \param buffer a 512-byte buffer in which to store the sector data
 *
 * \return \c true on success, or \c false on failure
 */
bool sd_read(uint32_t sector, void *buffer);

/**
 * \brief Checks whether a multi-sector write is currently active.
 *
 * \return \c true if a multi-sector write is active, or \c false if not
 */
bool sd_write_multi_active(void);

/**
 * \brief Starts a multi-sector write operation to the SD card.
 *
 * While a multi-sector write operation is ongoing, no other operation can be performed.
 *
 * \param sector the first sector to write
 *
 * \return \c true on success, or \c false on failure
 */
bool sd_write_multi_start(uint32_t sector);

/**
 * \brief Checks whether a sector is currently being written.
 *
 * \return \c true if a sector write is currently occurring, or \c false if not
 */
bool sd_write_multi_busy(void);

/**
 * \brief Starts writing one sector of a multi-sector write.
 *
 * There must not be a sector currently being written.
 *
 * \param data the 512 bytes of data to write to the sector (which must remain unmodified until the write is no longer running)
 *
 * \return \c true on success, or \c false on failure
 */
bool sd_write_multi_sector(const void *data);

/**
 * \brief Ends a multi-sector write.
 *
 * \return \c true on success, or \c false on failure
 */
bool sd_write_multi_end(void);

#endif

