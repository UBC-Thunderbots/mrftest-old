#ifndef FIRMWARE_SCHEDULER_H
#define FIRMWARE_SCHEDULER_H

#include "util/ihex.h"
#include "util/noncopyable.h"
#include <queue>
#include <vector>
#include <cstddef>
#include <stdint.h>

//
// An individual abstract IO request packet. This is NOT the concrete bytewise
// representation of an IRP.
//
struct upload_irp {
	enum ioop {
		IOOP_WRITE_PAGE,
		IOOP_CRC_CHUNK,
		IOOP_READ_PAGE,
		IOOP_ERASE_SECTOR,
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

		//
		// Returns the number of CRC failures seen so far.
		//
		unsigned int crc_failure_count() const;

	private:
		std::vector<uint8_t> data;
		std::vector<uint8_t> crc_failures;
		unsigned int sector;
		bool erase_done;
		unsigned int chunk;
		uint16_t pages_written;
		unsigned int next_page;

		static const std::size_t PAGE_BYTES = 256;
		static const std::size_t CHUNK_PAGES = 16;
		static const std::size_t SECTOR_CHUNKS = 16;

		void sector_start();
		bool sector_done() const;
		upload_irp sector_next();
		bool sector_check_crcs(uint16_t first_page, const uint16_t *crcs) __attribute__((warn_unused_result));
		double sector_progress() const;

		void chunk_start();
		bool chunk_done() const;
		upload_irp chunk_next();
		bool chunk_check_crcs(uint16_t first_page, const uint16_t *crcs) __attribute__((warn_unused_result));
};

#endif

