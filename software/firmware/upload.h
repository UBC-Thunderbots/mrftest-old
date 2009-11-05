#ifndef FIRMWARE_UPLOAD_H
#define FIRMWARE_UPLOAD_H

#include "firmware/bootproto.h"
#include "firmware/scheduler.h"
#include "util/byref.h"
#include "xbee/util.h"
#include "xbee/xbee.h"
#include <vector>
#include <stdint.h>
#include <sigc++/sigc++.h>

//
// An in-progress firmware upgrade operation.
//
class upload : public byref, public sigc::trackable {
	public:
		//
		// Constructs an uploader object.
		//
		upload(xbee &modem, uint64_t bot, const std::vector<std::vector<uint8_t> > &data);

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
		uint8_t irpptr, irpmask;
		upload_irp irps[4];

		sigc::signal<void, double> sig_progress_made;
		sigc::signal<void> sig_upload_finished;
		sigc::signal<void, const Glib::ustring &> sig_error;

		void bootloader_entered();
		void ident_received(const void *);
		void init_irps_cleared(const void *);
		void init_irps_read(const void *);
		void push_irps();
		void submit_erase_block();
		void submit_write_page1();
		void submit_write_page2(const void *);
		void submit_write_page3(const void *);
		void submit_write_page4(const void *);
		void submit_crc_sector();
		void start_irp_scan(const void *);
		void irps_read(const void *);
		void crcs_received(const void *, unsigned int);
};

#endif

