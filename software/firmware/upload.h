#ifndef FIRMWARE_UPLOAD_H
#define FIRMWARE_UPLOAD_H

#include "firmware/bootproto.h"
#include "firmware/scheduler.h"
#include "firmware/watchable_operation.h"
#include "util/ihex.h"

//
// An in-progress firmware upgrade operation.
//
class upload : public watchable_operation, public sigc::trackable {
	public:
		//
		// Constructs an uploader object.
		//
		upload(xbee &modem, uint64_t bot, const intel_hex &data);

		//
		// Starts the upload process.
		//
		void start();

	private:
		bootproto proto;
		upload_scheduler sched;
		upload_irp irp;

		void bootloader_entered();
		void ident_received(const void *);
		void send_next_irp();
		void irp_done(const void *);
		void submit_erase_block();
		void erase_block_done(const void *);
		void submit_write_page();
		void submit_crc_sector();
		void crc_sector_done(const void *);
		void submit_erase_sector();
		void erase_sector_done(const void *);
};

#endif

