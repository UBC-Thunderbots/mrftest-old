#include <core_progmem.h>
#include <registers/flash.h>

uint32_t core_progmem_dump[256 * 1024 / 4] __attribute__((section(".coredump")));
static volatile uint32_t *pointer;

static void start(void) {
	// Enable Flash writing.
	FLASH_KEYR = 0x45670123;
	FLASH_KEYR = 0xCDEF89AB;

	// Clear any pending errors.
	while (FLASH_SR.BSY);
	FLASH_SR = FLASH_SR;

	// Erase sectors 10 and 11 which are where we keep core dumps.
	{
		FLASH_CR_t tmp = {
			.LOCK = 0,
			.ERRIE = 0,
			.EOPIE = 0,
			.STRT = 0,
			.PSIZE = 2,
			.SNB = 10,
			.MER = 0,
			.SER = 1,
			.PG = 0,
		};
		FLASH_CR = tmp;
		tmp.STRT = 1;
		FLASH_CR = tmp;
		while (FLASH_SR.BSY);
		tmp.STRT = 0;
		tmp.SNB = 11;
		FLASH_CR = tmp;
		tmp.STRT = 1;
		FLASH_CR = tmp;
		while (FLASH_SR.BSY);
	}

	// Enable Flash programming.
	FLASH_CR.PG = 1;

	// Set up the pointer.
	pointer = core_progmem_dump;
}

static void write(const void *data, size_t length) {
	const uint32_t *source = data;
	while (length) {
		*pointer = *source;
		while (FLASH_SR.BSY);
		if (length > 4) {
			length -= 4;
		} else {
			length = 0;
		}
		++pointer;
		++source;
	}
}

static bool end(void) {
	// Check if anything failed.
	bool failed = !!FLASH_SR.PGSERR || !!FLASH_SR.PGPERR || !!FLASH_SR.PGAERR || !!FLASH_SR.WRPERR;

	// Relock the Flash programming interface.
	{
		FLASH_CR_t tmp = {
			.LOCK = 1,
			.ERRIE = 0,
			.EOPIE = 0,
			.STRT = 0,
			.PSIZE = 2,
			.SNB = 0,
			.MER = 0,
			.SER = 0,
			.PG = 0,
		};
		FLASH_CR = tmp;
	}

	return !failed;
}

const exception_core_writer_t core_progmem_writer = {
	.start = &start,
	.write = &write,
	.end = &end,
};

