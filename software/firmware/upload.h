#ifndef FIRMWARE_UPLOAD_H
#define FIRMWARE_UPLOAD_H

#include "firmware/bootproto.h"
#include "firmware/scheduler.h"
#include "util/ihex.h"

//
// An in-progress firmware upgrade operation.
//
class upload : public noncopyable, public sigc::trackable {
	public:
		//
		// Constructs an uploader object.
		//
		upload(xbee &modem, uint64_t bot, const intel_hex &data);

		//
		// Starts the upload process.
		//
		void start();

		//
		// Fired whenever progress is made.
		//
		sigc::signal<void, double> &signal_progress_made() {
			return sig_progress_made;
		}

		//
		// Fired when the upload completes.
		//
		sigc::signal<void> &signal_upload_finished() {
			return sig_upload_finished;
		}

		//
		// Fired when an error occurs. No further activity will occur.
		//
		sigc::signal<void, const Glib::ustring &> &signal_error() {
			return sig_error;
		}

		//
		// Returns the textual status of the current upload stage.
		//
		const Glib::ustring &get_status() const {
			return status;
		}

	private:
		bootproto proto;
		upload_scheduler sched;
		Glib::ustring status;
		upload_irp irp;

		sigc::signal<void, double> sig_progress_made;
		sigc::signal<void> sig_upload_finished;
		sigc::signal<void, const Glib::ustring &> sig_error;

		void bootloader_entered();
		void ident_received(const void *);
		void send_next_irp();
		void irp_done(const void *);
		void submit_erase_block();
		void erase_block_done(const void *);
		void submit_write_page1();
		void submit_write_page2(const void *);
		void submit_write_page3(const void *);
		void write_page_done(const void *);
		void submit_crc_sector();
		void crc_sector_done(const void *);
};

#endif

