#ifndef XBEE_DAEMON_FRONTEND_SCHEDULER_H
#define XBEE_DAEMON_FRONTEND_SCHEDULER_H

#include "util/noncopyable.h"
#include "xbee/daemon/frontend/request.h"
#include <ctime>
#include <queue>
#include <vector>
#include <glibmm.h>
#include <stdint.h>

class daemon;

//
// A packet scheduler that manages when packets should be sent over the radio.
//
class scheduler : public noncopyable {
	public:
		//
		// Constructs a new scheduler.
		//
		scheduler(class daemon &);

		//
		// Queues a new unicast packet for transmission.
		//
		void queue(request::ptr req);

	private:
		class daemon &daemon;
		std::queue<request::ptr> pending;
		struct sent_request {
			request::ptr data;
			sigc::connection timeout_connection;
		} sent[256];
		unsigned int sent_count;
		enum NEXT_TYPE {
			NEXT_QUEUED,
			NEXT_BULK
		} next_type;
		unsigned int last_feedback_index;
		uint16_t last_feedback_address;
		sigc::connection feedback_timeout_connection;
		timespec last_feedback_timestamp;

		void push();
		bool on_request_timeout(uint8_t);
		void on_feedback_timeout();
		void on_receive(const std::vector<uint8_t> &);
};

#endif

