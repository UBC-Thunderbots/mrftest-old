#include "firmware/upload.h"
#include "util/rle.h"
#include <iomanip>

namespace {
	struct __attribute__((packed)) IDENT_DATA {
		uint8_t signature[5];
		uint8_t manufacturer;
		uint8_t memory_type;
		uint8_t capacity;
	};

	const uint8_t COMMAND_IDENT = 0x1;
	const uint8_t COMMAND_WRITE_DATA = 0x2;
	const uint8_t COMMAND_CRC_CHUNK = 0x3;
	const uint8_t COMMAND_ERASE_SECTOR = 0x4;
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
	if (resp->manufacturer != 0x20 || resp->memory_type != 0x20 || resp->capacity != 0x15) {
		signal_error().emit(Glib::ustring::compose("Failed to check identity: Incorrect Flash JEDEC id! (have %1%2%3, want 202015)",
			Glib::ustring::format(std::hex, std::setw(2), std::setfill(L'0'), resp->manufacturer),
			Glib::ustring::format(std::hex, std::setw(2), std::setfill(L'0'), resp->memory_type),
			Glib::ustring::format(std::hex, std::setw(2), std::setfill(L'0'), resp->capacity)));
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
			case upload_irp::IOOP_WRITE_PAGE:
				submit_write_page();
				break;

			case upload_irp::IOOP_CRC_CHUNK:
				submit_crc_chunk();
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

void upload::submit_write_page() {
	rle_compressor comp(irp.data, 256);
	while (!comp.done()) {
		unsigned char buffer[97];
		std::size_t len = comp.next(buffer, sizeof(buffer));
		proto.send_no_response(COMMAND_WRITE_DATA, irp.page, buffer, len);
	}
}

void upload::submit_crc_chunk() {
	proto.send(COMMAND_CRC_CHUNK, irp.page, 0, 0, 34, sigc::mem_fun(*this, &upload::crc_chunk_done));
}

void upload::crc_chunk_done(const void *response) {
	uint16_t crcs[17];
	const unsigned char *data = static_cast<const unsigned char *>(response);
	for (unsigned int i = 0; i < 17; ++i) {
		crcs[i] = 256 * data[i * 2] + data[i * 2 + 1];
	}
	if (!sched.check_crcs(irp.page, crcs)) {
		signal_error().emit("CRC failed!");
		return;
	}

	signal_progress().emit(sched.progress());
	send_next_irp();
}

void upload::submit_erase_sector() {
	proto.send_no_response(COMMAND_ERASE_SECTOR, irp.page, 0, 0);
}

