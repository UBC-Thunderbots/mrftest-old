#include "usb.h"
#include "usb_internal.h"
#include <pic18fregs.h>
#include <stdint.h>
#include <string.h>

/**
 * \file
 *
 * \brief Implements endpoint zero operations.
 */

#if USB_CONFIG_NUM_EP0_IOVECS < 2
#error Number of EP0 iovecs must be at least 2
#endif

__data volatile usb_bdpair_t __at(0x400) usb_bdpairs[USB_CONFIG_MAX_ENDPOINT + 1];

usb_setup_packet_t usb_ep0_setup_buffer;

uint8_t usb_ep0_out_buffer[8];

uint8_t usb_ep0_in_buffer[8];

usb_iovec_t usb_ep0_data[USB_CONFIG_NUM_EP0_IOVECS];
uint8_t usb_ep0_data_length;
static uint8_t usb_ep0_next_iovec;

uint16_t usb_halted_in_endpoints, usb_halted_out_endpoints;

volatile uint8_t usb_current_configuration;

/**
 * \brief A small buffer for stashing miscellaneous bits and pieces of data to return to the host.
 */
static uint8_t scratch_buffer[8];

/**
 * \brief Loads some data and prepares the in buffer for transmission to the host.
 */
static void populate_in_data(void) {
	if (usb_ep0_next_iovec == usb_ep0_data_length) {
		/* There is no more data to send, but because we got here at all, the host is expecting more. Send a zero-length packet. */
		usb_bdpairs[0].in.BDCNT = 0;
		usb_bdpairs[0].in.BDADR = usb_ep0_in_buffer;
		if (usb_bdpairs[0].in.BDSTATbits.sie.OLDDTS) {
			usb_bdpairs[0].in.BDSTAT = BDSTAT_UOWN | BDSTAT_DTSEN;
		} else {
			usb_bdpairs[0].in.BDSTAT = BDSTAT_UOWN | BDSTAT_DTS | BDSTAT_DTSEN;
		}
		return;
	}

	usb_bdpairs[0].in.BDCNT = 0;
	while (usb_bdpairs[0].in.BDCNT != 8 && usb_ep0_next_iovec != usb_ep0_data_length && usb_ep0_setup_buffer.length) {
		/* We have (1) space in the buffer, (2) data to send, and (3) an allowance from the host to send data. Put data into the buffer. */
		uint8_t to_copy;
		if (usb_ep0_setup_buffer.length & 0xFF00) {
			to_copy = 255;
		} else {
			to_copy = usb_ep0_setup_buffer.length;
		}
		if (to_copy > (uint8_t) 8 - usb_bdpairs[0].in.BDCNT) {
			to_copy = 8 - usb_bdpairs[0].in.BDCNT;
		}
		if (to_copy > usb_ep0_data[usb_ep0_next_iovec].length) {
			to_copy = usb_ep0_data[usb_ep0_next_iovec].length;
		}
		memcpy(usb_ep0_in_buffer + usb_bdpairs[0].in.BDCNT, usb_ep0_data[usb_ep0_next_iovec].ptr, to_copy);
		usb_bdpairs[0].in.BDCNT += to_copy;
		usb_ep0_data[usb_ep0_next_iovec].ptr = ((const char *) usb_ep0_data[usb_ep0_next_iovec].ptr) + to_copy;
		usb_ep0_data[usb_ep0_next_iovec].length -= to_copy;
		usb_ep0_setup_buffer.length -= to_copy;

		if (!usb_ep0_data[usb_ep0_next_iovec].length) {
			/* We have exhausted all the data in this iovec. Move to the next one. */
			++usb_ep0_next_iovec;
		}
	}

	if (usb_bdpairs[0].in.BDCNT != 8) {
		/* We have run out of data to send and will be sending a short packet. We do NOT want to later send a zero-length packet! */
		usb_ep0_setup_buffer.length = 0;
	}

	/* Arm the buffer. */
	usb_bdpairs[0].in.BDADR = usb_ep0_in_buffer;
	if (usb_bdpairs[0].in.BDSTATbits.sie.OLDDTS) {
		usb_bdpairs[0].in.BDSTAT = BDSTAT_UOWN | BDSTAT_DTSEN;
	} else {
		usb_bdpairs[0].in.BDSTAT = BDSTAT_UOWN | BDSTAT_DTS | BDSTAT_DTSEN;
	}
}

