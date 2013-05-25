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
	 * \brief The card was idle when it should not be or was not idle when it should be.
	 */
	SD_STATUS_ILLEGAL_IDLE,

	/**
	 * \brief The card encountered an internal error in its controller or uncorrectable corruption in its memory array.
	 */
	SD_STATUS_CARD_INTERNAL_ERROR,
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

