#include "firmware/upload.h"

namespace {
	struct __attribute__((packed)) IDENT_DATA {
		uint8_t signature[5];
		uint8_t manufacturer;
		uint8_t memory_type;
		uint8_t capacity;
	};

	const uint8_t COMMAND_IDENT = 0x1;
	const uint8_t COMMAND_READ_BUFFER = 0x2;
	const uint8_t COMMAND_WRITE_BUFFER = 0x3;
	const uint8_t COMMAND_READ_IRPS = 0x4;
	const uint8_t COMMAND_CLEAR_IRPS = 0x5;
	const uint8_t COMMAND_SUBMIT_IRP = 0x6;

	const uint8_t IO_OPERATION_ERASE_BLOCK = 0x1;
	const uint8_t IO_OPERATION_WRITE_PAGE = 0x2;
	const uint8_t IO_OPERATION_CRC_SECTOR = 0x3;
	const uint8_t IRP_STATUS_EMPTY = 0x0;
	const uint8_t IRP_STATUS_PENDING = 0x1;
	const uint8_t IRP_STATUS_BUSY = 0x2;
	const uint8_t IRP_STATUS_COMPLETE = 0x3;
	const uint8_t IRP_STATUS_BAD_OPERATION = 0x4;
	const uint8_t IRP_STATUS_BAD_ADDRESS = 0x5;
}

upload::upload(xbee &modem, uint64_t bot, const std::vector<std::vector<uint8_t> > &data) : proto(modem, bot), sched(data), status("Initializing..."), irpptr(0), irpmask(0) {
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

	proto.send(COMMAND_CLEAR_IRPS, 0x0F, 0, 0, 0, sigc::mem_fun(*this, &upload::init_irps_cleared));
}

void upload::init_irps_cleared(const void *) {
	proto.send(COMMAND_READ_IRPS, 0, 0, 0, 4, sigc::mem_fun(*this, &upload::init_irps_read));
}

void upload::init_irps_read(const void *data) {
	const uint8_t *irpstatii = static_cast<const uint8_t *>(data);
	bool any_nonempty = false;
	for (unsigned int i = 0; i < 4; i++)
		if ((irpstatii[i] >> 4) != IRP_STATUS_EMPTY)
			any_nonempty = true;

	if (any_nonempty) {
		proto.send(COMMAND_CLEAR_IRPS, 0x0F, 0, 0, 0, sigc::mem_fun(*this, &upload::init_irps_cleared));
	} else {
	}
}

void upload::push_irps() {
	if (sched.done()) {
		if (irpmask == 0x0F) {
			status = "Exiting...";
			sig_progress_made.emit(1);
			proto.exit_bootloader(sig_upload_finished.make_slot());
		}
		return;
	}

	if (irpmask == 0x0F) {
		return;
	}

	while (irpmask & (1 << irpptr))
		irpptr = (irpptr + 1) & 0x03;

	irps[irpptr] = sched.next();
	switch (irps[irpptr].op) {
		case upload_irp::IOOP_ERASE_BLOCK:
			submit_erase_block();
			return;

		case upload_irp::IOOP_WRITE_PAGE:
			submit_write_page1();
			return;
		case upload_irp::IOOP_CRC_SECTOR:
			submit_crc_sector();
			return;

		default:
			sig_error.emit("Scheduler returned illegal IRP!");
			return;
	}
}

void upload::submit_erase_block() {
	uint8_t buffer[3];
	buffer[0] = IO_OPERATION_ERASE_BLOCK;
	buffer[1] = irps[irpptr].page >> 8;
	buffer[2] = irps[irpptr].page & 0xFF;
	proto.send(COMMAND_SUBMIT_IRP, irpptr, buffer, sizeof(buffer), 0, sigc::mem_fun(*this, &upload::start_irp_scan));
	irpmask |= 1 << irpptr;
}

void upload::submit_write_page1() {
	const uint8_t *bufdata = static_cast<const uint8_t *>(irps[irpptr].data);
	uint8_t buffer[86];
	buffer[0] = 0;
	std::copy(&bufdata[0], &bufdata[85], &buffer[1]);
	proto.send(COMMAND_WRITE_BUFFER, irpptr, buffer, sizeof(buffer), 0, sigc::mem_fun(*this, &upload::submit_write_page2));
}

