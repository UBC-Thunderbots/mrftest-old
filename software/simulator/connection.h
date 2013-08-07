#ifndef SIMULATOR_CONNECTION_H
#define SIMULATOR_CONNECTION_H

#include "simulator/sockproto/proto.h"
#include "util/fd.h"
#include "util/noncopyable.h"
#include <memory>
#include <glibmm/main.h>
#include <sigc++/trackable.h>
#include <sigc++/signal.h>

namespace Simulator {
	/**
	 * A connection to an AI.
	 */
	class Connection : public NonCopyable, public sigc::trackable {
		public:
			/**
			 * \brief A pointer to a Connection.
			 */
			typedef std::unique_ptr<Connection> Ptr;

			/**
			 * Constructs a new Connection.
			 *
			 * \param[in] sock the socket connected to the AI, which should have already been authenticated.
			 */
			Connection(FileDescriptor &&sock);

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
			sigc::signal<void, const Proto::A2SPacket &, std::shared_ptr<FileDescriptor>> &signal_packet() const;

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
			FileDescriptor sock;

			/**
			 * The signal emitted when the AI process closes its end of the socket.
			 */
			mutable sigc::signal<void> signal_closed_;

			/**
			 * The signal emitted when the AI process sends a packet to the simulator.
			 */
			mutable sigc::signal<void, const Proto::A2SPacket &, std::shared_ptr<FileDescriptor>> signal_packet_;

			/**
			 * Invoked when the socket has data waiting.
			 *
			 * \return \c true to keep reading more data.
			 */
			bool on_readable(Glib::IOCondition);
	};
}

#endif

