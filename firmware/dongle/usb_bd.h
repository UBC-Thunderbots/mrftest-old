#ifndef USB_BD_H
#define USB_BD_H

/**
 * \file
 *
 * \brief Contains definitions of the buffer descriptors and utilities to work with them.
 */

#include <stdbool.h>
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
 * \brief An outbound buffer descriptor.
 */
typedef struct {
	union {
		usb_bdstat_bits_t BDSTATbits;
		uint8_t BDSTAT;
	};
	uint8_t BDCNT;
	__data void *BDADR;
} usb_out_bd_t;

/**
 * \brief An inbound buffer descriptor.
 */
typedef struct {
	union {
		usb_bdstat_bits_t BDSTATbits;
		uint8_t BDSTAT;
	};
	uint8_t BDCNT;
	__data const void *BDADR;
} usb_in_bd_t;

/**
 * \brief A pair of buffer descriptors corresponding to an endpoint number.
 */
typedef struct {
	usb_out_bd_t out;
	usb_in_bd_t in;
} usb_bdpair_t;

/**
 * \brief The buffer descriptors.
 */
extern __data volatile usb_bdpair_t __at(0x400) usb_bdpairs[USB_CONFIG_MAX_ENDPOINT + 1];



/**
 * \brief Initializes an inbound endpoint's buffer descriptor(s) and makes it (them) ready for use.
 *
 * This macro should be invoked before setting the \c EPINEN bit of the relevant \c UEP.
 *
 * \param[in] ep the endpoint number.
 */
#define USB_BD_IN_INIT(ep) do { usb_bdpairs[(ep)].in.BDSTAT = BDSTAT_DTS; } while (0)

/**
 * \brief Checks if there is a free buffer descriptor to submit a new inbound transaction.
 *
 * \param[in] ep the endpoint number.
 *
 * \return \c true if there is a free BD, or \c false if not.
 */
#define USB_BD_IN_HAS_FREE(ep) (!usb_bdpairs[(ep)].in.BDSTATbits.sie.UOWN)

/**
 * \brief Submits a buffer into an inbound endpoint's buffer descriptor.
 *
 * \pre USB_BD_IN_HAS_FREE(ep)
 *
 * \param[in] ep the endpoint number.
 *
 * \param[in] buf the buffer containing the data to send.
 *
 * \param[in] len the number of bytes to send.
 */
#define USB_BD_IN_SUBMIT(ep, buf, len) do { \
	usb_bdpairs[(ep)].in.BDADR = (buf); \
	usb_bdpairs[(ep)].in.BDCNT = (len); \
	if (usb_bdpairs[(ep)].in.BDSTATbits.sie.OLDDTS) { \
		usb_bdpairs[(ep)].in.BDSTAT = BDSTAT_UOWN | BDSTAT_DTSEN; \
	} else { \
		usb_bdpairs[(ep)].in.BDSTAT = BDSTAT_UOWN | BDSTAT_DTS | BDSTAT_DTSEN; \
	} \
} while (0)

/**
 * \brief Sets a commanded stall on an inbound endpoint.
 *
 * \pre This macro may only be invoked in the endpoint's commanded halt callback.
 *
 * \param[in] ep the endpoint number.
 */
#define USB_BD_IN_COMMANDED_STALL(ep) do { usb_bdpairs[(ep)].in.BDSTAT = BDSTAT_UOWN | BDSTAT_BSTALL; } while (0)

/**
 * \brief Checks whether a non-commanded functional stall can be placed on an inbound endpoint.
 *
 * \param[in] ep the endpoint number.
 *
 * \return \c true if the endpoint can be stalled, or \c false if not (e.g. because other transactions are already queued).
 */
#define USB_BD_IN_CAN_FUNCTIONAL_STALL(ep) USB_BD_IN_HAS_FREE(ep)

/**
 * \brief Sets a non-commanded functional stall on an inbound endpoint.
 *
 * \pre USB_BD_IN_CAN_FUNCTIONAL_STALL(ep)
 *
 * \param[in] ep the endpoint number.
 */
#define USB_BD_IN_FUNCTIONAL_STALL(ep) do { usb_bdpairs[(ep)].in.BDSTAT = BDSTAT_UOWN | BDSTAT_BSTALL; } while (0)

/**
 * \brief Unstalls an inbound endpoint.
 *
 * After clearing the stall condition, the endpoint is ready to accept submissions.
 *
 * \pre This macro may only be invoked in the endpoint's unhalt callback.
 *
 * \param[in] ep the endpoint number.
 */
