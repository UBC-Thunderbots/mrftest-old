#ifndef FIRMWARE_SCHEDULER_H
#define FIRMWARE_SCHEDULER_H

#include "util/ihex.h"
#include "util/noncopyable.h"
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
		upload_scheduler(const intel_hex &data);

		//
		// Returns true if all IRPs have been returned.
		//
		bool done();

		//
		// Gets the next IRP.
		//
		upload_irp next() __attribute__((warn_unused_result));

		//
		// Checks a set of 16 CRCs.
		//
		bool check_crcs(uint16_t first_page, const uint16_t *crcs) __attribute__((warn_unused_result));

	private:
		const intel_hex &data;
		unsigned int blocks_erased, pages_written, sectors_checksummed;

		void erase_finished(upload_irp, const void *);
		void write_finished(upload_irp, const void *);
		void checksum_finished(upload_irp, const void *);

		static const std::size_t PAGE_BYTES = 256;
		static const uint16_t SECTOR_PAGES = 16;
		static const uint16_t BLOCK_SECTORS = 16;

		static unsigned int bytes_pages(unsigned int bytes) {
			return (bytes + PAGE_BYTES - 1) / PAGE_BYTES;
		}

		static unsigned int pages_sectors(unsigned int pages) {
			return (pages + SECTOR_PAGES - 1) / SECTOR_PAGES;
		}

		static unsigned int bytes_sectors(unsigned int bytes) {
			return pages_sectors(bytes_pages(bytes));
		}

		static unsigned int sectors_blocks(unsigned int sectors) {
			return (sectors + BLOCK_SECTORS - 1) / BLOCK_SECTORS;
		}

		static unsigned int pages_blocks(unsigned int pages) {
			return sectors_blocks(pages_sectors(pages));
		}

		static unsigned int bytes_blocks(unsigned int bytes) {
			return sectors_blocks(bytes_sectors(bytes));
		}
};

#endif

