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
	 * This is the actual core of the simulator.
	 */
	class Simulator : public NonCopyable {
		public:
			/**
			 * Constructs a new Simulator using the robots found in a configuration file.
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
			const SimulatorEngine::Ptr engine;
			FileDescriptor::Ptr listen_socket;
			Team team1, team2, *teams[2];
			timespec next_tick_game_monotonic_time, next_tick_phys_monotonic_time, last_fps_report_time;
			bool fast, tick_scheduled;
			AI::Common::PlayType::PlayType playtype;
			unsigned int frame_count;

			bool on_ai_connecting(Glib::IOCondition);
			bool on_pending_ai_readable(Glib::IOCondition, FileDescriptor::Ptr fd);
			void check_tick();
			void tick();
	};
}

#endif

