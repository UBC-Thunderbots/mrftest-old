#ifndef FIRMWARE_SCHEDULER_H
#define FIRMWARE_SCHEDULER_H

#include "util/ihex.h"
#include "util/noncopyable.h"
#include <queue>
#include <cstddef>
#include <stdint.h>

//
// An individual abstract IO request packet. This is NOT the concrete bytewise
// representation of an IRP.
//
struct upload_irp {
	enum ioop {
		IOOP_ERASE_BLOCK,
		IOOP_WRITE_PAGE,
		IOOP_CRC_SECTOR,
		IOOP_READ_PAGE,
	} op;
	uint16_t page;
	const void *data;
};

//
// Manages scheduling which IRPs should be dispatched when to the chip.
//
class upload_scheduler : public noncopyable {
	public:
		//
		// Constructs a new upload_scheduler.
		//
		upload_scheduler(const intel_hex &data);

		//
		// Returns true if all IRPs have been returned.
		//
		bool done() const;

		//
		// Gets the next IRP.
		//
		upload_irp next() __attribute__((warn_unused_result));

		//
		// Checks a set of 16 CRCs.
		//
		bool check_crcs(uint16_t first_page, const uint16_t *crcs) __attribute__((warn_unused_result));

		//
		// Returns the approximate progress of the operation (between 0 and 1).
		//
		double progress() const;

	private:
		const intel_hex &data;
		std::queue<upload_irp> irps;
		std::size_t initial_qlen;

		static const std::size_t PAGE_BYTES = 256;
		static const uint16_t SECTOR_PAGES = 16;
		static const uint16_t BLOCK_SECTORS = 16;

		static const std::size_t SECTOR_BYTES = SECTOR_PAGES * PAGE_BYTES;
		static const std::size_t BLOCK_BYTES = BLOCK_SECTORS * SECTOR_BYTES;
};

#endif