/**
 * \brief Handles a SETUP transaction finishing on endpoint zero.
 */
static void on_setup(void) {
	/* Clear stuff. */
	BOOL ok = false;
	usb_ep0_next_iovec = 0;
	usb_ep0_data_length = 0;

	/* Copy the received packet into the SETUP packet buffer. */
	memcpyram2ram(&usb_ep0_setup_buffer, usb_ep0_out_buffer, sizeof(usb_ep0_setup_buffer));

	/* If there is to be no data stage, the effective "direction" is host-to-device since status stage is device-to-host. */
	if (!usb_ep0_setup_buffer.length) {
		usb_ep0_setup_buffer.request_type.bits.direction = 0;
	}

	/* Check if we have a custom handler interested in handling the transaction. */
	if (usb_devinfo->custom_ep0_setup_handler) {
		if (usb_devinfo->custom_ep0_setup_handler()) {
			ok = true;
		}
	}

	/* If there is no custom handler or it's not interested, check standard requests. */
	if (!ok && usb_ep0_setup_buffer.request_type.bits.type == USB_SETUP_PACKET_REQUEST_STANDARD) {
		switch (usb_ep0_setup_buffer.request_type.bits.recipient) {
			case USB_SETUP_PACKET_RECIPIENT_DEVICE:
				/* Addressed to the device as a whole. */
				switch (usb_ep0_setup_buffer.request) {
					case USB_SETUP_PACKET_STDREQ_GET_STATUS:
						scratch_buffer[0] = 0x00 | (USB_CONFIG_SELF_POWERED ? 0x01 : 0x00);
						scratch_buffer[1] = 0x00;
						usb_ep0_data[0].ptr = scratch_buffer;
						usb_ep0_data[0].length = 2;
						usb_ep0_data_length = 1;
						ok = true;
						break;

					case USB_SETUP_PACKET_STDREQ_SET_ADDRESS:
						ok = true;
						break;

					case USB_SETUP_PACKET_STDREQ_GET_DESCRIPTOR:
						switch (usb_ep0_setup_buffer.value >> 8) {
							case USB_DESCRIPTOR_DEVICE:
								usb_ep0_data[0].ptr = usb_devinfo->device_descriptor;
								usb_ep0_data[0].length = usb_devinfo->device_descriptor->length;
								usb_ep0_data_length = 1;
								ok = true;
								break;

							case USB_DESCRIPTOR_CONFIGURATION:
								if (((uint8_t) usb_ep0_setup_buffer.value) < usb_devinfo->device_descriptor->num_configurations) {
									__code const usb_configuration_descriptor_t *confdesc;
									confdesc = usb_devinfo->configurations[(uint8_t) usb_ep0_setup_buffer.value]->configuration_descriptor;
									usb_ep0_data[0].ptr = confdesc;
									usb_ep0_data[0].length = confdesc->length;
									usb_ep0_data[1].ptr = confdesc->following_bytes;
									usb_ep0_data[1].length = confdesc->total_length - confdesc->length;
									usb_ep0_data_length = 2;
									ok = true;
								}
								break;

#if USB_CONFIG_STRING_DESCRIPTORS
							case USB_DESCRIPTOR_STRING:
								if ((usb_ep0_setup_buffer.value & 0xFF) == 0) {
									usb_ep0_data[0].ptr = usb_devinfo->string_descriptor_zero;
									usb_ep0_data[0].length = *((__code const uint8_t *) usb_ep0_data[0].ptr);
									usb_ep0_data_length = 1;
									ok = true;
								} else if ((usb_ep0_setup_buffer.value & 0xFF) <= usb_devinfo->string_metatable->table_length) {
									uint8_t i;
									__code const uint16_t *p;
									for (i = 0; i < usb_devinfo->string_metatable->length; ++i) {
										if (usb_devinfo->string_metatable->string_tables[i]->language == usb_ep0_setup_buffer.index) {
											p = usb_devinfo->string_metatable->string_tables[i]->strings[(usb_ep0_setup_buffer.value & 0xFF) - 1];
											usb_ep0_data[0].ptr = scratch_buffer;
											usb_ep0_data[0].length = 2;
											usb_ep0_data[1].ptr = p;
											usb_ep0_data[1].length = 0;
											while (*p) {
												++p;
												usb_ep0_data[1].length += 2;
											}
											scratch_buffer[0] = 2 + usb_ep0_data[1].length;
											scratch_buffer[1] = USB_DESCRIPTOR_STRING;
											usb_ep0_data_length = 2;
											ok = true;
											break;
										}
									}
								}
								break;
#endif
						}
						break;

					case USB_SETUP_PACKET_STDREQ_GET_CONFIGURATION:
						if (usb_current_configuration == 0xFF) {
							scratch_buffer[0] = 0;
							usb_ep0_data[0].ptr = scratch_buffer;
						} else {
							usb_ep0_data[0].ptr = &usb_devinfo->configurations[usb_current_configuration]->configuration_descriptor->id;
						}
						usb_ep0_data[0].length = 1;
						usb_ep0_data_length = 1;
						ok = true;
						break;

					case USB_SETUP_PACKET_STDREQ_SET_CONFIGURATION:
						if (usb_ep0_setup_buffer.value) {
							uint8_t i;
							for (i = 0; i != usb_devinfo->device_descriptor->num_configurations; ++i) {
								if (usb_devinfo->configurations[i]->configuration_descriptor->id == usb_ep0_setup_buffer.value) {
									break;
								}
							}
							if (i != usb_devinfo->device_descriptor->num_configurations) {
								void (*fptr)(void);
								if (usb_current_configuration != 0xFF) {
									fptr = usb_devinfo->configurations[usb_current_configuration]->on_exit;
									if (fptr) {
										fptr();
									}
								}
								usb_current_configuration = i;
								fptr = usb_devinfo->configurations[i]->on_enter;
								if (fptr) {
									fptr();
								}
								ok = true;
							}
						} else {
							if (usb_current_configuration != 0xFF) {
								void (*fptr)(void) = usb_devinfo->configurations[usb_current_configuration]->on_exit;
								if (fptr) {
									fptr();
								}
							}
							usb_current_configuration = 0xFF;
						}
						usb_halted_in_endpoints = 0;
						usb_halted_out_endpoints = 0;
						ok = true;
						break;
				}
				break;

			case USB_SETUP_PACKET_RECIPIENT_INTERFACE:
				/* Addressed to an interface. */
				/* Check if the interface exists. */
				if (usb_current_configuration != 0xFF && usb_ep0_setup_buffer.index < usb_devinfo->configurations[usb_current_configuration]->num_interfaces) {
					switch (usb_ep0_setup_buffer.request) {
						case USB_SETUP_PACKET_STDREQ_GET_INTERFACE:
							scratch_buffer[0] = 0;
							usb_ep0_data[0].ptr = scratch_buffer;
							usb_ep0_data[0].length = 1;
							usb_ep0_data_length = 1;
							ok = true;
							break;

						case USB_SETUP_PACKET_STDREQ_GET_STATUS:
							scratch_buffer[0] = 0x00;
							scratch_buffer[1] = 0x00;
							usb_ep0_data[0].ptr = scratch_buffer;
							usb_ep0_data[0].length = 2;
							usb_ep0_data_length = 1;
							ok = true;
							break;
					}
				}
				break;

			case USB_SETUP_PACKET_RECIPIENT_ENDPOINT:
				/* Addressed to an endpoint. */
				{
					/* Check if the endpoint exists. */
					BOOL is_in = !!(usb_ep0_setup_buffer.index & 0x80);
					uint8_t ep = usb_ep0_setup_buffer.index & 0x7F;
					uint16_t ep_mask = ((uint16_t) 1) << ep;
					uint16_t valid_mask;
					if (usb_current_configuration != 0xFF) {
						if (is_in) {
							valid_mask = usb_devinfo->configurations[usb_current_configuration]->valid_in_endpoints;
						} else {
							valid_mask = usb_devinfo->configurations[usb_current_configuration]->valid_out_endpoints;
						}
					} else {
						valid_mask = 0;
					}
					valid_mask |= 1;
					if (valid_mask & ep_mask) {
						switch (usb_ep0_setup_buffer.request) {
							case USB_SETUP_PACKET_STDREQ_CLEAR_FEATURE:
								switch (usb_ep0_setup_buffer.value) {
									case USB_SETUP_PACKET_STDFEAT_ENDPOINT_HALT:
										if (!ep) {
											ok = true;
										} else if (is_in) {
											if (usb_ep_callbacks[ep].in.clear_halt()) {
												usb_halted_in_endpoints &= ~ep_mask;
												ok = true;
											}
										} else {
											if (usb_ep_callbacks[ep].out.clear_halt()) {
												usb_halted_out_endpoints &= ~ep_mask;
												ok = true;
											}
										}
										break;
								}
								break;

							case USB_SETUP_PACKET_STDREQ_GET_STATUS:
								{
									scratch_buffer[0] = 0;
									scratch_buffer[1] = 0;
									if (is_in) {
										if (usb_halted_in_endpoints & ep_mask) {
											scratch_buffer[0] = 0x01;
										}
									} else {
										if (usb_halted_out_endpoints & ep_mask) {
											scratch_buffer[0] = 0x01;
										}
									}
									usb_ep0_data[0].ptr = scratch_buffer;
									usb_ep0_data[0].length = 2;
									usb_ep0_data_length = 1;
									ok = true;
								}
								break;

							case USB_SETUP_PACKET_STDREQ_SET_FEATURE:
								switch (usb_ep0_setup_buffer.value) {
									case USB_SETUP_PACKET_STDFEAT_ENDPOINT_HALT:
										if (ep) {
											if (is_in) {
												if (!(usb_halted_in_endpoints & ep_mask)) {
													usb_halted_in_endpoints |= ep_mask;
													usb_ep_callbacks[ep].in.commanded_stall();
												}
											} else {
												if (!(usb_halted_out_endpoints & ep_mask)) {
													usb_halted_out_endpoints |= ep_mask;
													usb_ep_callbacks[ep].out.commanded_stall();
												}
											}
											ok = true;
										}
										break;
								}
						}
					}
				}
				break;
		}
	}

	/* Retract ownership of the IN endpoint from the SIE. */
	usb_bdpairs[0].in.BDSTAT = 0;

	/* Re-enable packet processing. */
	UCONbits.PKTDIS = 0;

	/* Clamp length of data we will provide to length of data host will accept. */
	if (usb_ep0_data_length > usb_ep0_setup_buffer.length) {
		usb_ep0_data_length = usb_ep0_setup_buffer.length;
	}

	/* If the request is acceptable so far, get ready for the subsequent stages. */
	if (ok) {
		/* The request was acceptable. */
		if (!usb_ep0_setup_buffer.length) {
			/* The request has no data stage. */
			/* The out buffer needs to accept a subsequent SETUP transaction. */
			usb_bdpairs[0].out.BDCNT = 8;
			usb_bdpairs[0].out.BDADR = usb_ep0_out_buffer;
			usb_bdpairs[0].out.BDSTAT = BDSTAT_UOWN | BDSTAT_BSTALL;
			/* The in buffer needs to run the status stage. */
			/* The status stage is a zero-length IN DATA1. */
			usb_bdpairs[0].in.BDCNT = 0;
			usb_bdpairs[0].in.BDADR = usb_ep0_in_buffer;
			usb_bdpairs[0].in.BDSTAT = BDSTAT_UOWN | BDSTAT_DTS | BDSTAT_DTSEN;
		} else if (usb_ep0_setup_buffer.request_type.bits.direction) {
			/* The request has a data stage from device to host. */
			/* The in buffer will contain data being transferred. Prime it. */
			populate_in_data();
			/* The out buffer needs to run the status stage as well as accept possible SETUP transactions. */
			/* The status stage is a zero-length OUT DATA1. */
			/* In case of SETUP, we need to accept up to eight bytes. */
			usb_bdpairs[0].out.BDCNT = 8;
			usb_bdpairs[0].out.BDADR = usb_ep0_out_buffer;
			usb_bdpairs[0].out.BDSTAT = BDSTAT_UOWN | BDSTAT_DTS | BDSTAT_DTSEN;
		} else {
			/* The request has a data stage from host to device. */
			/* We never actually use this. */
			ok = false;
		}
	}

	/* If the request is not acceptable, stall everything and wait for the next SETUP transaction. */
	if (!ok) {
		usb_bdpairs[0].out.BDCNT = 8;
		usb_bdpairs[0].out.BDADR = usb_ep0_out_buffer;
		usb_bdpairs[0].out.BDSTAT = BDSTAT_UOWN | BDSTAT_BSTALL;
		usb_bdpairs[0].in.BDCNT = 0;
		usb_bdpairs[0].in.BDADR = usb_ep0_in_buffer;
		usb_bdpairs[0].in.BDSTAT = BDSTAT_UOWN | BDSTAT_BSTALL;
	}
}

