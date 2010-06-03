#ifndef SIM_SIMULATOR_H
#define SIM_SIMULATOR_H

#include "sim/ball.h"
#include "sim/player.h"
#include "sim/robot.h"
#include "sim/visdata.h"
#include "sim/engines/engine.h"
#include "util/clocksource.h"
#include "util/config.h"
#include "util/fd.h"
#include "util/noncopyable.h"
#include "xbee/daemon/frontend/backend.h"
#include <cstddef>
#include <queue>
#include <vector>
#include <unordered_map>
#include <stdint.h>

/**
 * This is the actual core of the simulator.
 */
class simulator : public backend, public sigc::trackable {
	public:
		/**
		 * The configuration file driving this simulator.
		 */
		const config &conf;

		/**
		 * Constructs a new simulator using the robots found in a configuration
		 * file.
		 * \param conf the configuration data to initialize the simulator with
		 * \param engine the engine to drive the simulator with
		 * \param clk the clock source to drive the simulator with
		 */
		simulator(const config &conf, simulator_engine::ptr engine, clocksource &clk);

		/**
		 * \return All the robots recognized by this simulator, keyed by XBee
		 * address
		 */
		const std::unordered_map<uint64_t, robot::ptr> &robots() const {
			return robots_;
		}

		/**
		 * \return The ball
		 */
		::ball::ptr ball() const {
			return engine->get_ball();
		}

		/**
		 * Searches for a robot by its 16-bit address.
		 * \param addr the address to search for
		 * \return The robot matching the address, or a null pointer if no robot
		 * has the address
		 */
		robot::ptr find_by16(uint16_t addr) const;

		/**
		 * \return The visualization data
		 */
		const visualizable &visualizer_data() const {
			return visdata;
		}

	private:
		const simulator_engine::ptr engine;
		std::unordered_map<uint64_t, robot::ptr> robots_;
		std::queue<std::vector<uint8_t> > responses;
		sigc::connection response_push_connection;
		uint16_t host_address16;
		file_descriptor sock;
		uint32_t frame_counters[2];
		simulator_visdata visdata;

		void send(const iovec *, std::size_t);
		void packet_handler(const std::vector<uint8_t> &data);
		void queue_response(const void *, std::size_t);
		bool push_response();
		void tick();
		bool tick_geometry();
};

#endif

