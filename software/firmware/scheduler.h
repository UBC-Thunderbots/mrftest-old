#ifndef FIRMWARE_SCHEDULER_H
#define FIRMWARE_SCHEDULER_H

#include "util/noncopyable.h"
#include <vector>
#include <tr1/unordered_set>
#include <cstddef>
#include <stdint.h>
#include <sigc++/sigc++.h>

//
// An individual abstract IO request packet. This is NOT the concrete bytewise
// representation of an IRP.
//
struct upload_irp {
	enum ioop {
		IOOP_ERASE_BLOCK,
		IOOP_WRITE_PAGE,
		IOOP_CRC_SECTOR
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
		upload_scheduler(const std::vector<std::vector<uint8_t> > &data);

		//
		// Returns true if all IRPs have been returned.
		//
		bool done();

		//
		// Gets the next IRP.
		//
		upload_irp next();

		//
		// Checks a set of 16 CRCs.
		//
		bool check_crcs(uint16_t first_page, const uint16_t *crcs);

	private:
		std::vector<std::vector<uint8_t> > data;
		unsigned int blocks_erased, pages_written, sectors_checksummed;

		void erase_finished(upload_irp, const void *);
		void write_finished(upload_irp, const void *);
		void checksum_finished(upload_irp, const void *);

		static const std::size_t PAGE_BYTES = 256;
		static const uint16_t SECTOR_SIZE = 16;
		static const uint16_t BLOCK_SIZE = 256;

		static uint16_t round_down_to_sector(uint16_t page) {
			return page & ~(SECTOR_SIZE - 1);
		}

		static uint16_t round_up_to_sector(uint16_t page) {
			return round_down_to_sector(page + SECTOR_SIZE - 1);
		}

		static uint16_t round_down_to_block(uint16_t page) {
			return page & ~(BLOCK_SIZE - 1);
		}

		static uint16_t round_up_to_block(uint16_t page) {
			return round_down_to_block(page + SECTOR_SIZE - 1);
		}

		static uint16_t sector_count(uint16_t pages) {
			return (pages + SECTOR_SIZE - 1) / SECTOR_SIZE;
		}

		static uint16_t block_count(uint16_t pages) {
			return (pages + BLOCK_SIZE - 1) / BLOCK_SIZE;
		}
};

#endif

