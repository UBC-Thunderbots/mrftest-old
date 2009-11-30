#include "firmware/crc16.h"
#include "firmware/scheduler.h"
#include <cassert>

upload_scheduler::upload_scheduler(const intel_hex &data) : data(data), blocks_erased(0), pages_written(0), sectors_checksummed(0) {
}

upload_irp upload_scheduler::next() {
	assert(!done());

	upload_irp irp;
	if (blocks_erased < bytes_blocks(data.data().size())) {
		irp.op = upload_irp::IOOP_ERASE_BLOCK;
		irp.page = blocks_erased * BLOCK_SECTORS * SECTOR_PAGES;
		irp.data = 0;
		blocks_erased++;
	} else if (pages_written < bytes_pages(data.data().size())) {
		irp.op = upload_irp::IOOP_WRITE_PAGE;
		irp.page = pages_written;
		irp.data = &data.data()[pages_written * PAGE_BYTES];
		pages_written++;
	} else {
		irp.op = upload_irp::IOOP_CRC_SECTOR;
		irp.page = sectors_checksummed * SECTOR_PAGES;
		irp.data = 0;
		sectors_checksummed++;
	}

	return irp;
}

bool upload_scheduler::done() {
	return sectors_checksummed == bytes_sectors(data.data().size());
}

bool upload_scheduler::check_crcs(uint16_t first_page, const uint16_t *crcs) {
	for (uint16_t i = 0; i < 16 && first_page + i < bytes_pages(data.data().size()); i++) {
		if (crcs[i] != crc16::calculate(&data.data()[(first_page + i) * PAGE_BYTES], 256)) {
			return false;
		}
	}
	return true;
}

