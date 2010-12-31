#ifndef USB_BD_H
#define USB_BD_H

/**
 * \file
 *
 * \brief Contains definitions of the buffer descriptors and utilities to work with them.
 */

#include <stdint.h>

/**
 * \brief The bitmasks of the BDSTAT bits.
 */
enum {
	BDSTAT_UOWN = 0x80,
	BDSTAT_DTS = 0x40,
	BDSTAT_DTSEN = 0x08,
	BDSTAT_BSTALL = 0x04,
	BDSTAT_BC9 = 0x02,
	BDSTAT_BC8 = 0x01,
};

/**
 * \brief The layout of a BDSTAT when written by the CPU.
 */
typedef struct {
	unsigned BC8 : 1;
	unsigned BC9 : 1;
	unsigned BSTALL : 1;
	unsigned DTSEN : 1;
	unsigned : 2;
	unsigned DTS : 1;
	unsigned UOWN : 1;
} usb_bdstat_cpu_bits_t;

/**
 * \brief The layout of a BDSTAT when written by the SIE.
 */
typedef struct {
	unsigned BC8 : 1;
	unsigned BC9 : 1;
	unsigned PID : 4;
	unsigned OLDDTS : 1;
	unsigned UOWN : 1;
} usb_bdstat_sie_bits_t;

/**
 * \brief A BDSTAT.
 */
typedef union {
	usb_bdstat_cpu_bits_t cpu;
	usb_bdstat_sie_bits_t sie;
} usb_bdstat_bits_t;

/**
 * \brief A buffer descriptor.
 */
typedef struct {
	union {
		usb_bdstat_bits_t BDSTATbits;
		uint8_t BDSTAT;
	};
	uint8_t BDCNT;
	__data void *BDADR;
} usb_bd_t;

/**
 * \brief A pair of buffer descriptors corresponding to an endpoint number.
 */
typedef struct {
	usb_bd_t out;
	usb_bd_t in;
} usb_bdpair_t;

/**
 * \brief The buffer descriptors.
 */
extern __data volatile usb_bdpair_t __at(0x400) usb_bdpairs[USB_CONFIG_MAX_ENDPOINT + 1];

#endif

