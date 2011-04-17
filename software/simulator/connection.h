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
			sigc::signal<void, const Proto::A2SPacket &, FileDescriptor::Ptr> &signal_packet() const;

			/**
			 * Sends a packet to the AI.
			 *
			 * \param[in] packet the packet to send.
			 */
			void send(const Proto::S2APacket &packet);

		private:
			/**
			 * The socket connected to the AI process.
			 */
			const FileDescriptor::Ptr sock;

			/**
			 * The signal emitted when the AI process closes its end of the socket.
			 */
			mutable sigc::signal<void> signal_closed_;

			/**
			 * The signal emitted when the AI process sends a packet to the simulator.
			 */
			mutable sigc::signal<void, const Proto::A2SPacket &, FileDescriptor::Ptr> signal_packet_;

			/**
			 * Constructs a new Connection.
			 *
			 * \param[in] sock the socket connected to the AI, which should have already been authenticated.
			 *
			 * \return the Connection.
			 */
			Connection(FileDescriptor::Ptr sock);

			/**
			 * Invoked when the socket has data waiting.
			 *
			 * \return \c true to keep reading more data.
			 */
			bool on_readable(Glib::IOCondition);
	};
}

#endif

