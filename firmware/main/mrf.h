#ifndef MRF_H
#define MRF_H

#include <stdbool.h>
#include <stdint.h>
#include "dma.h"
#include "io.h"

/**
 * \brief The possible short register addresses.
 */
typedef enum {
	MRF_REG_SHORT_RXMCR,
	MRF_REG_SHORT_PANIDL,
	MRF_REG_SHORT_PANIDH,
	MRF_REG_SHORT_SADRL,
	MRF_REG_SHORT_SADRH,
	MRF_REG_SHORT_EADR0,
	MRF_REG_SHORT_EADR1,
	MRF_REG_SHORT_EADR2,
	MRF_REG_SHORT_EADR3,
	MRF_REG_SHORT_EADR4,
	MRF_REG_SHORT_EADR5,
	MRF_REG_SHORT_EADR6,
	MRF_REG_SHORT_EADR7,
	MRF_REG_SHORT_RXFLUSH,
	MRF_REG_SHORT_ORDER = 0x10,
	MRF_REG_SHORT_TXMCR,
	MRF_REG_SHORT_ACKTMOUT,
	MRF_REG_SHORT_ESLOTG1,
	MRF_REG_SHORT_SYMTICKL,
	MRF_REG_SHORT_SYMTICKH,
	MRF_REG_SHORT_PACON0,
	MRF_REG_SHORT_PACON1,
	MRF_REG_SHORT_PACON2,
	MRF_REG_SHORT_TXBCON0 = 0x1A,
	MRF_REG_SHORT_TXNCON,
	MRF_REG_SHORT_TXG1CON,
	MRF_REG_SHORT_TXG2CON,
	MRF_REG_SHORT_ESLOTG23,
	MRF_REG_SHORT_ESLOTG45,
	MRF_REG_SHORT_ESLOTG67,
	MRF_REG_SHORT_TXPEND,
	MRF_REG_SHORT_WAKECON,
	MRF_REG_SHORT_FRMOFFSET,
	MRF_REG_SHORT_TXSTAT,
	MRF_REG_SHORT_TXBCON1,
	MRF_REG_SHORT_GATECLK,
	MRF_REG_SHORT_TXTIME,
	MRF_REG_SHORT_HSYMTMRL,
	MRF_REG_SHORT_HSYMTMRH,
	MRF_REG_SHORT_SOFTRST,
	MRF_REG_SHORT_SECCON0 = 0x2C,
	MRF_REG_SHORT_SECCON1,
	MRF_REG_SHORT_TXSTBL,
	MRF_REG_SHORT_RXSR = 0x30,
	MRF_REG_SHORT_INTSTAT,
	MRF_REG_SHORT_INTCON,
	MRF_REG_SHORT_GPIO,
	MRF_REG_SHORT_TRISGPIO,
	MRF_REG_SHORT_SLPACK,
	MRF_REG_SHORT_RFCTL,
	MRF_REG_SHORT_SECCR2,
	MRF_REG_SHORT_BBREG0,
	MRF_REG_SHORT_BBREG1,
	MRF_REG_SHORT_BBREG2,
	MRF_REG_SHORT_BBREG3,
	MRF_REG_SHORT_BBREG4,
	MRF_REG_SHORT_BBREG6 = 0x3E,
	MRF_REG_SHORT_CCAEDTH,
} mrf_reg_short_t;

/**
 * \brief The possible long register addresses.
 */
