#include <crc32.h>
#include <rcc.h>
#include <registers/crc.h>

/**
 * \brief Initializes the CRC32 module.
 */
void crc32_init(void) {
	rcc_enable_reset(AHB1, CRC);
}

/**
 * \brief Computes the CRC32 of a block of data.
 *
 * The CRC is computed as though the data will be transmitted MSb-first.
 *
 * \warning This function is not reentrant.
 *
 * \param[in] data the data block to process
 * \param[in] length the number of bytes
 * \param[in] initial the initial CRC to accumulate onto
 * \return the CRC32 value
 */
uint32_t crc32_be(const void *data, size_t length, uint32_t initial) {
	const CRC_CR_t reset_cr = { .RESET = 1 };
	uint32_t acc = initial;
	const uint8_t *bptr = data;

	while (length && ((uintptr_t) bptr & 3U)) {
		CRC.CR = reset_cr;
		asm volatile("dmb");
		acc = acc ^ ((uint32_t) *bptr << 24U);
		CRC.DR = (acc >> 24U) | 0xFFFFFF00U;
		acc = (acc << 8U) ^ CRC.DR ^ 0xFFU;
		--length;
		++bptr;
	}

	const uint32_t *wptr = (const uint32_t *) bptr;
	if (length >= 4U) {
		CRC.CR = reset_cr;
		asm volatile("dmb");
		CRC.DR = acc ^ __builtin_bswap32(*wptr);
		++wptr;
		length -= 4U;
		while (length >= 4U) {
			CRC.DR = __builtin_bswap32(*wptr);
			++wptr;
			length -= 4U;
		}
		acc = ~CRC.DR;
	}

	bptr = (const uint8_t *) wptr;
	while (length) {
		CRC.CR = reset_cr;
		asm volatile("dmb");
		acc = acc ^ ((uint32_t) *bptr << 24U);
		CRC.DR = (acc >> 24U) | 0xFFFFFF00U;
		acc = (acc << 8U) ^ CRC.DR ^ 0xFFU;
		--length;
		++bptr;
	}

	return acc;
}
