#ifndef MRF_PACKET_LOGGER_H
#define MRF_PACKET_LOGGER_H

/**
 * \file
 *
 * \brief Provides a way to log all USB packets sent to and received from the
 * dongle.
 */
#include <cstddef>

/**
 * \brief A consumer of packet log data.
 */
class MRFPacketLogger {
	public:
		virtual ~MRFPacketLogger();
		virtual void log_mrf_drive(const void *data, std::size_t length) = 0;
		virtual void log_mrf_message_out(unsigned int index, bool reliable, unsigned int id, const void *data, std::size_t length) = 0;
		virtual void log_mrf_message_in(unsigned int index, const void *data, std::size_t length, unsigned int lqi, unsigned int rssi) = 0;
		virtual void log_mrf_mdr(unsigned int id, unsigned int code) = 0;
};

#endif
