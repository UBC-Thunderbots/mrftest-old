#ifndef USB_INTERNAL_H
#define USB_INTERNAL_H

#include "stddef.h"
#include "usb.h"

extern const usb_device_info_t *usb_device_info;

void usb_copy_out_packet(void *target, size_t length);

void usb_fifo_init(size_t fifo_zero_size);
void usb_fifo_rx_flush(void);

void usb_ep0_init(void);
void usb_ep0_deinit(void);
void usb_ep0_handle_receive(unsigned int ep, uint32_t status_word);

#endif

