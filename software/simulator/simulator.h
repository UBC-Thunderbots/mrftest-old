#ifndef SIM_SIMULATOR_H
#define SIM_SIMULATOR_H

#include "simulator/player.h"
#include "simulator/team.h"
#include "simulator/engines/engine.h"
#include "util/fd.h"
#include "util/noncopyable.h"
#include "util/time.h"
#include <cstddef>
#include <vector>

namespace Simulator {
	/**
	 * The core of the simulator.
	 * Handles accepting incoming connections from the AI processes, scheduling time ticks, and binding together the ball and the teams.
	 */
	class Simulator : public NonCopyable {
		public:
			/**
			 * The simulation engine driving the simulation.
			 */
			const SimulatorEngine::Ptr engine;

			/**
			 * Constructs a new Simulator.
			 *
			 * \param[in] engine the engine to drive the simulator with.
			 */
			Simulator(SimulatorEngine::Ptr engine);

			/**
			 * Returns the speed mode of the simulator.
			 *
			 * \return \c true if the simulator is running in fast mode, or \c false if it is running in normal-speed mode.
			 */
			bool is_fast() const;

			/**
			 * Sets the speed mode of the simulator.
			 *
			 * \param[in] fast \c true to run in fast mode, or \c false to run in normal-speed mode.
			 */
			void set_speed_mode(bool fast);

			/**
			 * Returns the current play type.
			 *
			 * \return the current play type.
			 */
			AI::Common::PlayType::PlayType play_type() const;

			/**
			 * Sets the play type.
			 *
			 * \param[in] pt the play type to select.
			 */
			void set_play_type(AI::Common::PlayType::PlayType pt);

			/**
			 * Stores the current state of the ball into a state block.
			 *
			 * \param[out] state the state block to fill.
			 *
			 * \param[in] invert \c true to invert the coordinates, or \c false to use non-inverted coordinates.
			 */
			void encode_ball_state(Proto::S2ABallInfo &state, bool invert);

		private:
			/**
			 * The listening socket that accepts connections from AI processes.
			 */
			FileDescriptor::Ptr listen_socket;

			/**
			 * The first team.
			 */
			Team team1;

			/**
			 * The second team.
			 */
			Team team2;

			/**
			 * The time of the next tick in game monotonic time.
			 */
			timespec next_tick_game_monotonic_time;

			/**
			 * The time of the next tick in physical monotonic time.
			 */
			timespec next_tick_phys_monotonic_time;

			/**
			 * The time of the last status line update in physical monotonic time.
			 */
			timespec last_fps_report_time;

			/**
			 * Whether the AI is running in fast mode or normal-speed mode.
			 */
			bool fast;

			/**
			 * Whether a call to tick() has been scheduled on a timer but not yet executed.
			 */
			bool tick_scheduled;

			/**
			 * The current play type.
			 */
			AI::Common::PlayType::PlayType playtype;

			/**
			 * The number of ticks since the last update of the status line.
			 */
			unsigned int frame_count;

			/**
			 * Invoked when an inbound connection is waiting at the listening socket.
			 *
			 * \return \c true to continue accepting connections.
			 */
			bool on_ai_connecting(Glib::IOCondition);

			/**
			 * Invoked when a packet arrives on a socket that has been accepted but not yet authenticated.
			 *
			 * \param[in] fd the file descriptor of the pending socket.
			 *
			 * \return \c true if the socket still needs to wait for authentication data,
			 * or \c false if it can close or has been attached to a team.
			 */
			bool on_pending_ai_readable(Glib::IOCondition, FileDescriptor::Ptr fd);

			/**
			 * Checks whether it's time to run another AI tick yet.
			 * If a team is not yet ready, does nothing and waits for the team to be ready.
			 * If all teams are ready but the simulator is in normal-speed mode and it's not time for another tick,
			 * attaches a timer to call tick().
			 * Otherwise, calls tick() immediately.
			 */
			void check_tick();

			/**
			 * Runs an AI time tick, and also potentially updates the status line.
			 */
			void tick();
	};
}

#endif

