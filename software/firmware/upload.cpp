#include "firmware/upload.h"

namespace {
	struct __attribute__((packed)) IDENT_DATA {
		uint8_t signature[5];
		uint8_t manufacturer;
		uint8_t memory_type;
		uint8_t capacity;
	};

	const uint8_t COMMAND_IDENT = 0x1;
	const uint8_t COMMAND_ERASE_BLOCK = 0x2;
	const uint8_t COMMAND_WRITE_PAGE1 = 0x3;
	const uint8_t COMMAND_WRITE_PAGE2 = 0x4;
	const uint8_t COMMAND_WRITE_PAGE3 = 0x5;
	const uint8_t COMMAND_CRC_SECTOR = 0x6;
	const uint8_t COMMAND_ERASE_SECTOR = 0x7;
}

upload::upload(xbee &modem, uint64_t bot, const intel_hex &data) : proto(modem, bot), sched(data), status("Initializing...") {
	proto.signal_error().connect(sig_error.make_slot());
}

void upload::start() {
	status = "Entering Bootloader...";
	sig_progress_made.emit(0);
	proto.enter_bootloader(sigc::mem_fun(*this, &upload::bootloader_entered));
}

void upload::bootloader_entered() {
	status = "Checking Identity...";
	sig_progress_made.emit(0);
	proto.send(COMMAND_IDENT, 0, 0, 0, 8, sigc::mem_fun(*this, &upload::ident_received));
}

void upload::ident_received(const void *data) {
	const IDENT_DATA *resp = static_cast<const IDENT_DATA *>(data);
	if (!std::equal(resp->signature, resp->signature + 5, "TBOTS")) {
		sig_error.emit("Failed to check identity: Incorrect signature!");
		return;
	}
	if (resp->manufacturer != 0xEF || resp->memory_type != 0x30 || resp->capacity != 0x15) {
		sig_error.emit("Failed to check identity: Incorrect Flash JEDEC id!");
		return;
	}

	status = "Uploading...";
	sig_progress_made.emit(0);
	send_next_irp();
}

void upload::send_next_irp() {
	if (sched.done()) {
		status = "Exiting...";
		sig_progress_made.emit(1);
		proto.exit_bootloader(sig_upload_finished.make_slot());
		return;
	}

	for (;;) {
		irp = sched.next();
		switch (irp.op) {
			case upload_irp::IOOP_ERASE_BLOCK:
				submit_erase_block();
				return;

			case upload_irp::IOOP_WRITE_PAGE:
				submit_write_page();
				break;

			case upload_irp::IOOP_CRC_SECTOR:
				submit_crc_sector();
				return;

			case upload_irp::IOOP_ERASE_SECTOR:
				submit_erase_sector();
				return;

			default:
				sig_error.emit("Scheduler returned illegal IRP!");
				return;
		}
	}
}

void upload::submit_erase_block() {
	proto.send(COMMAND_ERASE_BLOCK, irp.page, 0, 0, 0, sigc::mem_fun(*this, &upload::erase_block_done));
}

void upload::erase_block_done(const void *) {
	send_next_irp();
}

void upload::submit_write_page() {
	proto.send_no_response(COMMAND_WRITE_PAGE1, irp.page, irp.data, 86);
	proto.send_no_response(COMMAND_WRITE_PAGE2, irp.page, &static_cast<const uint8_t *>(irp.data)[86], 86);
	proto.send_no_response(COMMAND_WRITE_PAGE3, irp.page, &static_cast<const uint8_t *>(irp.data)[86+86], 84);
}

void upload::submit_crc_sector() {
	proto.send(COMMAND_CRC_SECTOR, irp.page, 0, 0, 32, sigc::mem_fun(*this, &upload::crc_sector_done));
}

void upload::crc_sector_done(const void *response) {
	if (!sched.check_crcs(irp.page, static_cast<const uint16_t *>(response))) {
		sig_error.emit("CRC failed!");
		return;
	}

	sig_progress_made.emit(sched.progress());
	send_next_irp();
}

void upload::submit_erase_sector() {
	proto.send(COMMAND_ERASE_SECTOR, irp.page, 0, 0, 0, sigc::mem_fun(*this, &upload::erase_sector_done));
}

void upload::erase_sector_done(const void *) {
	send_next_irp();
}

