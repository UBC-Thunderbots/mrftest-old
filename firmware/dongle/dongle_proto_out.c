#include "dongle_proto_out.h"
#include "buffers.h"
#include "critsec.h"
#include "usb.h"
#include <pic18fregs.h>
#include <stdbool.h>

/**
 * \brief Whether or not the subsystem is initialized.
 */
static BOOL inited = false;

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

/**
 * \brief All the USB receive buffer metadata structures.
 */
__data static rxbuf_info_t rxbuf_infos[NUM_DONGLE_PROTO_OUT_BUFFERS];

/**
 * \brief The first free USB receive buffer metadata structure.
 */
__data static rxbuf_info_t * volatile first_free_rxbuf_info;

/**
 * \brief The USB receive buffer metadata structures corresponding to the packets currently granted to the SIE.
 */
__data static rxbuf_info_t *rxbuf_infos_sie[2];

/**
 * \brief The micropacket structures.
 */
__data static dongle_proto_out_micropacket_t micropackets[NUM_DONGLE_PROTO_OUT_BUFFERS * 8];

/**
 * \brief The first free micropacket structure.
 */
__data static dongle_proto_out_micropacket_t * volatile first_free_micropacket;

/**
 * \brief The oldest pending micropacket structure.
 */
__data static dongle_proto_out_micropacket_t * volatile oldest_pending_micropacket;

/**
 * \brief The newest pending micropacket structure.
 */
__data static dongle_proto_out_micropacket_t * volatile newest_pending_micropacket;

static void check_sie(void) {
	__data rxbuf_info_t *rxbuf;
	uint8_t packet;

	if (inited) {
		if (!usb_bdpairs[4].out.BDSTATbits.sie.UOWN) {
			if (usb_halted_out_endpoints & (1 << 4)) {
				usb_bdpairs[4].out.BDSTAT = BDSTAT_UOWN | BDSTAT_BSTALL;
			} else {
				if (first_free_rxbuf_info) {
					rxbuf = first_free_rxbuf_info;
					first_free_rxbuf_info = rxbuf->next;
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
				if (first_free_rxbuf_info) {
					rxbuf = first_free_rxbuf_info;
					first_free_rxbuf_info = rxbuf->next;
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
		micropacket = first_free_micropacket;
		if (!micropacket) {
			return false;
		}
		first_free_micropacket = micropacket->next;
		micropacket->ptr = ptr;
		micropacket->packet = packet;
		micropacket->next = 0;
		if (newest_pending_micropacket) {
			newest_pending_micropacket->next = micropacket;
			newest_pending_micropacket = micropacket;
		} else {
			newest_pending_micropacket = oldest_pending_micropacket = micropacket;
		}

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
		rxbuf->next = first_free_rxbuf_info;
		first_free_rxbuf_info = rxbuf;
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
		rxbuf->next = first_free_rxbuf_info;
		first_free_rxbuf_info = rxbuf;
		check_sie();
	}
}

void dongle_proto_out_init(void) {
	uint8_t i;

	if (!inited) {
		/* Fill in the metadata structures and put them into linked lists. */
		for (i = 0; i != NUM_DONGLE_PROTO_OUT_BUFFERS; ++i) {
			rxbuf_infos[i].refs = 0;
			rxbuf_infos[i].next = &rxbuf_infos[i + 1];
		}
		first_free_rxbuf_info = &rxbuf_infos[0];
		rxbuf_infos[NUM_DONGLE_PROTO_OUT_BUFFERS - 1].next = 0;
		rxbuf_infos_sie[0] = rxbuf_infos_sie[1] = 0;
		for (i = 0; i != NUM_DONGLE_PROTO_OUT_BUFFERS * 8; ++i) {
			micropackets[i].next = &micropackets[i + 1];
		}
		first_free_micropacket = &micropackets[0];
		micropackets[NUM_DONGLE_PROTO_OUT_BUFFERS * 8 - 1].next = 0;
		oldest_pending_micropacket = newest_pending_micropacket = 0;

		/* Set up the USB stuff. */
		usb_ep_callbacks[4].out = &on_out4;
		usb_ep_callbacks[5].out = &on_out5;
		usb_bdpairs[4].out.BDSTAT = BDSTAT_DTS;
		usb_bdpairs[5].out.BDSTAT = BDSTAT_DTS;
		UEP4bits.EPHSHK = 1;
		UEP4bits.EPCONDIS = 1;
		UEP4bits.EPOUTEN = 1;
		UEP5bits.EPHSHK = 1;
		UEP5bits.EPCONDIS = 1;
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
		rxbuf_infos_sie[ep - 4]->next = first_free_rxbuf_info;
		first_free_rxbuf_info = rxbuf_infos_sie[ep - 4];
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
		ret = oldest_pending_micropacket;
		if (ret) {
			oldest_pending_micropacket = ret->next;
			if (!ret->next) {
				newest_pending_micropacket = 0;
			}
			ret->next = 0;
		}
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
		rxbuf_infos[micropacket->packet].next = first_free_rxbuf_info;
		first_free_rxbuf_info = &rxbuf_infos[micropacket->packet];
		check_sie();
	}

	micropacket->next = first_free_micropacket;
	first_free_micropacket = micropacket;

	CRITSEC_LEAVE(cs);
}

