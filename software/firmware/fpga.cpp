#include "firmware/fpga.h"
#include "util/crc16.h"
#include "util/rle.h"
#include <algorithm>
#include <cstddef>

namespace {
	struct __attribute__((packed)) IDENT_DATA {
		uint8_t signature[5];
		uint8_t manufacturer;
		uint8_t memory_type;
		uint8_t capacity;
	};

	const uint8_t COMMAND_IDENT = 0x1;
	const uint8_t COMMAND_FPGA_WRITE_DATA = 0x2;
	const uint8_t COMMAND_FPGA_CRC_CHUNK = 0x3;
	const uint8_t COMMAND_FPGA_ERASE_SECTOR = 0x4;

	template<typename T>
	T divup(T num, T den) {
		return (num + den - 1) / den;
	}

	void get_page_data(const intel_hex &data, unsigned int page, unsigned char (&buffer)[fpga_upload::PAGE_BYTES]) {
		std::fill(buffer, buffer + fpga_upload::PAGE_BYTES, 0xFF);
		unsigned int byte = page * fpga_upload::PAGE_BYTES;
		if (byte < data.data()[0].size()) {
			std::copy(&data.data()[0][byte], &data.data()[0][std::min<std::size_t>(byte + fpga_upload::PAGE_BYTES, data.data()[0].size())], buffer);
		}
	}
}

fpga_upload::fpga_upload(xbee_raw_bot::ptr bot, const intel_hex &data) : bot(bot), data(data), proto(bot), sectors_erased(0), pages_written(0), chunks_crcd(0) {
	status = "Idle";
	proto.signal_error.connect(signal_error.make_slot());
}

void fpga_upload::start() {
	status = "Entering Bootloader";
	signal_progress.emit(0);
	proto.enter_bootloader(sigc::mem_fun(this, &fpga_upload::enter_bootloader_done));
}

void fpga_upload::enter_bootloader_done() {
	status = "Checking Identity";
	signal_progress.emit(0);
	proto.send(COMMAND_IDENT, 0, 0, 0, 8, sigc::mem_fun(this, &fpga_upload::ident_received));
}

void fpga_upload::ident_received(const void *response) {
	const IDENT_DATA &ident = *static_cast<const IDENT_DATA *>(response);
	if (!std::equal(ident.signature, ident.signature + 5, "TBOTS")) {
		signal_error.emit("Incorrect IDENT signature!");
		return;
	}
	if (ident.manufacturer != 0x20 || ident.memory_type != 0x20 || ident.capacity != 0x15) {
		signal_error.emit("Incorrect JEDEC ID!");
		return;
	}
	status = "Uploading";
	signal_progress.emit(0);
	do_work();
}

void fpga_upload::do_work() {
	for (;;) {
		if (chunks_crcd == divup<std::size_t>(data.data()[0].size(), CHUNK_PAGES * PAGE_BYTES)) {
			// We have CRCd as many chunks as are in the HEX file. We're done.
			status = "Exiting Bootloader";
			signal_progress.emit(1);
			proto.exit_bootloader(signal_finished.make_slot());
			return;
		} else if (pages_written == (chunks_crcd + 1) * CHUNK_PAGES) {
			// We have written a full chunk's worth of pages. CRC the new pages.
			unsigned int first_page = chunks_crcd * CHUNK_PAGES;
			std::fill(pages_prewritten, pages_prewritten + CHUNK_PAGES, false);
			proto.send(COMMAND_FPGA_CRC_CHUNK, first_page, 0, 0, 34, sigc::mem_fun(this, &fpga_upload::crcs_received));
			return;
		} else if (chunks_crcd == sectors_erased * SECTOR_CHUNKS) {
			// We have CRCd a full sector's worth of pages. We must erase more before continuing.
			unsigned int first_page = sectors_erased * SECTOR_CHUNKS * CHUNK_PAGES;
			std::fill(pages_prewritten, pages_prewritten + CHUNK_PAGES, false);
			proto.send_no_response(COMMAND_FPGA_ERASE_SECTOR, first_page, 0, 0);
			++sectors_erased;
		} else {
			// We should write a page.
			if (pages_written >= divup<std::size_t>(data.data()[0].size(), PAGE_BYTES)) {
				// Skip this page because it's beyond the end of the data.
			} else if (pages_prewritten[pages_written % CHUNK_PAGES]) {
				// Skip this page because it was written properly according to
				// the bitmap.
			} else {
				unsigned char inbuf[PAGE_BYTES];
				get_page_data(data, pages_written, inbuf);
				rle_compressor comp(inbuf, PAGE_BYTES);
				while (!comp.done()) {
					unsigned char rlebuf[97];
					std::size_t len = comp.next(rlebuf, sizeof(rlebuf));
					proto.send_no_response(COMMAND_FPGA_WRITE_DATA, pages_written, rlebuf, len);
				}
			}
			++pages_written;
		}

		signal_progress.emit(static_cast<double>(pages_written) / (divup<std::size_t>(divup<std::size_t>(data.data()[0].size(), PAGE_BYTES), CHUNK_PAGES) * CHUNK_PAGES));
	}
}

void fpga_upload::crcs_received(const void *response) {
	// Decode the received data.
	uint16_t words[CHUNK_PAGES + 1];
	for (unsigned int i = 0; i < CHUNK_PAGES + 1; ++i) {
		words[i] = static_cast<const unsigned char *>(response)[i * 2] * 256 + static_cast<const unsigned char *>(response)[i * 2 + 1];
	}

	// First consider the bitmap of pages written.
	bool all_ok = true;
	for (unsigned int i = 0; i < CHUNK_PAGES && chunks_crcd * CHUNK_PAGES + i < divup<std::size_t>(data.data()[0].size(), PAGE_BYTES); ++i) {
		pages_prewritten[i] = !!(words[0] & (1 << i));
		if (!pages_prewritten[i]) {
			all_ok = false;
		}
	}
	if (!all_ok) {
		pages_written -= CHUNK_PAGES;
		do_work();
		return;
	}

	// Now verify the CRCs of the data in Flash.
	for (unsigned int i = 0; i < CHUNK_PAGES && chunks_crcd * CHUNK_PAGES + i < divup<std::size_t>(data.data()[0].size(), PAGE_BYTES); ++i) {
		unsigned char pagedata[PAGE_BYTES];
		get_page_data(data, chunks_crcd * CHUNK_PAGES + i, pagedata);
		uint16_t computed = crc16::calculate(pagedata, PAGE_BYTES);
		if (computed != words[i + 1]) {
			signal_error.emit("CRC mismatch!");
			return;
		}
	}

	// Proceed.
	++chunks_crcd;
	std::fill(pages_prewritten, pages_prewritten + CHUNK_PAGES, false);
	do_work();
}

