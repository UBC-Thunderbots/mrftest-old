#include "dongle_proto_out.h"
#include "buffers.h"
#include "critsec.h"
#include "queue.h"
#include "stack.h"
#include "usb.h"
#include <pic18fregs.h>
#include <stdbool.h>

/**
 * \brief Metadata about a USB receive buffer.
 */
typedef struct rxbuf_info {
	/**
	 * \brief The number of allocated micropackets referring to substrings of this buffer.
	 */
	uint8_t refs;

	/**
	 * \brief The next free receive buffer.
	 */
	__data struct rxbuf_info *next;
} rxbuf_info_t;

QUEUE_DEFINE_TYPE(dongle_proto_out_micropacket_t);

STACK_DEFINE_TYPE(dongle_proto_out_micropacket_t);

STACK_DEFINE_TYPE(rxbuf_info_t);

/**
 * \brief Whether or not the subsystem is initialized.
 */
static BOOL inited = false;

/**
 * \brief All the USB receive buffer metadata structures.
 */
__data static rxbuf_info_t rxbuf_infos[NUM_DONGLE_PROTO_OUT_BUFFERS];

/**
 * \brief The free USB receive buffer metadata structures.
 */
static STACK_TYPE(rxbuf_info_t) free_rxbuf_infos;

/**
 * \brief The USB receive buffer metadata structures corresponding to the packets currently granted to the SIE.
 */
__data static rxbuf_info_t *rxbuf_infos_sie[2];

/**
 * \brief The micropacket structures.
 */
__data static dongle_proto_out_micropacket_t micropackets[NUM_DONGLE_PROTO_OUT_BUFFERS * 8];

/**
 * \brief The free micropacket structures.
 */
static STACK_TYPE(dongle_proto_out_micropacket_t) free_micropackets;

/**
 * \brief The filled micropackets awaiting delivery to the application.
 */
static QUEUE_TYPE(dongle_proto_out_micropacket_t) pending_micropackets;

static void check_sie(void) {
	__data rxbuf_info_t *rxbuf;
	uint8_t packet;

	if (inited) {
		if (!usb_bdpairs[4].out.BDSTATbits.sie.UOWN) {
			if (usb_halted_out_endpoints & (1 << 4)) {
				usb_bdpairs[4].out.BDSTAT = BDSTAT_UOWN | BDSTAT_BSTALL;
			} else {
				if ((rxbuf = STACK_TOP(free_rxbuf_infos))) {
					STACK_POP(free_rxbuf_infos);
					packet = rxbuf - rxbuf_infos;
					usb_bdpairs[4].out.BDADR = dongle_proto_out_buffers[packet];
					usb_bdpairs[4].out.BDCNT = 64;
					if (usb_bdpairs[4].out.BDSTATbits.sie.OLDDTS) {
						usb_bdpairs[4].out.BDSTAT = BDSTAT_UOWN | BDSTAT_DTSEN;
					} else {
						usb_bdpairs[4].out.BDSTAT = BDSTAT_UOWN | BDSTAT_DTS | BDSTAT_DTSEN;
					}
					rxbuf_infos_sie[0] = rxbuf;
				}
			}
		}
		if (!usb_bdpairs[5].out.BDSTATbits.sie.UOWN) {
			if (usb_halted_out_endpoints & (1 << 5)) {
				usb_bdpairs[5].out.BDSTAT = BDSTAT_UOWN | BDSTAT_BSTALL;
			} else {
				if ((rxbuf = STACK_TOP(free_rxbuf_infos))) {
					STACK_POP(free_rxbuf_infos);
					packet = rxbuf - rxbuf_infos;
					usb_bdpairs[5].out.BDADR = dongle_proto_out_buffers[packet];
					usb_bdpairs[5].out.BDCNT = 64;
					if (usb_bdpairs[5].out.BDSTATbits.sie.OLDDTS) {
						usb_bdpairs[5].out.BDSTAT = BDSTAT_UOWN | BDSTAT_DTSEN;
					} else {
						usb_bdpairs[5].out.BDSTAT = BDSTAT_UOWN | BDSTAT_DTS | BDSTAT_DTSEN;
					}
					rxbuf_infos_sie[1] = rxbuf;
				}
			}
		}
	}
}

static BOOL parse_micropackets(__data rxbuf_info_t *rxbuf_info, uint8_t len) {
	uint8_t packet = rxbuf_info - rxbuf_infos;
	__data uint8_t *ptr = dongle_proto_out_buffers[packet];
	uint8_t micropacket_len;
	__data dongle_proto_out_micropacket_t *micropacket;

	while (len) {
		micropacket_len = (*ptr) & 0x3F;
		micropacket = STACK_TOP(free_micropackets);
		if (!micropacket) {
			return false;
		}
		STACK_POP(free_micropackets);
		micropacket->ptr = ptr;
		micropacket->packet = packet;
		QUEUE_PUSH(pending_micropackets, micropacket);

		ptr += micropacket_len;
		len -= micropacket_len;
		++rxbuf_info->refs;
	}

	return true;
}

