#ifndef SIMULATOR_CONNECTION_H
#define SIMULATOR_CONNECTION_H

#include "simulator/sockproto/proto.h"
#include "util/byref.h"
#include "util/fd.h"
#include <glibmm.h>

namespace Simulator {
	/**
	 * A connection to an AI.
	 */
	class Connection : public ByRef, public sigc::trackable {
		public:
			/**
			 * A pointer to a Connection.
			 */
			typedef RefPtr<Connection> Ptr;

			/**
			 * Constructs a new Connection.
			 *
			 * \param[in] sock the socket connected to the AI, which should have already been authenticated.
			 *
			 * \return the Connection.
			 */
			static Ptr create(FileDescriptor::Ptr sock);

			/**
			 * Returns the signal emitted when the socket is closed.
			 *
			 * \return the close signal.
			 */
			sigc::signal<void> &signal_closed() const;

			/**
			 * Returns the signal emitted when a packet is received.
			 *
			 * \return the packet receive signal.
			 */
			sigc::signal<void, const Proto::A2SPacket &> &signal_packet() const;

			/**
			 * Sends a packet to the AI.
			 *
			 * \param[in] packet the packet to send.
			 */
			void send(const Proto::S2APacket &packet);

		private:
			const FileDescriptor::Ptr sock;
			mutable sigc::signal<void> signal_closed_;
			mutable sigc::signal<void, const Proto::A2SPacket &> signal_packet_;

			Connection(FileDescriptor::Ptr sock);
			~Connection();
			bool on_readable(Glib::IOCondition);
			void close_connection();
	};
}

#endif

