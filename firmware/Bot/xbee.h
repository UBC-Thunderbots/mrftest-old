#ifndef XBEE_H
#define XBEE_H

struct xbee_rxdata {
	int8_t vx;
	int8_t vy;
	int8_t vt;
	uint8_t dribble;
	uint8_t kick;
	uint8_t emergency;
	int8_t vx_measured;
	int8_t vy_measured;
	uint8_t reboot;
	int8_t extra;
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

