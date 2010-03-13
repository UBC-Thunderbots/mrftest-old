#define DEBUG 0
#include "firmware/pic.h"
#include "util/dprint.h"
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
	const uint8_t COMMAND_PIC_READ_FUSES = 0x5;
	const uint8_t COMMAND_PIC_WRITE_DATA = 0x6;
	const uint8_t COMMAND_PIC_ENABLE_UPGRADE = 0x7;

	template<typename T>
	T divup(T num, T den) {
		return (num + den - 1) / den;
	}

	void get_page_data(const intel_hex &data, unsigned int page, unsigned char (&buffer)[pic_upload::PAGE_BYTES]) {
		std::fill(buffer, buffer + pic_upload::PAGE_BYTES, 0xFF);
		unsigned int byte = page * pic_upload::PAGE_BYTES;
		if (byte < data.data()[0].size()) {
			std::copy(&data.data()[0][byte], &data.data()[0][std::min<std::size_t>(byte + pic_upload::PAGE_BYTES, data.data()[0].size())], buffer);
		}
	}

	const uint8_t FUSE_MASK[16] = {
		UINT8_C(0x3F), /* CONFIG1L = NA : NA : USBDIV : CPUDIV1    :   CPUDIV0 : PLLDIV2 : PLLDIV1 : PLLDIV0 */
		UINT8_C(0xCF), /* CONFIG1H = IESO : FCMEN : NA : NA        :   FOSC3 : FOSC2 : FOSC1 : FOSC0 */
		UINT8_C(0x3F), /* CONFIG2L = NA : NA : VREGEN : BORV1      :   BORV0 : BOREN1 : BOREN0 : /PWRTEN */
		UINT8_C(0x1F), /* CONFIG2H = NA : NA : NA : WDTPS3         :   WDTPS2 : WDTPS1 : WDTPS0 : WDTEN */
		UINT8_C(0x00), /* unimplemented */
		UINT8_C(0x87), /* CONFIG3H = MCLRE : NA : NA : NA          :   NA : LPT1OSC : PBADEN : CCP2MX */
		UINT8_C(0xE5), /* CONFIG4L = /DEBUG : XINST : ICPRT : NA   :   NA : LVP : NA : STVREN */
		UINT8_C(0x00), /* unimplemented */
		UINT8_C(0x0F), /* CONFIG5L = NA : NA : NA : NA             :   CP3 : CP2 : CP1 : CP0 */
		UINT8_C(0xC0), /* CONFIG5H = CPD : CPB : NA : NA           :   NA : NA : NA : NA */
		UINT8_C(0x0F), /* CONFIG6L = NA : NA : NA : NA             :   WRT3 : WRT2 : WRT1 : WRT0 */
		UINT8_C(0xE0), /* CONFIG6H = WRTD : WRTB : WRTC : NA       :   NA : NA : NA : NA */
		UINT8_C(0x0F), /* CONFIG7L = NA : NA : NA : NA             :   EBTR3 : EBTR2 : EBTR1 :EBTR0 */
		UINT8_C(0x40), /* CONFIG7H = NA : EBTRB : NA : NA          :   NA : NA : NA : NA */
		UINT8_C(0x00), /* unimplemented */
		UINT8_C(0x00)  /* unimplemented */
	};
}

pic_upload::pic_upload(xbee &modem, uint64_t bot, const intel_hex &data) : modem(modem), bot(bot), data(data), proto(modem, bot), pages_written(0) {
	DPRINT(Glib::ustring::compose("Constructed pic_upload with %1 bytes of boot block, %2 bytes of main ROM, and %3 bytes of fuses.", data.data()[0].size(), data.data()[1].size(), data.data()[2].size()));
	status = "Idle";
	proto.signal_error().connect(signal_error().make_slot());
}

void pic_upload::start() {
	DPRINT("Entering bootloader.");
	status = "Entering Bootloader";
	signal_progress().emit(0);
	proto.enter_bootloader(sigc::mem_fun(*this, &pic_upload::enter_bootloader_done));
}

void pic_upload::enter_bootloader_done() {
	DPRINT("Sending COMMAND_IDENT.");
	status = "Checking Identity";
	signal_progress().emit(0);
	proto.send(COMMAND_IDENT, 0, 0, 0, 8, sigc::mem_fun(*this, &pic_upload::ident_received));
}

