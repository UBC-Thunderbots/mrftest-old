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
			 * \param[in] engine the simulator engine.
			 *
			 * \param[in] sim the main simulator.
			 *
			 * \param[in] other the other team in the simulator.
			 *
			 * \param[in] invert \c true to invert field coordinates and play types for this team, or \c false to not invert.
			 */
			Team(SimulatorEngine::Ptr engine, Simulator &sim, const Team *other, bool invert);

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

		private:
			struct PlayerInfo {
				SimulatorPlayer::Ptr player;
				unsigned int pattern;
			};

			const SimulatorEngine::Ptr engine;
			Simulator &sim;
			const Team * const other;
			const bool invert;
			mutable sigc::signal<void> signal_ready_;
			Connection::Ptr connection;
			std::vector<PlayerInfo> players;
			std::vector<unsigned int> to_add, to_remove;
			bool ready_;

			void close_connection();
			void on_packet(const Proto::A2SPacket &packet);
	};
}

#endif

