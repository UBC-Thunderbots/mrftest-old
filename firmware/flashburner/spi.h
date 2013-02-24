#ifndef SPI_H
#define SPI_H

#include <stddef.h>
#include <stdint.h>

void spi_init(void);

struct spi_ops {
	void (*enable)(void);
	void (*disable)(void);
	void (*assert_cs)(void);
	void (*deassert_cs)(void);
	uint8_t (*transceive_byte)(uint8_t);
	void (*read_bytes)(void *, size_t);
	void (*write_bytes)(const void *, size_t);
};

extern const struct spi_ops spi_internal_ops, spi_external_ops;

#endif

