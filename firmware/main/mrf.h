#ifndef MRF_H
#define MRF_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
	MRF_TX_OK,
	MRF_TX_NO_ACK,
	MRF_TX_CCA_FAIL,
	MRF_TX_CANCELLED,
} mrf_tx_result_t;

void mrf_init(uint8_t channel, bool symbol_rate, uint16_t pan_id, uint16_t short_address, uint64_t mac_address);
void mrf_shutdown(void);
uint16_t mrf_pan_id(void);
uint16_t mrf_short_address(void);
uint8_t mrf_alloc_seqnum(void);
mrf_tx_result_t mrf_transmit(const void *frame);
void mrf_transmit_cancel(void);
size_t mrf_receive(void *buffer);
void mrf_receive_cancel(void);

#endif
