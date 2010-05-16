#ifndef FIRMWARE_FPGA_H
#define FIRMWARE_FPGA_H

#include "firmware/bootproto.h"
#include "firmware/watchable_operation.h"
#include "util/ihex.h"

//
// An operation to upload data to be burned into the SPI Flash for the FPGA.
//
class fpga_upload : public watchable_operation, public sigc::trackable {
	public:
		//
		// Constructs an uploader.
		//
		fpga_upload(xbee &modem, uint64_t bot, const intel_hex &data);

		//
		// Starts the upload process.
		//
		void start();

		//
		// The number of bytes in a page.
		//
		static const unsigned int PAGE_BYTES = 256;

		//
		// The number of pages in a chunk.
		//
		static const unsigned int CHUNK_PAGES = 16;

		//
		// The number of chunks in a sector.
		//
		static const unsigned int SECTOR_CHUNKS = 16;

	private:
		xbee &modem;
		uint64_t bot;
		const intel_hex &data;
		bootproto proto;
		unsigned int sectors_erased;
		unsigned int pages_written;
		unsigned int chunks_crcd;
		bool pages_prewritten[CHUNK_PAGES];

		void enter_bootloader_done();
		void ident_received(const void *);
		void do_work();
		void crcs_received(const void *);
};

#endif

