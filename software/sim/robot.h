#ifndef SIM_ROBOT_H
#define SIM_ROBOT_H

#include "sim/player.h"
#include "sim/engines/engine.h"
#include "util/byref.h"
#include <glibmm.h>

class simulator;

/**
 * A simulation of an actual physical robot. This is different from the player
 * class in that a player is a live, driving robot provided by an engine,
 * whereas a robot may actually be powered off, crashed, off the field, unclaimed,
 * or otherwise not driving normally.
 *
 * While the set of players in existence may change as robots are powered up and
 * down and moved onto and off of the field, the set of robots is fixed when the
 * simulator is launched by the contents of the configuration file.
 */
class robot : public byref {
	public:
		/**
		 * A pointer to a robot.
		 */
		typedef Glib::RefPtr<robot> ptr;

		/**
		 * The XBee address of the robot.
		 */
		const uint64_t address;

		/**
		 * Emitted when a property about the robot changes.
		 */
		sigc::signal<void> signal_changed;

		/**
		 * Creates a new robot. The robot is initially located off the field and
		 * its power switch is initially off.
		 * \param address the robot's address
		 * \param engine the simulator engine that will back the robot when it
		 * is running
		 */
		static ptr create(uint64_t address, simulator_engine::ptr engine);

		/**
		 * \return true if the robot is powered, or false if not
		 */
		bool powered() const {
			return powered_;
		}

		/**
		 * Powers the robot up or down.
		 * \param pwr true to power the robot up, or false to power it down
		 */
		void powered(bool pwr);

		/**
		 * \return the voltage of this robot's battery
		 */
		double battery() const {
			return battery_;
		}

		/**
		 * Sets the voltage of the robot's battery.
		 * \param bat the battery voltage
		 */
		void battery(double bat);

		/**
		 * \return true if the robot is bootloading, or false if not
		 */
		bool bootloading() const {
			return bootloading_;
		}

		/**
		 * Puts the robot into or out of bootload mode.
		 * \param bl true to enter bootload mode, or false to exit it
		 */
		void bootloading(bool bl);

		/**
		 * \return The robot's currently-assigned 16-bit address
		 */
		uint16_t address16() const {
			return address16_;
		}

		/**
		 * Sets the robot's 16-bit address.
		 * \param addr the new address
		 */
		void address16(uint16_t addr);

		/**
		 * \return The robot's currently-assigned run data offset
		 */
		uint8_t run_data_offset() const {
			return run_data_offset_;
		}

		/**
		 * Sets the robot's run data offset.
		 * \param offset the new offset
		 */
		void run_data_offset(uint8_t offset);

		/**
		 * \return The engine player backing the robot, or a null pointer if the
		 * robot is not currently in a state where it is backed by an engine
		 * player
		 */
		player::ptr get_player() const {
			return player_;
		}

		/**
		 * Places this robot on the field and attaches it to the engine, if not
		 * already done.
		 */
		void add_player();

		/**
		 * Removes this robot from the field and detaches it from the engine, if
		 * it is on the field.
		 */
		void remove_player();

	private:
		const simulator_engine::ptr engine;
		bool powered_;
		double battery_;
		bool bootloading_;
		uint16_t address16_;
		uint8_t run_data_offset_;
		player::ptr player_;

		robot(uint64_t, simulator_engine::ptr);
};

#endif

