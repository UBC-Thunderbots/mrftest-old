#ifndef SIM_ROBOT_H
#define SIM_ROBOT_H

#include "simulator/player.h"
#include "simulator/engines/engine.h"
#include "util/byref.h"
#include "util/config.h"
#include <cstdlib>
#include <glibmm.h>

class Simulator;

/**
 * A simulation of an actual physical robot.
 * This is different from the SimulatorPlayer class in that a SimulatorPlayer is a live, driving SimulatorRobot provided by an engine,
 * whereas a SimulatorRobot may actually be powered off, crashed, off the field, unclaimed, or otherwise not driving normally.
 *
 * While the set of players in existence may change as robots are powered up and down and moved onto and off of the field,
 * the set of robots is fixed when the simulator is launched by the contents of the configuration file.
 */
class SimulatorRobot : public ByRef {
	public:
		/**
		 * A pointer to a SimulatorRobot.
		 */
		typedef RefPtr<SimulatorRobot> Ptr;

		/**
		 * The XBee address of the robot.
		 */
		const uint64_t address;

		/**
		 * Emitted when a property about the robot changes.
		 */
		sigc::signal<void> signal_changed;

		/**
		 * Creates a new SimulatorRobot.
		 * The robot is initially located off the field and its power switch is initially off.
		 *
		 * \param[in] botinfo the information describing the robot.
		 *
		 * \param[in] engine the simulator engine that will back the robot when it is running.
		 */
		static Ptr create(const Config::RobotInfo &botinfo, SimulatorEngine::Ptr engine);

		/**
		 * Checks whether the robot is powered.
		 *
		 * \return \c true if the robot is powered, or \c false if not.
		 */
		bool powered() const {
			return powered_;
		}

		/**
		 * Powers the robot up or down.
		 *
		 * \param[in] pwr \c true to power the robot up, or \c false to power it down.
		 */
		void powered(bool pwr);

		/**
		 * Returns the voltage of this robot's battery.
		 *
		 * \return the voltage of this robot's battery.
		 */
		double battery() const {
			return battery_;
		}

		/**
		 * Sets the voltage of the robot's battery.
		 *
		 * \param[in] bat the battery voltage.
		 */
		void battery(double bat);

		/**
		 * Checks if the robot is bootloading.
		 *
		 * \return \c true if the robot is bootloading, or \c false if not.
		 */
		bool bootloading() const {
			return bootloading_;
		}

		/**
		 * Puts the robot into or out of bootload mode.
		 *
		 * \param[in] bl \c true to enter bootload mode, or \c false to exit it.
		 */
		void bootloading(bool bl);

		/**
		 * Returns the robot's currently-assigned 16-bit address.
		 *
		 * \return the robot's currently-assigned 16-bit address.
		 */
		uint16_t address16() const {
			return address16_;
		}

		/**
		 * Sets the robot's 16-bit address.
		 *
		 * \param[in] addr the new address.
		 */
		void address16(uint16_t addr);

		/**
		 * Returns the robot's currently-assigned run data offset.
		 *
		 * \return the robot's currently-assigned run data offset.
		 */
		uint8_t run_data_offset() const {
			return run_data_offset_;
		}

		/**
		 * Sets the robot's run data offset.
		 *
		 * \param[in] offset the new offset.
		 */
		void run_data_offset(uint8_t offset);

		/**
		 * Returns the backing SimulatorPlayer.
		 *
		 * \return the engine SimulatorPlayer backing the robot,
		 * or a null pointer if the robot is not currently in a state where it is backed by an engine SimulatorPlayer.
		 */
		SimulatorPlayer::Ptr get_player() const {
			return player_;
		}

		/**
		 * Places this robot on the field and attaches it to the engine, if not already done.
		 */
		void add_player();

		/**
		 * Removes this robot from the field and detaches it from the engine, if it is on the field.
		 */
		void remove_player();

		/**
		 * Checks whether this robot has an associated engine player.
		 *
		 * \return \c true if there is an engine player, or \c false if not.
		 */
		bool has_player() {
			return player_.is();
		}

		/**
		 * Returns the pattern index of the robot.
		 *
		 * \return the pattern index.
		 */
		unsigned int pattern() {
			return botinfo.pattern;
		}

		/**
		 * Returns the colour of the robot.
		 *
		 * \return \c true if the robot is yellow, or \c false if not.
		 */
		bool yellow() {
#warning TODO something sensible
			return true;
		}

		Point position() const {
			return player_.is() ? player_->position() : Point();
		}

		double orientation() const {
			return player_.is() ? player_->orientation() : 0.0;
		}

	private:
		const SimulatorEngine::Ptr engine;
		const Config::RobotInfo &botinfo;
		bool powered_;
		double battery_;
		bool bootloading_;
		uint16_t address16_;
		uint8_t run_data_offset_;
		SimulatorPlayer::Ptr player_;

		SimulatorRobot(const Config::RobotInfo &, SimulatorEngine::Ptr);
};

#endif