typedef enum {
	MRF_REG_LONG_TXNFIFO = 0x000,
	MRF_REG_LONG_TXBFIFO = 0x080,
	MRF_REG_LONG_TXGTS1FIFO = 0x100,
	MRF_REG_LONG_TXGTS2FIFO = 0x180,
	MRF_REG_LONG_RFCON0 = 0x200,
	MRF_REG_LONG_RFCON1,
	MRF_REG_LONG_RFCON2,
	MRF_REG_LONG_RFCON3,
	MRF_REG_LONG_RFCON5 = 0x205,
	MRF_REG_LONG_RFCON6,
	MRF_REG_LONG_RFCON7,
	MRF_REG_LONG_RFCON8,
	MRF_REG_LONG_SLPCAL0,
	MRF_REG_LONG_SLPCAL1,
	MRF_REG_LONG_SLPCAL2,
	MRF_REG_LONG_RFSTATE = 0x20F,
	MRF_REG_LONG_RSSI,
	MRF_REG_LONG_SLPCON0,
	MRF_REG_LONG_SLPCON1 = 0x220,
	MRF_REG_LONG_WAKETIMEL = 0x222,
	MRF_REG_LONG_WAKETIMEH,
	MRF_REG_LONG_REMCNTL,
	MRF_REG_LONG_REMCNTH,
	MRF_REG_LONG_MAINCNT0,
	MRF_REG_LONG_MAINCNT1,
	MRF_REG_LONG_MAINCNT2,
	MRF_REG_LONG_MAINCNT3,
	MRF_REG_LONG_TESTMODE = 0x22F,
	MRF_REG_LONG_ASSOEADR0,
	MRF_REG_LONG_ASSOEADR1,
	MRF_REG_LONG_ASSOEADR2,
	MRF_REG_LONG_ASSOEADR3,
	MRF_REG_LONG_ASSOEADR4,
	MRF_REG_LONG_ASSOEADR5,
	MRF_REG_LONG_ASSOEADR6,
	MRF_REG_LONG_ASSOEADR7,
	MRF_REG_LONG_ASSOSADR0,
	MRF_REG_LONG_ASSOSADR1,
	MRF_REG_LONG_UPNONCE0 = 0x240,
	MRF_REG_LONG_UPNONCE1,
	MRF_REG_LONG_UPNONCE2,
	MRF_REG_LONG_UPNONCE3,
	MRF_REG_LONG_UPNONCE4,
	MRF_REG_LONG_UPNONCE5,
	MRF_REG_LONG_UPNONCE6,
	MRF_REG_LONG_UPNONCE7,
	MRF_REG_LONG_UPNONCE8,
	MRF_REG_LONG_UPNONCE9,
	MRF_REG_LONG_UPNONCE10,
	MRF_REG_LONG_UPNONCE11,
	MRF_REG_LONG_UPNONCE12,
	MRF_REG_LONG_KEYFIFO = 0x280,
	MRF_REG_LONG_RXFIFO = 0x300,
} mrf_reg_long_t;

/**
 * \brief A buffer into which received data is placed.
 *
 * The structure of data found here is as described in the MRF24J40 datasheet.
 * The application must only read from this buffer immediately following a \c true return from a call to \ref mrf_rx_poll.
 */
extern uint8_t mrf_rx_buffer[128];

/**
 * \brief A buffer into which the application places data for transmission.
 *
 * The structure of data placed here must match the format described in the MRF24J40 datasheet.
 * The application must only write to this buffer if \c mrf_tx_buffer_free returned \c true.
 */
extern uint8_t mrf_tx_buffer[128];

/**
 * \brief Initializes the radio.
 *
 * \param channel the channel to operate on, from 11 to 26 inclusive
 *
 * \param symbol_rate \c true to run at 625 kb/s, or \c false to run at 250 kb/s
 *
 * \param short_address the 16-bit address to use
 *
 * \param pan_id the PAN ID to communicate on, from 0 to 0xFFFE
 *
 * \param mac_address the node’s MAC address
 */
void mrf_init(uint8_t channel, bool symbol_rate, uint16_t pan_id, uint16_t short_address, uint64_t mac_address);

/**
 * \brief Checks whether a new packet has been received.
 *
 * This function returns \c true exactly once for each packet received.
 * Once this function returns \c true, the application must consume the packet before calling this function again.
 *
 * \return \c true if a packet is ready in the receive buffer for the application to process, or \c false if not
 */
bool mrf_rx_poll(void);

/**
 * \brief Checks whether the transmit buffer is available for application use.
 *
 * If \ref mrf_tx_path_free returns \c true, then this function can also be assumed to return true.
 *
 * \return \c true if it is safe for the application to modify \ref mrf_tx_buffer, or \c false if not
 */
bool mrf_tx_buffer_free(void);

/**
 * \brief Checks whether the transmit path is available for starting a packet transmission.
 *
 * \return \c true if it is safe for the appication to call \ref mrf_tx_start, or \c false if not
 */
bool mrf_tx_path_free(void);

/**
 * \brief Starts a packet transmission.
 *
 * \pre \ref mrf_tx_path_free must have returned \c true
 *
 * \param reliable \c true if the transmission must be acknowledged by the peer, or \c false if not
 */
void mrf_tx_start(bool reliable);

#endif

