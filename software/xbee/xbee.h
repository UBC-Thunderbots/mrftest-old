#ifndef XBEE_XBEE_H
#define XBEE_XBEE_H

#include "util/fd.h"
#include "util/noncopyable.h"
#include <cstddef>
#include <sigc++/sigc++.h>
#include <glibmm.h>

//
// Allows access to the XBee radio modem, via the arbiter daemon.
//
// In order to arbitrate access to the modem, there is a conceptual "token"
// which is passed between modem clients. The token is managed by the arbiter. A
// client produces a stream of messages which are delivered to the arbiter. Each
// message can be either SEND or UNLOCK. A client is considered to want the
// token if it has any messages pending. Messages are only processed once the
// client has been granted the token; once this happens, messages from that
// client are processed up until an UNLOCK message is received, at which point
// the client is relieved of the token.
//
// When packets arrive from the modem, the arbiter forwards them to whichever
// client currently holds the token. This may not always be the client who
// expects those packets.
//
// To implement a typical request-response protocol with timeout, a client might
// do the following:
//
// 1. Call send() to enqueue a packet with the arbiter. The packet will be sent
//    over the modem once the client is granted the token.
// 2. Start a timer to handle the timeout situation.
// 3. Use signal_received() to register its interest in incoming packets.
// 4. When signal_received() fires with the appropriate response packet, or when
//    the timer expires, the client calls unlock() to release the token and
//    allow other clients their share of airtime. If it wishes to send more
//    data, it may return to step 1 and call send() again immediately after
//    calling unlock(). It is likely that if other clients wish to use the modem
//    as well, those other clients will get the token before that next packet is
//    sent.
//
class xbee : public noncopyable {
	public:
		//
		// Connects to the arbiter daemon, launching it if necessary.
		//
		xbee();

		//
		// Enqueues a packet to be sent to the modem.
		//
		void send(const void *, std::size_t);

		//
		// Enqueues a release of the token, following any queued sends.
		//
		void unlock();

		//
		// Fired when a packet is received while this client holds the token.
		//
		sigc::signal<void, const void *, std::size_t> &signal_received() {
			return sig_received;
		}

	private:
		const file_descriptor sock;
		sigc::signal<void, const void *, std::size_t> sig_received;

		bool on_readable(Glib::IOCondition);
};

#endif