void pic_upload::ident_received(const void *response) {
	DPRINT("IDENT response received. Sending COMMAND_PIC_READ_FUSES.");
	const IDENT_DATA &ident = *static_cast<const IDENT_DATA *>(response);
	if (!std::equal(ident.signature, ident.signature + 5, "TBOTS")) {
		signal_error().emit("Incorrect IDENT signature!");
		return;
	}
	status = "Checking Fuses";
	signal_progress().emit(0);
	proto.send(COMMAND_PIC_READ_FUSES, 0, 0, 0, 18, sigc::mem_fun(*this, &pic_upload::fuses_received));
}

void pic_upload::fuses_received(const void *response) {
	DPRINT("Fuse data received.");
	const uint8_t *fuses = static_cast<const uint8_t *>(response);
	DPRINT(Glib::ustring::compose("Device ID bits are %1:%2.", fuses[16] + 0U, fuses[17] + 0U));
	if (fuses[17] != UINT8_C(0x12) || (fuses[16] & UINT8_C(0xE0)) != UINT8_C(0x00)) {
		signal_error().emit("Device ID mismatch; please check proper chip type!");
		return;
	}
	DPRINT(Glib::ustring::compose("Revision number is %1.", fuses[16] & 0x1FU));
	for (unsigned int i = 0; i < std::min<std::size_t>(16U, data.data()[2].size()); ++i) {
		DPRINT(Glib::ustring::compose("Fuse byte %1 is %2 in PIC, %3 in HEX file.", i, fuses[i] + 0U, data.data()[2][i] + 0U));
		if ((fuses[i] & FUSE_MASK[i]) != (data.data()[2][i] & FUSE_MASK[i])) {
			signal_error().emit("Configuration fuse mismatch; please burn with a real programmer!");
			return;
		}
	}
	status = "Uploading";
	signal_progress().emit(0);
	do_work();
}

void pic_upload::do_work() {
	if (pages_written < divup<std::size_t>(data.data()[1].size(), PAGE_BYTES)) {
		// We should write a page. The address is offset by 0x800 to advance by
		// the size of the boot block, and 0x4000 to move into the staging area.
		DPRINT(Glib::ustring::compose("Writing page %1 at address %2.", pages_written, pages_written * PAGE_BYTES + 0x4800));
		proto.send(COMMAND_PIC_WRITE_DATA, pages_written * PAGE_BYTES + 0x4800, &data.data()[1][pages_written * PAGE_BYTES], PAGE_BYTES, PAGE_BYTES, sigc::mem_fun(*this, &pic_upload::page_written));
	} else {
		// All pages are written. Set the upgrade flag.
		DPRINT("Enabling upgrade flag.");
		proto.send(COMMAND_PIC_ENABLE_UPGRADE, 0, 0, 0, 2, sigc::mem_fun(*this, &pic_upload::upgrade_enabled));
	}

	signal_progress().emit(static_cast<double>(pages_written) / divup<std::size_t>(data.data()[1].size(), PAGE_BYTES));
}

void pic_upload::page_written(const void *response) {
	const unsigned char *ptr = static_cast<const unsigned char *>(response);
	if (std::equal(ptr, ptr + PAGE_BYTES, &data.data()[1][pages_written * PAGE_BYTES])) {
		DPRINT(Glib::ustring::compose("Page %1 written intact.", pages_written));
		++pages_written;
		do_work();
	} else {
		DPRINT(Glib::ustring::compose("Page %1 not written intact.", pages_written));
		signal_error().emit("Page readback incorrect!");
	}
}

void pic_upload::upgrade_enabled(const void *response) {
	const unsigned char *ptr = static_cast<const unsigned char *>(response);
	if (ptr[0] == 0x12 && ptr[1] == 0x34) {
		DPRINT("Upgrade enable flag intact.");
		status = "Exiting Bootloader";
		proto.exit_bootloader(signal_finished().make_slot());
	} else {
		DPRINT("Upgrade enable flag incorrect.");
		signal_error().emit("Upgrade enable flag incorrect!");
	}
}

