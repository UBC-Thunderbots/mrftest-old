#include "firmware/crc16.h"
#include "firmware/scheduler.h"
#include <cassert>

upload_scheduler::upload_scheduler(const std::vector<std::vector<uint8_t> > &data) : data(data), blocks_erased(0), pages_written(0), sectors_checksummed(0) {
}

upload_irp upload_scheduler::next() {
	assert(!done());

	upload_irp irp;
	if (blocks_erased < block_count(data.size())) {
		irp.op = upload_irp::IOOP_ERASE_BLOCK;
		irp.page = blocks_erased * BLOCK_SIZE;
		irp.data = 0;
		blocks_erased++;
	} else if (pages_written < data.size()) {
		irp.op = upload_irp::IOOP_WRITE_PAGE;
		irp.page = pages_written;
		irp.data = &data[pages_written][0];
		pages_written++;
	} else {
		irp.op = upload_irp::IOOP_CRC_SECTOR;
		irp.page = sectors_checksummed * SECTOR_SIZE;
		irp.data = 0;
		sectors_checksummed++;
	}

	return irp;
}

bool upload_scheduler::done() {
	return sectors_checksummed == sector_count(data.size());
}

bool upload_scheduler::check_crcs(uint16_t first_page, const uint16_t *crcs) {
	for (uint16_t i = 0; i < 16 && first_page + i < data.size(); i++) {
		if (crcs[i] != crc16::calculate(&data[first_page + i][0], 256)) {
			return false;
		}
	}
	return true;
}

