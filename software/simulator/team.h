#ifndef SIMULATOR_TEAM_H
#define SIMULATOR_TEAM_H

#include "simulator/connection.h"
#include "simulator/player.h"
#include "simulator/engines/engine.h"
#include "util/noncopyable.h"
#include <vector>

namespace Simulator {
	class Simulator;

	/**
	 * Encapsulates both a set of robots and a potential connection to an AI.
	 */
	class Team : public NonCopyable {
		public:
			/**
			 * Constructs a new Team.
			 *
			 * \param[in] sim the main simulator.
			 *
			 * \param[in] other the other team in the simulator.
			 *
			 * \param[in] invert \c true to invert field coordinates and play types for this team, or \c false to not invert.
			 */
			Team(Simulator &sim, const Team *other, bool invert);

			/**
			 * Destroys a Team.
			 */
			~Team();

			/**
			 * Checks whether the team has an AI connected.
			 *
			 * \return \c true if there is an AI connected, or \c false if not.
			 */
			bool has_connection() const;

			/**
			 * Attaches an AI connection to this team.
			 *
			 * \param[in] conn the connection to attach.
			 */
			void add_connection(Connection::Ptr conn);

			/**
			 * Checks whether the team is ready for the simulator to tick.
			 *
			 * \return \c true if the team is ready, or \c false if the AI is still running.
			 */
			bool ready() const;

			/**
			 * Returns the signal emitted when the team becomes ready (including if its connection becomes closed).
			 *
			 * \return the ready signal.
			 */
			sigc::signal<void> &signal_ready() const;

			/**
			 * Sends a packet to the AI with the world state.
			 *
			 * \param[in] ts the game monotonic timestamp of the tick.
			 */
			void send_tick(const timespec &ts);

			/**
			 * Sends a packet to the AI with the current speed mode.
			 */
			void send_speed_mode();

			/**
			 * Sends a packet to the AI with the current play type.
			 */
			void send_play_type();

			/**
			 * Loads a team state from a file.
			 *
			 * \param[in] fd the file to load from.
			 */
			void load_state(FileDescriptor::Ptr fd);

			/**
			 * Saves the current state of the team into a file.
			 *
			 * \param[in] fd the file to store into.
			 */
			void save_state(FileDescriptor::Ptr fd) const;

		private:
			/**
			 * The information about a player as stored in a team.
			 */
			struct PlayerInfo {
				/**
				 * The player object provided by the engine.
				 */
				Player::Ptr player;

				/**
				 * The player's lid pattern, used to identify it over the socket.
				 */
				unsigned int pattern;
			};

			/**
			 * The simulator to which this team belongs.
			 */
			Simulator &sim;

			/**
			 * The other team on the field.
			 */
			const Team *const other;

			/**
			 * Whether field coordinates and play types are inverted when reported to this team's AI processes.
			 */
			const bool invert;

			/**
			 * The signal emitted when this team becomes ready to run a tick.
			 */
			mutable sigc::signal<void> signal_ready_;

			/**
			 * The socket connection to this team's AI.
			 */
			Connection::Ptr connection;

			/**
			 * The players who belong to this team.
			 */
			std::vector<PlayerInfo> players;

			/**
			 * The lid patterns of robots who will be added to this team at the next tick.
			 */
			std::vector<unsigned int> to_add;

			/**
			 * The lid patterns of robots who will be removed from this team at the next tick.
			 */
			std::vector<unsigned int> to_remove;

			/**
			 * Whether this team is ready for a simulation tick to run.
			 */
			bool ready_;

			/**
			 * Closes the connection to this team's AI process and halts the team's robots.
			 */
			void close_connection();

			/**
			 * Invoked when this team's AI process sends a packet over the socket.
			 *
			 * \param[in] packet the packet.
			 *
			 * \param[in] ancillary_fd a file descriptor which may have been sent with the packet.
			 */
			void on_packet(const Proto::A2SPacket &packet, FileDescriptor::Ptr ancillary_fd);
	};
}

#endif

