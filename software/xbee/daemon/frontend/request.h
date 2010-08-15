#ifndef XBEE_DAEMON_FRONTEND_REQUEST_H
#define XBEE_DAEMON_FRONTEND_REQUEST_H

#include "util/byref.h"
#include <cassert>
#include <vector>
#include <glibmm.h>
#include <stdint.h>

/**
 * A packet to be sent to the XBee which may or may not provoke a response and
 * which, if it provokes a response, may or may not have callback functions to
 * be invoked to handle the response.
 */
class XBeeRequest : public ByRef {
	public:
		/**
		 * A pointer to a XBeeRequest.
		 */
		typedef RefPtr<XBeeRequest> Ptr;

		/**
		 * Constructs a new XBeeRequest. The data is copied.
		 *
		 * \param[in] data the data that makes up the request packet.
		 *
		 * \param[in] length the number of bytes in \p data.
		 *
		 * \param[in] has_response \c true if the modem will send a response to
		 * the request, or \c false if not.
		 */
		static Ptr create(const void *data, std::size_t length, bool has_response) {
			Ptr p(new XBeeRequest(data, length, has_response));
			return p;
		}

		/**
		 * Returns the data for the XBeeRequest.
		 *
		 * \return the data.
		 */
		const std::vector<uint8_t> &data() const {
			return data_;
		}

		/**
		 * Returns whether or not the XBeeRequest is expecting a response.
		 *
		 * \return \c true if a response will be sent, or \c false if not.
		 */
		bool has_response() const {
			return has_response_;
		}

		/**
		 * Returns a signal that will be fired when the response arrives.
		 *
		 * \return the signal.
		 */
		sigc::signal<void, const void *, std::size_t> &signal_complete() {
			assert(has_response_);
			return signal_complete_;
		}

	private:
		std::vector<uint8_t> data_;
		bool has_response_;
		sigc::signal<void, const void *, std::size_t> signal_complete_;

		XBeeRequest(const void *data, std::size_t length, bool has_response) : data_(static_cast<const uint8_t *>(data), static_cast<const uint8_t *>(data) + length), has_response_(has_response) {
			if (has_response) {
				assert(static_cast<const uint8_t *>(data)[1]);
			}
		}
};

#endif

