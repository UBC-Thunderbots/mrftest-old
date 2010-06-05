#ifndef FIRMWARE_PIC_H
#define FIRMWARE_PIC_H

#include "firmware/bootproto.h"
#include "firmware/watchable_operation.h"
#include "util/ihex.h"
#include "xbee/client/raw.h"

//
// An operation to upload data to be burned into the PIC.
//
class pic_upload : public watchable_operation, public sigc::trackable {
	public:
		//
		// Constructs an uploader.
		//
		pic_upload(xbee_raw_bot::ptr bot, const intel_hex &data);

		//
		// Starts the upload process.
		//
		void start();

		//
		// The number of bytes in a page.
		//
		static const unsigned int PAGE_BYTES = 64;

	private:
		const xbee_raw_bot::ptr bot;
		const intel_hex &data;
		bootproto proto;
		unsigned int pages_written;

		void enter_bootloader_done();
		void ident_received(const void *);
		void fuses_received(const void *);
		void do_work();
		void page_written(const void *);
		void upgrade_enabled(const void *);
};

#endif