void upload::submit_write_page2(const void *) {
	const uint8_t *bufdata = static_cast<const uint8_t *>(irps[irpptr].data);
	uint8_t buffer[86];
	buffer[0] = 85;
	std::copy(&bufdata[85], &bufdata[170], &buffer[1]);
	proto.send(COMMAND_WRITE_BUFFER, irpptr, buffer, sizeof(buffer), 0, sigc::mem_fun(*this, &upload::submit_write_page3));
}

void upload::submit_write_page3(const void *) {
	const uint8_t *bufdata = static_cast<const uint8_t *>(irps[irpptr].data);
	uint8_t buffer[87];
	buffer[0] = 170;
	std::copy(&bufdata[170], &bufdata[256], &buffer[1]);
	proto.send(COMMAND_WRITE_BUFFER, irpptr, buffer, sizeof(buffer), 0, sigc::mem_fun(*this, &upload::submit_write_page4));
}

void upload::submit_write_page4(const void *) {
	uint8_t buffer[3];
	buffer[0] = IO_OPERATION_WRITE_PAGE;
	buffer[1] = irps[irpptr].page >> 8;
	buffer[2] = irps[irpptr].page & 0xFF;
	proto.send(COMMAND_SUBMIT_IRP, irpptr, buffer, sizeof(buffer), 0, sigc::mem_fun(*this, &upload::start_irp_scan));
	irpmask |= 1 << irpptr;
}

void upload::submit_crc_sector() {
	uint8_t buffer[3];
	buffer[0] = IO_OPERATION_CRC_SECTOR;
	buffer[1] = irps[irpptr].page >> 8;
	buffer[2] = irps[irpptr].page & 0xFF;
	proto.send(COMMAND_SUBMIT_IRP, irpptr, buffer, sizeof(buffer), 0, sigc::mem_fun(*this, &upload::start_irp_scan));
	irpmask |= 1 << irpptr;
}

void upload::start_irp_scan(const void *) {
	if (irpmask != 0x0F) {
		push_irps();
		return;
	}

	proto.send(COMMAND_READ_IRPS, 0, 0, 0, 4, sigc::mem_fun(*this, &upload::irps_read));
}

void upload::irps_read(const void *response) {
	const uint8_t *statii = static_cast<const uint8_t *>(response);
	uint8_t toclear = 0;

	for (unsigned int i = 0; i < 4; i++) {
		switch (statii[i]) {
			case IRP_STATUS_EMPTY:
				irpmask &= ~(1 << i);
				break;

			case IRP_STATUS_PENDING:
			case IRP_STATUS_BUSY:
				break;

			case IRP_STATUS_COMPLETE:
				if (irps[i].op == upload_irp::IOOP_CRC_SECTOR) {
					uint8_t buffer[2];
					buffer[0] = 0;
					buffer[1] = 32;
					proto.send(COMMAND_READ_BUFFER, i, buffer, sizeof(buffer), 32, sigc::bind(sigc::mem_fun(*this, &upload::crcs_received), i));
					return;
				} else {
					irpmask &= ~(1 << i);
					toclear |= 1 << i;
				}
				break;

			default:
				sig_error.emit("IRP failed!");
				return;
		}
	}

	if (toclear) {
		proto.send(COMMAND_CLEAR_IRPS, toclear, 0, 0, 0, sigc::mem_fun(*this, &upload::start_irp_scan));
	} else {
		push_irps();
	}
}

void upload::crcs_received(const void *response, unsigned int index) {
	if (!sched.check_crcs(irps[index].page, static_cast<const uint16_t *>(response))) {
		sig_error.emit("CRC failed!");
		return;
	}

	proto.send(COMMAND_CLEAR_IRPS, 1 << index, 0, 0, 0, sigc::mem_fun(*this, &upload::start_irp_scan));
	irpmask &= ~(1 << index);
}

