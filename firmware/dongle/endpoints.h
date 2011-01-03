#ifndef ENDPOINTS_H
#define ENDPOINTS_H

#define EP_DONGLE_STATUS 1
#define EP_LOCAL_ERROR_QUEUE 2
#define EP_STATISTICS 3
#define EP_STATE_TRANSPORT 4
#define EP_INTERRUPT 5
#define EP_BULK 6
#define EP_DEBUG 6

#define UEPBITS_(n) UEP ## n ## bits
#define UEPBITS(n) UEPBITS_(n)

#endif

