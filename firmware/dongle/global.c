#include "global.h"
#include "pins.h"
#include "usb.h"
#include "xbee_rxpacket.h"
#include "xbee_txpacket.h"
#include <delay.h>
#include <pic18fregs.h>

volatile BOOL should_start_up = false;

volatile BOOL should_shut_down = false;

volatile uint8_t requested_channels[2] = { 0, 0 };

uint16_t xbee_versions[2];