static void on_out4(void) {
	__data rxbuf_info_t *rxbuf = rxbuf_infos_sie[0];
	rxbuf_infos_sie[0] = 0;
	if (usb_bdpairs[4].out.BDCNT) {
		if (parse_micropackets(rxbuf, usb_bdpairs[4].out.BDCNT)) {
			check_sie();
		} else {
			usb_halted_out_endpoints |= 1 << 4;
			dongle_proto_out_halt(4);
		}
	} else {
		STACK_PUSH(free_rxbuf_infos, rxbuf);
		check_sie();
	}
}

static void on_out5(void) {
	__data rxbuf_info_t *rxbuf = rxbuf_infos_sie[1];
	rxbuf_infos_sie[1] = 0;
	if (usb_bdpairs[5].out.BDCNT) {
		if (parse_micropackets(rxbuf, usb_bdpairs[5].out.BDCNT)) {
			check_sie();
		} else {
			usb_halted_out_endpoints |= 1 << 5;
			dongle_proto_out_halt(5);
		}
	} else {
		STACK_PUSH(free_rxbuf_infos, rxbuf);
		check_sie();
	}
}

void dongle_proto_out_init(void) {
	uint8_t i;

	if (!inited) {
		/* Fill in the metadata structures and put them into linked lists. */
		STACK_INIT(free_rxbuf_infos);
		for (i = 0; i != NUM_DONGLE_PROTO_OUT_BUFFERS; ++i) {
			rxbuf_infos[i].refs = 0;
			STACK_PUSH(free_rxbuf_infos, &rxbuf_infos[i]);
		}
		rxbuf_infos_sie[0] = rxbuf_infos_sie[1] = 0;
		STACK_INIT(free_micropackets);
		for (i = 0; i != NUM_DONGLE_PROTO_OUT_BUFFERS * 8; ++i) {
			STACK_PUSH(free_micropackets, &micropackets[i]);
		}
		QUEUE_INIT(pending_micropackets);

		/* Set up the USB stuff. */
		usb_ep_callbacks[4].out = &on_out4;
		usb_ep_callbacks[5].out = &on_out5;
		usb_bdpairs[4].out.BDSTAT = BDSTAT_DTS;
		usb_bdpairs[5].out.BDSTAT = BDSTAT_DTS;
		UEP4bits.EPHSHK = 1;
		UEP4bits.EPOUTEN = 1;
		UEP5bits.EPHSHK = 1;
		UEP5bits.EPOUTEN = 1;

		/* Record state. */
		inited = true;

		/* Submit buffers to the SIE. */
		check_sie();
	}
}

void dongle_proto_out_deinit(void) {
	if (inited) {
		UEP4bits.EPOUTEN = 0;
		UEP5bits.EPOUTEN = 0;
		usb_bdpairs[4].out.BDSTAT = 0;
		usb_bdpairs[5].out.BDSTAT = 0;
	}
}

void dongle_proto_out_halt(uint8_t ep) {
	usb_bdpairs[ep].out.BDSTAT = BDSTAT_UOWN | BDSTAT_BSTALL;
	if (rxbuf_infos_sie[ep - 4]) {
		STACK_PUSH(free_rxbuf_infos, rxbuf_infos_sie[ep - 4]);
		rxbuf_infos_sie[ep - 4] = 0;
	}
}

void dongle_proto_out_unhalt(uint8_t ep) {
	usb_bdpairs[ep & 0x7F].out.BDSTAT = BDSTAT_DTS;
	check_sie();
}

__data dongle_proto_out_micropacket_t *dongle_proto_out_get(void) {
	__data dongle_proto_out_micropacket_t *ret;
	CRITSEC_DECLARE(cs);

	CRITSEC_ENTER(cs);

	if (inited) {
		ret = QUEUE_FRONT(pending_micropackets);
		QUEUE_POP(pending_micropackets);
	} else {
		ret = 0;
	}

	CRITSEC_LEAVE(cs);

	return ret;
}

void dongle_proto_out_free(__data dongle_proto_out_micropacket_t *micropacket) {
	CRITSEC_DECLARE(cs);

	CRITSEC_ENTER(cs);

	if (!--rxbuf_infos[micropacket->packet].refs) {
		STACK_PUSH(free_rxbuf_infos, &rxbuf_infos[micropacket->packet]);
		check_sie();
	}

	STACK_PUSH(free_micropackets, micropacket);

	CRITSEC_LEAVE(cs);
}

