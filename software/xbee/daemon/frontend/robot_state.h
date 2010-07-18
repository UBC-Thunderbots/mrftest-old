#ifndef XBEE_DAEMON_FRONTEND_ROBOT_STATE_H
#define XBEE_DAEMON_FRONTEND_ROBOT_STATE_H

#include "util/byref.h"
#include "xbee/shared/packettypes.h"
#include <cstddef>
#include <ctime>
#include <glibmm.h>
#include <stdint.h>

class XBeeClient;
class XBeeDaemon;

/*
 * All the data corresponding to a single robot tracked by this arbiter.
 */
class XBeeRobot : public ByRef {
	public:
		/**
		 * A pointer to a XBeeRobot.
		 */
		typedef Glib::RefPtr<XBeeRobot> ptr;

		/**
		 * An individual state that a robot can be in is represented by a
		 * subclass of this class, and the current state of the robot is held in
		 * a pointer.
		 */
		class RobotState : public ByRef, public sigc::trackable {
			public:
				/**
				 * A pointer to a RobotState.
				 */
				typedef Glib::RefPtr<RobotState> ptr;

				/**
				 * Switches the robot into raw mode.
				 *
				 * \param cli the XBeeClient who requested the claim
				 */
				virtual void enter_raw_mode(XBeeClient *cli) = 0;

				/**
				 * Attempts to switch the robot into drive mode. May throw
				 * resource_allocation_error if insufficient resources are
				 * available.
				 *
				 * \param cli the XBeeClient who requested the claim
				 */
				virtual void enter_drive_mode(XBeeClient *cli) = 0;

				/**
				 * Unclaims the robot and, if it was in drive mode, starts
				 * releasing resources.
				 */
				virtual void release() = 0;

				/**
				 * Invoked when a feedback packet is received over the radio
				 * from this robot.
				 *
				 * \param rssi the signal strength of the received packet
				 *
				 * \param packet the received data
				 *
				 * \param latency the amount of time the packet took to arrive
				 */
				virtual void on_feedback(uint8_t rssi, const XBeePacketTypes::FEEDBACK_DATA &packet, const timespec &latency) = 0;

				/**
				 * Invoked when a feedback packet was expected from this robot
				 * but was not received.
				 */
				virtual void on_feedback_timeout() = 0;

				/**
				 * \return true if this robot is claimed by some client, or
				 * false if not
				 */
				virtual bool claimed() const = 0;

				/**
				 * \return true if this robot is in the process of freeing
				 * resources from a prior drive-mode claim, or false if not
				 */
				virtual bool freeing() const = 0;

				/**
				 * \return The 16-bit address of the robot, or 0 if no address
				 * has been allocated.
				 */
				virtual uint16_t address16() const = 0;

				/**
				 * \return The index within the run data packet where data for
				 * this robot is stored.
				 */
				virtual uint8_t run_data_index() const = 0;
		};

		class IdleState;
		class RawState;
		class Setting16State;
		class SettingRDOState;
		class AliveState;
		class Releasing16State;
		class BootloadingHighState;
		class BootloadingLowState;
		class BootloadingLowToSetting16State;

		/**
		 * Constructs a new RobotState corresponding to a dead, unclaimed robot.
		 *
		 * \param address64 the 64-bit address of the robot to create
		 *
		 * \param daemon the dæmon used to communicate with this robot
		 *
		 * \return A new XBeeRobot object
		 */
		static ptr create(uint64_t address64, class XBeeDaemon &daemon);

		/**
		 * Switches the robot into raw mode.
		 *
		 * \param cli the XBeeClient who requested the claim
		 */
		void enter_raw_mode(XBeeClient *cli);

		/**
		 * Attempts to switch the robot into drive mode. May throw
		 * resource_allocation_error if insufficient resources are available.
		 *
		 * \param cli the XBeeClient who requested the claim
		 */
		void enter_drive_mode(XBeeClient *cli);

		/**
		 * Unclaims the robot and, if it was in drive mode, starts releasing
		 * resources.
		 */
		void release();

		/**
		 * Invoked when a feedback packet is received over the radio from this
		 * robot.
		 *
		 * \param rssi the signal strength of the received packet
		 *
		 * \param packet the received data
		 *
		 * \param latency the amount of time the packet took to arrive
		 */
		void on_feedback(uint8_t rssi, const XBeePacketTypes::FEEDBACK_DATA &packet, const timespec &latency);

		/**
		 * Invoked when a feedback packet was expected from this robot but was
		 * not received.
		 */
		void on_feedback_timeout();

		/**
		 * \return true if this robot is claimed by some client, or false if not
		 */
		bool claimed() const;

		/**
		 * \return true if this robot is in the process of freeing resources
		 * from a prior drive-mode claim, or false if not
		 */
		bool freeing() const;

		/**
		 * \return The 16-bit address of the robot, or 0 if no address has been
		 * allocated.
		 */
		uint16_t address16() const;

		/**
		 * \return The index within the run data packet where data for this
		 * robot is stored.
		 */
		uint8_t run_data_index() const;

		/**
		 * The 64-bit address of the robot.
		 */
		const uint64_t address64;

		/**
		 * Fired when all allocated resources have been assigned and the robot is
		 * alive and ready to drive.
		 */
		sigc::signal<void> signal_alive;

		/**
		 * Fired when the robot was formerly alive but has stopped responding.
		 */
		sigc::signal<void> signal_dead;

		/**
		 * Fired when all assigned resources have been freed during a shutdown
		 * procedure.
		 */
		sigc::signal<void> signal_resources_freed;

		/**
		 * Fired when a feedback packet is received.
		 */
		sigc::signal<void> signal_feedback;

	private:
		RobotState::ptr state_;
		class XBeeDaemon &daemon;

		XBeeRobot(uint64_t address64, class XBeeDaemon &daemon);

		friend class IdleState;
		friend class RawState;
		friend class Setting16State;
		friend class SettingRDOState;
		friend class AliveState;
		friend class Releasing16State;
		friend class BootloadingHighState;
		friend class BootloadingLowState;
		friend class BootloadingLowToSetting16State;
};

#endif

