#ifndef SIM_SIMULATOR_H
#define SIM_SIMULATOR_H

#include "simulator/ball.h"
#include "simulator/player.h"
#include "simulator/robot.h"
#include "simulator/engines/engine.h"
#include "util/clocksource.h"
#include "util/config.h"
#include "util/fd.h"
#include "util/noncopyable.h"
#include "xbee/daemon/frontend/backend.h"
#include <cstddef>
#include <queue>
#include <stdint.h>
#include <unordered_map>
#include <vector>

/**
 * This is the actual core of the simulator.
 */
class Simulator : public BackEnd, public sigc::trackable {
	public:
		/**
		 * The configuration file driving this simulator.
		 */
		const Config &conf;

		/**
		 * Constructs a new Simulator using the robots found in a configuration file.
		 *
		 * \param[in] conf the configuration data to initialize the simulator with.
		 *
		 * \param[in] engine the engine to drive the simulator with.
		 *
		 * \param[in] clk the clock source to drive the simulator with.
		 */
		Simulator(const Config &conf, SimulatorEngine::Ptr engine, ClockSource &clk);

		/**
		 * Returns all the robots recognized by this simulator.
		 *
		 * \return all the robots recognized by this simulator, keyed by XBee address.
		 */
		const std::unordered_map<uint64_t, SimulatorRobot::Ptr> &robots() const {
			return robots_;
		}

		/**
		 * Returns the ball.
		 *
		 * \return the ball.
		 */
		SimulatorBall::Ptr ball() const {
			return engine->get_ball();
		}

		/**
		 * Searches for a robot by its 16-bit address.
		 *
		 * \param[in] addr the address to search for.
		 *
		 * \return the robot matching the address, or a null pointer if no robot has the address.
		 */
		SimulatorRobot::Ptr find_by16(uint16_t addr) const;

	private:
		const SimulatorEngine::Ptr engine;
		std::unordered_map<uint64_t, SimulatorRobot::Ptr> robots_;
		std::queue<std::vector<uint8_t> > responses;
		sigc::connection response_push_connection;
		uint16_t host_address16;
		const FileDescriptor::Ptr sock;
		uint32_t frame_counters[2];

		void send(const iovec *, std::size_t);
		void packet_handler(const std::vector<uint8_t> &data);
		void queue_response(const void *, std::size_t);
		bool push_response();
		void tick();
		bool tick_geometry();
};

#endif

