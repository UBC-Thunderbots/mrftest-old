#ifndef MRF_H
#define MRF_H

#include <stdbool.h>
#include <stdint.h>

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

/**
 * \brief Checks whether the last transmitted packet was successfully delivered or not.
 *
 * This function is only applicable to unreliable packets, as reliable packets are retried forever until they succeed.
 *
 * \return \c true if the packet was delivered and acknowledged, or \c false if not
 */
bool mrf_tx_successful(void);

#endif