/**
 * \brief Handles an OUT or SETUP transaction finishing on endpoint zero.
 */
static void on_out(void) {
	if (usb_bdpairs[0].out.BDSTATbits.sie.PID == 0xD) {
		/* This is a SETUP transaction. */
		on_setup();
	} else if (usb_ep0_setup_buffer.request_type.bits.direction) {
		/* The data stage of the transfer is device-to-host (in), so this interrupt indicates completion of the status stage.
		 * We could see a new SETUP, or we could see a retransmitted status DATA1. Arm the out buffer. */
		usb_bdpairs[0].out.BDCNT = 8;
		usb_bdpairs[0].out.BDADR = usb_ep0_out_buffer;
		usb_bdpairs[0].out.BDSTAT = BDSTAT_UOWN | BDSTAT_DTSEN;
	} else {
		/* This case shouldn't happen: we don't implement host-to-device (out) data stages and the no-data case has an IN DATA1, not an OUT DATA1. */
		usb_bdpairs[0].out.BDCNT = 8;
		usb_bdpairs[0].out.BDADR = usb_ep0_out_buffer;
		usb_bdpairs[0].out.BDSTAT = BDSTAT_UOWN | BDSTAT_BSTALL;
	}
}

/**
 * \brief Handles an IN transaction finishing on endpoint zero.
 */
