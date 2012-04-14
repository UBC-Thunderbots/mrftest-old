#ifndef SIM_SIMULATOR_H
#define SIM_SIMULATOR_H

#include "simulator/player.h"
#include "simulator/team.h"
#include "simulator/engines/engine.h"
#include "util/fd.h"
#include "util/noncopyable.h"
#include "util/time.h"
#include <cstddef>
#include <memory>
#include <vector>

namespace Simulator {
	/**
	 * \brief The core of the simulator.
	 *
	 * Handles accepting incoming connections from the AI processes, scheduling time ticks, and binding together the ball and the teams.
	 */
	class Simulator : public NonCopyable {
		public:
			/**
			 * \brief The simulation engine driving the simulation.
			 */
			SimulatorEngine &engine;

			/**
			 * \brief Constructs a new Simulator.
			 *
			 * \param[in] engine the engine to drive the simulator with.
			 */
			Simulator(SimulatorEngine &engine);

			/**
			 * \brief Returns the speed mode of the simulator.
			 *
			 * \return the current speed mode.
			 */
			::Simulator::Proto::SpeedMode speed_mode() const;

			/**
			 * \brief Sets the speed mode of the simulator.
			 *
			 * \param[in] mode the speed mode to run in.
			 */
			void speed_mode(::Simulator::Proto::SpeedMode mode);

			/**
			 * \brief Stores the current state of the ball into a state block.
			 *
			 * \param[out] state the state block to fill.
			 *
			 * \param[in] invert \c true to invert the coordinates, or \c false to use non-inverted coordinates.
			 */
			void encode_ball_state(Proto::S2ABallInfo &state, bool invert);

			/**
			 * \brief Prepares to load a simulation state from a file at the next tick.
			 *
			 * \param[in] fd the file to load from.
			 */
			void queue_load_state(std::shared_ptr<FileDescriptor> fd);

			/**
			 * \brief Saves the current state of the complete simulation into a file.
			 *
			 * \param[in] fd the file to store into.
			 */
			void save_state(std::shared_ptr<FileDescriptor> fd) const;

		private:
			/**
			 * \brief The listening socket that accepts connections from AI processes.
			 */
			FileDescriptor listen_socket;

			/**
			 * \brief The first team.
			 */
			Team team1;

			/**
			 * \brief The second team.
			 */
			Team team2;

			/**
			 * \brief The time of the next tick in game monotonic time.
			 */
			timespec next_tick_game_monotonic_time;

			/**
			 * \brief The time of the next tick in physical monotonic time.
			 */
			timespec next_tick_phys_monotonic_time;

			/**
			 * \brief The time of the last status line update in physical monotonic time.
			 */
			timespec last_fps_report_time;

			/**
			 * \brief The simulator's current speed mode.
			 */
			::Simulator::Proto::SpeedMode speed_mode_;

			/**
			 * \brief Whether a call to tick() has been scheduled on a timer but not yet executed.
			 */
			bool tick_scheduled;

			/**
			 * \brief The number of ticks since the last update of the status line.
			 */
			unsigned int frame_count;

			/**
			 * \brief The current position in the spinner character array.
			 */
			unsigned int spinner_index;

			/**
			 * \brief A file descriptor of a file containing state data that should be loaded at the next tick.
			 */
			std::shared_ptr<FileDescriptor> load_state_pending_fd;

			/**
			 * \brief Invoked when an inbound connection is waiting at the listening socket.
			 *
			 * \return \c true to continue accepting connections.
			 */
			bool on_ai_connecting(Glib::IOCondition);

			/**
			 * \brief Checks whether it's time to run another AI tick yet.
			 *
			 * If a team is not yet ready, does nothing and waits for the team to be ready.
			 * If all teams are ready but the simulator is in normal-speed mode and it's not time for another tick,
			 * attaches a timer to call tick().
			 * Otherwise, calls tick() immediately.
			 */
			void check_tick();

			/**
			 * \brief Runs an AI time tick, and also potentially updates the status line.
			 */
			void tick();
	};
}

#endif

