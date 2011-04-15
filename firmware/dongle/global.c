#include "global.h"
#include "pins.h"
#include "usb.h"
#include "xbee_rxpacket.h"
#include "xbee_txpacket.h"
#include <delay.h>
#include <pic18fregs.h>

volatile BOOL should_run = false;

uint16_t xbee_versions[2] = { 0, 0 };

