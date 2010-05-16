#ifndef UTIL_CLOCKSOURCE_MASTER_H
#define UTIL_CLOCKSOURCE_MASTER_H

#include "util/fd.h"
#include "util/noncopyable.h"
#include <list>

//
// A distributor mechanism that sends clock ticks out to clients so that work
// can be done in lockstep. This is not an actual clock source; it is the
// distributor that drives clocksource_slave instances.
//
class clocksource_master : public noncopyable {
	public:
		//
		// Initializes the distributor, including binding to the socket.
		//
		clocksource_master();

		//
		// Shuts down the distributor.
		//
		~clocksource_master();

		//
		// Distributes a clock tick. This method does not return until all
		// connected clients have acknowledged the tick. This acknowledgement
		// protocol keeps the clients in lockstep and prevents jitter.
		//
		void tick();

	private:
		file_descriptor listen_sock;
		std::list<int> socks;
};

#endif

