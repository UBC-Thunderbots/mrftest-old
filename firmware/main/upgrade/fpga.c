#include "fpga.h"
#include "internal.h"
#include "../dma.h"
#include "../icb.h"
#include "../sdcard.h"
#include <minmax.h>

/**
 * \brief The length of the FPGA data, when checked.
 */
static size_t upgrade_fpga_length;

bool upgrade_fpga_check(void) {
	uint32_t flags, crc;

	return upgrade_int_check_area(UPGRADE_FPGA_FIRST_SECTOR, UPGRADE_FPGA_MAGIC, &upgrade_fpga_length, &flags, &crc);
}

bool upgrade_fpga_send(void) {
	dma_memory_handle_t block_buffer_handle = dma_alloc(SD_SECTOR_SIZE);
	void *block_buffer = dma_get_buffer(block_buffer_handle);
	bool ok = true;
	uint32_t next_sector = UPGRADE_FPGA_FIRST_SECTOR + 1;
	size_t length_left = upgrade_fpga_length;
	while (ok && length_left) {
		ok = sd_read(next_sector++, block_buffer) == SD_STATUS_OK;
		if (ok) {
			icb_conf_block(block_buffer, MIN(SD_SECTOR_SIZE, length_left));
			length_left -= MIN(SD_SECTOR_SIZE, length_left);
		}
	}
	dma_free(block_buffer_handle);
	return ok;
}
