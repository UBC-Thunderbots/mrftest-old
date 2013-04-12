#include "io.h"
#include "mrf.h"
#include "sleep.h"

static inline void sleep_50ns(void) {
	asm volatile("nop");
	asm volatile("nop");
}

uint8_t mrf_read_short(uint8_t reg) {
	MRF_CTL = 0x04; // Assert chip select
	sleep_50ns();
	MRF_DATA = reg << 1; // Send first byte (read command plus address)
	while (MRF_CTL & 0x01); // Wait for bus idle
	MRF_DATA = 0x00; // Send second byte (dummy for receive)
	while (MRF_CTL & 0x01); // Wait for bus idle
	sleep_50ns();
	MRF_CTL = 0x06; // Deassert chip select
	return MRF_DATA; // Return read data
}

void mrf_write_short(uint8_t reg, uint8_t value) {
	MRF_CTL = 0x04; // Assert chip select
	sleep_50ns();
	MRF_DATA = (reg << 1) | 0x01; // Send first byte (write command plus address)
	while (MRF_CTL & 0x01); // Wait for bus idle
	MRF_DATA = value; // Send second byte (value)
	while (MRF_CTL & 0x01); // Wait for bus idle
	sleep_50ns();
	MRF_CTL = 0x06; // Deassert chip select
}

uint8_t mrf_read_long(uint16_t reg) {
	MRF_CTL = 0x04; // Assert chip select
	sleep_50ns();
	MRF_DATA = (reg >> 3) | 0x80; // Send first byte (start of address)
	while (MRF_CTL & 0x01); // Wait for bus idle
	MRF_DATA = reg << 5; // Send second byte (rest of address plus read command)
	while (MRF_CTL & 0x01); // Wait for bus idle
	MRF_DATA = 0x00; // Send third byte (dummy for receive)
	while (MRF_CTL & 0x01); // Wait for bus idle
	sleep_50ns();
	MRF_CTL = 0x06; // Deassert chip select
	return MRF_DATA; // Return read data
}

void mrf_write_long(uint16_t reg, uint8_t value) {
	MRF_CTL = 0x04; // Assert chip select
	sleep_50ns();
	MRF_DATA = (reg >> 3) | 0x80; // Send first byte (start of address)
	while (MRF_CTL & 0x01); // Wait for bus idle
	MRF_DATA = (reg << 5) | 0x10; // Send second byte (rest of address plus write command)
	while (MRF_CTL & 0x01); // Wait for bus idle
	MRF_DATA = value; // Send third byte (value)
	while (MRF_CTL & 0x01); // Wait for bus idle
	sleep_50ns();
	MRF_CTL = 0x06; // Deassert chip select
}

void mrf_common_init(uint8_t channel, bool symbol_rate, uint16_t pan_id, uint64_t mac_address) {
	struct init_elt_long {
		uint16_t address;
		uint8_t data;
	};
	struct init_elt_short {
		uint8_t address;
		uint8_t data;
	};
	static const struct init_elt_long INIT_ELTS1[] = {
		{ MRF_REG_SHORT_SOFTRST, 0x07 },
		{ MRF_REG_SHORT_PACON2, 0x98 },
		{ MRF_REG_SHORT_TXSTBL, 0x95 },
		{ MRF_REG_LONG_RFCON0, 0x03 },
		{ MRF_REG_LONG_RFCON1, 0x02 },
		{ MRF_REG_LONG_RFCON2, 0x80 },
		{ MRF_REG_LONG_RFCON6, 0x90 },
		{ MRF_REG_LONG_RFCON7, 0x80 },
		{ MRF_REG_LONG_RFCON8, 0x10 },
		{ MRF_REG_LONG_SLPCON0, 0x03 },
		{ MRF_REG_LONG_SLPCON1, 0x21 },
		{ MRF_REG_SHORT_RXFLUSH, 0x61 },
		{ MRF_REG_SHORT_BBREG2, 0x80 },
		{ MRF_REG_SHORT_CCAEDTH, 0x60 },
		{ MRF_REG_SHORT_BBREG6, 0x40 },
	};
	for (uint8_t i = 0; i < sizeof(INIT_ELTS1) / sizeof(*INIT_ELTS1); ++i) {
		if (INIT_ELTS1[i].address >= 0x200) {
			mrf_write_long(INIT_ELTS1[i].address, INIT_ELTS1[i].data);
		} else {
			mrf_write_short(INIT_ELTS1[i].address, INIT_ELTS1[i].data);
		}
	}
	mrf_write_long(MRF_REG_LONG_RFCON0, ((channel - 0x0B) << 4) | 0x03);
	mrf_write_short(MRF_REG_SHORT_RFCTL, 0x04);
	mrf_write_short(MRF_REG_SHORT_RFCTL, 0x00);
	sleep_short();
	if (symbol_rate) {
		mrf_write_short(MRF_REG_SHORT_BBREG0, 0x01);
		mrf_write_short(MRF_REG_SHORT_BBREG3, 0x34);
		mrf_write_short(MRF_REG_SHORT_BBREG4, 0x5C);
		mrf_write_short(MRF_REG_SHORT_SOFTRST, 0x02);
	}
	mrf_write_short(MRF_REG_SHORT_PANIDL, pan_id);
	mrf_write_short(MRF_REG_SHORT_PANIDH, pan_id >> 8);
	for (uint8_t i = 0; i < 8; ++i) {
		mrf_write_short(MRF_REG_SHORT_EADR0 + i, mac_address);
		mac_address >>= 8;
	}
	mrf_analogue_off();
	mrf_write_short(MRF_REG_SHORT_TRISGPIO, 0x3F);
}

void mrf_analogue_off(void) {
	mrf_write_short(MRF_REG_SHORT_GPIO, 0x00);
	mrf_write_long(MRF_REG_LONG_TESTMODE, 0x08);
}

void mrf_analogue_rx(void) {
	mrf_write_short(MRF_REG_SHORT_GPIO, 0x04);
	mrf_write_long(MRF_REG_LONG_TESTMODE, 0x08);
}

void mrf_analogue_txrx(void) {
	mrf_write_short(MRF_REG_SHORT_GPIO, 0x00);
	mrf_write_long(MRF_REG_LONG_TESTMODE, 0x0F);
}

