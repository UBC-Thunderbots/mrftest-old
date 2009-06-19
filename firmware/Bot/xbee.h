#ifndef XBEE_H
#define XBEE_H

enum xbee_rxflags {
	XBEE_RXFLAG_RUN    = 0,
	XBEE_RXFLAG_REBOOT = 1,
	XBEE_RXFLAG_REPORT = 2,
};

struct xbee_rxdata {
	int8_t vx;
	int8_t vy;
	int8_t vt;
	uint8_t dribble;
	uint8_t kick;
	uint8_t flags;
} __attribute__((packed));

struct xbee_txdata {
	uint8_t v_green[2];
	uint8_t v_motor[2];
	uint8_t firmware_version[2];
} __attribute__((packed));

void xbee_init(void);
void xbee_receive(void);
void xbee_send(void);

extern unsigned long xbee_rxtimestamp;
extern struct xbee_rxdata xbee_rxdata;
extern struct xbee_txdata xbee_txdata;

#endif