#define USB_BD_IN_UNSTALL(ep) do { usb_bdpairs[(ep)].in.BDSTAT = BDSTAT_DTS; } while (0)

/**
 * \brief Returns the number of bytes just sent on an inbound endpoint.
 *
 * \pre This macro may only be invoked in a transaction callback.
 *
 * \param[in] ep the endpoint number.
 *
 * \return the byte count.
 */
#define USB_BD_IN_SENT(ep) (usb_bdpairs[(ep)].in.BDCNT)



/**
 * \brief Initializes an outbound endpoint's buffer descriptor(s) and makes it (them) ready for use.
 *
 * One will most likely want to call USB_BD_OUT_SUBMIT() after clearing the BD(s).
 *
 * \param[in] ep the endpoint number.
 */
#define USB_BD_OUT_INIT(ep) do { usb_bdpairs[(ep)].out.BDSTAT = BDSTAT_DTS; } while (0)

/**
 * \brief Checks if there is a free buffer descriptor to submit a new buffer to fill.
 *
 * \param[in] ep the endpoint number.
 *
 * \return \c true if there is a free BD, or \c false if not.
 */
#define USB_BD_OUT_HAS_FREE(ep) (!usb_bdpairs[(ep)].out.BDSTATbits.sie.UOWN)

/**
 * \brief Submits a buffer into an outbound endpoint's buffer descriptor.
 *
 * \pre USB_BD_OUT_HAS_FREE(ep)
 *
 * \param[in] ep the endpoint number.
 *
 * \param[out] buf the buffer to store the received data into.
 *
 * \param[in] len the length of the buffer.
 */
#define USB_BD_OUT_SUBMIT(ep, buf, len) do { \
	usb_bdpairs[(ep)].out.BDADR = (buf); \
	usb_bdpairs[(ep)].out.BDCNT = (len); \
	if (usb_bdpairs[(ep)].out.BDSTATbits.sie.OLDDTS) { \
		usb_bdpairs[(ep)].out.BDSTAT = BDSTAT_UOWN | BDSTAT_DTSEN; \
	} else { \
		usb_bdpairs[(ep)].out.BDSTAT = BDSTAT_UOWN | BDSTAT_DTS | BDSTAT_DTSEN; \
	} \
} while (0)

/**
 * \brief Sets a commanded stall on an outbound endpoint.
 *
 * \pre This macro may only be invoked in the endpoint's commanded halt callback.
 *
 * \param[in] ep the endpoint number.
 */
#define USB_BD_OUT_COMMANDED_STALL(ep) do { usb_bdpairs[(ep)].out.BDSTAT = BDSTAT_UOWN | BDSTAT_BSTALL; } while (0)

/**
 * \brief Checks whether a non-commanded functional stall can be placed on an outbound endpoint.
 *
 * \param[in] ep the endpoint number.
 *
 * \return \c true if the endpoint can be stalled, or \c false if not (e.g. because other transactions are already queued).
 */
#define USB_BD_OUT_CAN_FUNCTIONAL_STALL(ep) USB_BD_OUT_HAS_FREE(ep)

/**
 * \brief Sets a non-commanded functional stall on an outbound endpoint.
 *
 * \pre USB_OUT_CAN_FUNCTIONAL_STALL(ep)
 *
 * \param[in] ep the endpoint number.
 */
#define USB_BD_OUT_FUNCTIONAL_STALL(ep) do { usb_bdpairs[(ep)].out.BDSTAT = BDSTAT_UOWN | BDSTAT_BSTALL; } while (0)

/**
 * \brief Unstalls an outbound endpoint.
 *
 * After clearing the stall condition, the endpoint is ready to accept submissions.
 *
 * \pre This macro may only be invoked in the endpoint's unhalt callback.
 *
 * \param[in] ep the endpoint number.
 */
#define USB_BD_OUT_UNSTALL(ep) do { usb_bdpairs[(ep)].out.BDSTAT = BDSTAT_DTS; } while (0)

/**
 * \brief In a transaction callback, returns the number of bytes just received on an outbound endpoint.
 *
 * \param[in] ep the endpoint number.
 *
 * \return the byte count.
 */
#define USB_BD_OUT_RECEIVED(ep) (usb_bdpairs[(ep)].out.BDCNT)

#endif