static void on_in(void) {
	if (usb_ep0_setup_buffer.request_type.bits.direction) {
		/* The data stage of the transfer is device-to-host (in), so this interrupt indicates completion of a segment of data stage. */
		if (usb_ep0_setup_buffer.length) {
			/* The host is still expecting more data. Populate the buffer. */
			populate_in_data();
		} else {
			/* The host doesn't expect any more data. The out buffer is already armed for the status stage; leave the in buffer disarmed. */
		}
	} else {
		/* The data stage of the transfer is host-to-device (out), so this interrupt indicates completion of the status stage.
		 * If the request was a standard SET ADDRESS, we must install the new address now.
		 * This is in contrast to every single other control request ever used, where the action is taken before the status stage. */
		if (usb_ep0_setup_buffer.request_type.bits.recipient == USB_SETUP_PACKET_RECIPIENT_DEVICE && usb_ep0_setup_buffer.request_type.bits.type == USB_SETUP_PACKET_REQUEST_STANDARD && usb_ep0_setup_buffer.request == USB_SETUP_PACKET_STDREQ_SET_ADDRESS) {
			UADDR = usb_ep0_setup_buffer.value;
		}
		/* Leave the in buffer disarmed. */
	}
}

void usb_ep0_init(void) {
	/* Register endpoint callbacks. */
	usb_ep_callbacks[0].out.transaction = &on_out;
	usb_ep_callbacks[0].in.transaction = &on_in;

	/* Clear local state. */
	usb_ep0_data_length = 0;
	usb_ep0_next_iovec = 0;
	usb_current_configuration = 0xFF;

	/* Disarm the in buffer. */
	usb_bdpairs[0].in.BDSTAT = 0;

	/* Configure the out buffer to accept a SETUP transaction. */
	usb_bdpairs[0].out.BDCNT = 8;
	usb_bdpairs[0].out.BDADR = usb_ep0_out_buffer;
	usb_bdpairs[0].out.BDSTAT = BDSTAT_UOWN | BDSTAT_BSTALL;

	/* Configure the endpoint register. */
	UEP0bits.EPHSHK = 1;
	UEP0bits.EPOUTEN = 1;
	UEP0bits.EPINEN = 1;
	UEP0bits.EPCONDIS = 0;
}

