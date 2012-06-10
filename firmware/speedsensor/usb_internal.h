#ifndef USB_INTERNAL_H
#define USB_INTERNAL_H

#include "stddef.h"
#include "usb.h"

extern const usb_device_info_t *usb_device_info;

void usb_ep0_init(void);
void usb_ep0_deinit(void);
void usb_ep0_handle_receive(uint32_t status_word);
void usb_ep0_handle_global_nak_effective(void);
void usb_copy_out_packet(void *target, size_t length);
void usb_set_global_nak(void);
void usb_clear_global_nak(void);

#endif

