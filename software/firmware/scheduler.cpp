#include "firmware/scheduler.h"
#include "util/crc16.h"
#include <algorithm>
#include <numeric>
#include <cassert>

upload_scheduler::upload_scheduler(const intel_hex &ihex) : data(ihex.data()) {
	while (data.size() % (CHUNK_PAGES * PAGE_BYTES)) {
		data.push_back(0xFF);
	}
	crc_failures.resize(data.size() / PAGE_BYTES, 0);
	sector = 0;
	sector_start();
}

upload_irp upload_scheduler::next() {
	assert(!done());

	if (sector_done()) {
		++sector;
		sector_start();
	}

	return sector_next();
}

bool upload_scheduler::done() const {
	return sector_done() && sector + 1 == (data.size() + SECTOR_CHUNKS * CHUNK_PAGES * PAGE_BYTES - 1) / (SECTOR_CHUNKS * CHUNK_PAGES * PAGE_BYTES);
}

bool upload_scheduler::check_crcs(uint16_t first_page, const uint16_t *crcs) {
	return sector_check_crcs(first_page, crcs);
}

double upload_scheduler::progress() const {
	return std::min(1.0, static_cast<double>(sector * SECTOR_CHUNKS * CHUNK_PAGES * PAGE_BYTES) / data.size() + sector_progress() / (data.size() / PAGE_BYTES / CHUNK_PAGES / SECTOR_CHUNKS));
}

unsigned int upload_scheduler::crc_failure_count() const {
	return std::accumulate(crc_failures.begin(), crc_failures.end(), 0U);
}



void upload_scheduler::sector_start() {
	chunk = 0;
	erase_done = false;
	chunk_start();
}

bool upload_scheduler::sector_done() const {
	return chunk_done() && (chunk + 1 == SECTOR_CHUNKS || (sector * SECTOR_CHUNKS + chunk + 1) == data.size() / (CHUNK_PAGES * PAGE_BYTES));
}

upload_irp upload_scheduler::sector_next() {
	assert(!sector_done());

	if (!erase_done) {
		upload_irp irp;
		irp.op = upload_irp::IOOP_ERASE_SECTOR;
		irp.page = (sector * SECTOR_CHUNKS + chunk) * CHUNK_PAGES;
		irp.data = 0;
		erase_done = true;
		return irp;
	}

	if (chunk_done()) {
		++chunk;
		chunk_start();
	}

	return chunk_next();
}

bool upload_scheduler::sector_check_crcs(uint16_t first_page, const uint16_t *crcs) {
	return chunk_check_crcs(first_page, crcs);
}

double upload_scheduler::sector_progress() const {
	return static_cast<double>(chunk) / SECTOR_CHUNKS;
}



void upload_scheduler::chunk_start() {
	pages_written = 0;
	next_page = 0;
}

bool upload_scheduler::chunk_done() const {
	return pages_written == 0xFFFF;
}

upload_irp upload_scheduler::chunk_next() {
	while (next_page < 16 && (pages_written & (1 << next_page))) ++next_page;

	if (next_page < 16) {
		upload_irp irp;
		irp.op = upload_irp::IOOP_WRITE_PAGE;
		irp.page = (sector * SECTOR_CHUNKS + chunk) * CHUNK_PAGES + next_page;
		irp.data = &data[irp.page * PAGE_BYTES];
		++next_page;
		return irp;
	} else {
		upload_irp irp;
		irp.op = upload_irp::IOOP_CRC_CHUNK;
		irp.page = (sector * SECTOR_CHUNKS + chunk) * CHUNK_PAGES;
		irp.data = 0;
		return irp;
	}
}

bool upload_scheduler::chunk_check_crcs(uint16_t first_page, const uint16_t *crcs) {
	assert(first_page == (sector * SECTOR_CHUNKS + chunk) * CHUNK_PAGES);

	// First uint16_t is the written-pages bitmap.
	pages_written = crcs[0];
	next_page = 0;
	if (pages_written != 0xFFFF) {
		return true;
	}

	// Verify the CRCs. If any fail, redo the entire sector.
	++crcs;
	bool ok = true;
	for (unsigned int i = 0; i < CHUNK_PAGES; ++i) {
		const unsigned int page_abs = first_page + i;
		if (crcs[i] != crc16::calculate(&data[page_abs * PAGE_BYTES], PAGE_BYTES)) {
			if (++crc_failures[page_abs] == 8) {
				return false;
			}
			ok = false;
		}
	}
	if (!ok) {
		sector_start();
	}
	return true;
}

