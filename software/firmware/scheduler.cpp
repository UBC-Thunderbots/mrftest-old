#include "firmware/crc16.h"
#include "firmware/scheduler.h"
#include <cassert>

upload_scheduler::upload_scheduler(const intel_hex &ihex) : data(ihex.data()) {
	while (data.size() % PAGE_BYTES) {
		data.push_back(0xFF);
	}
	upload_irp irp;
	for (unsigned int block = 0; block * BLOCK_BYTES < data.size(); ++block) {
		irp.op = upload_irp::IOOP_ERASE_BLOCK;
		irp.page = block * BLOCK_SECTORS * SECTOR_PAGES;
		irp.data = 0;
		irps.push(irp);
		for (unsigned int sector = 0; block * BLOCK_BYTES + sector * SECTOR_BYTES < data.size() && sector < BLOCK_SECTORS; ++sector) {
			for (unsigned int page = 0; block * BLOCK_BYTES + sector * SECTOR_BYTES + page * PAGE_BYTES < data.size() && page < SECTOR_PAGES; ++page) {
				irp.op = upload_irp::IOOP_WRITE_PAGE;
				irp.page = (block * BLOCK_SECTORS + sector) * SECTOR_PAGES + page;
				irp.data = &data[block * BLOCK_BYTES + sector * SECTOR_BYTES + page * PAGE_BYTES];
				irps.push(irp);
				crc_failures.push_back(0);
			}
			irp.op = upload_irp::IOOP_CRC_SECTOR;
			irp.page = (block * BLOCK_SECTORS + sector) * SECTOR_PAGES;
			irp.data = 0;
			irps.push(irp);
		}
	}
	initial_qlen = irps.size();
}

upload_irp upload_scheduler::next() {
	assert(!done());
	upload_irp irp = irps.front();
	irps.pop();
	return irp;
}

bool upload_scheduler::done() const {
	return irps.empty();
}

bool upload_scheduler::check_crcs(uint16_t first_page, const uint16_t *crcs) {
	bool ok = true;
	for (uint16_t i = 0; i < 16 && (first_page + i) * PAGE_BYTES < data.size(); ++i) {
		if (crcs[i] != crc16::calculate(&data[(first_page + i) * PAGE_BYTES], PAGE_BYTES)) {
			if (++crc_failures[first_page + i] == 8) {
				return false;
			}
			ok = false;
		}
	}
	if (ok) {
		return true;
	}
	unsigned int sector = first_page / SECTOR_PAGES;
	upload_irp irp;
	irp.op = upload_irp::IOOP_ERASE_SECTOR;
	irp.page = first_page;
	irp.data = 0;
	irps.push(irp);
	for (unsigned int page = 0; sector * SECTOR_BYTES + page * PAGE_BYTES < data.size() && page < SECTOR_PAGES; ++page) {
		irp.op = upload_irp::IOOP_WRITE_PAGE;
		irp.page = sector * SECTOR_PAGES + page;
		irp.data = &data[sector * SECTOR_BYTES + page * PAGE_BYTES];
		irps.push(irp);
	}
	irp.op = upload_irp::IOOP_CRC_SECTOR;
	irp.page = sector * SECTOR_PAGES;
	irp.data = 0;
	irps.push(irp);
	return true;
}

double upload_scheduler::progress() const {
	return static_cast<double>(initial_qlen - irps.size()) / initial_qlen;
}

unsigned int upload_scheduler::crc_failure_count() const {
	unsigned int result = 0;
	for (unsigned int i = 0; i < crc_failures.size(); ++i) {
		result += crc_failures[i];
	}
	return result;
}

