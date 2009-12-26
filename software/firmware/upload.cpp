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

upload::upload(xbee &modem, uint64_t bot, const intel_hex &data) : proto(modem, bot), sched(data) {
	status = "Initializing...";
	proto.signal_error().connect(signal_error().make_slot());
}

void upload::start() {
	status = "Entering Bootloader...";
	signal_progress().emit(0);
	proto.enter_bootloader(sigc::mem_fun(*this, &upload::bootloader_entered));
}

void upload::bootloader_entered() {
	status = "Checking Identity...";
	signal_progress().emit(0);
	proto.send(COMMAND_IDENT, 0, 0, 0, 8, sigc::mem_fun(*this, &upload::ident_received));
}

void upload::ident_received(const void *data) {
	const IDENT_DATA *resp = static_cast<const IDENT_DATA *>(data);
	if (!std::equal(resp->signature, resp->signature + 5, "TBOTS")) {
		signal_error().emit("Failed to check identity: Incorrect signature!");
		return;
	}
	if (resp->manufacturer != 0xEF || resp->memory_type != 0x30 || resp->capacity != 0x15) {
		signal_error().emit("Failed to check identity: Incorrect Flash JEDEC id!");
		return;
	}

	status = "Uploading...";
	signal_progress().emit(0);
	send_next_irp();
}

void upload::send_next_irp() {
	if (sched.done()) {
		status = "Exiting...";
		signal_progress().emit(1);
		proto.exit_bootloader(signal_finished().make_slot());
		return;
	}

	for (;;) {
		irp = sched.next();
		switch (irp.op) {
			case upload_irp::IOOP_ERASE_BLOCK:
				submit_erase_block();
				break;

			case upload_irp::IOOP_WRITE_PAGE:
				submit_write_page();
				break;

			case upload_irp::IOOP_CRC_SECTOR:
				submit_crc_sector();
				return;

			case upload_irp::IOOP_ERASE_SECTOR:
				submit_erase_sector();
				break;

			default:
				signal_error().emit("Scheduler returned illegal IRP!");
				return;
		}
	}
}

void upload::submit_erase_block() {
	proto.send_no_response(COMMAND_ERASE_BLOCK, irp.page, 0, 0);
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
		signal_error().emit("CRC failed!");
		return;
	}

	signal_progress().emit(sched.progress());
	send_next_irp();
}

void upload::submit_erase_sector() {
	proto.send_no_response(COMMAND_ERASE_SECTOR, irp.page, 0, 0);
}

