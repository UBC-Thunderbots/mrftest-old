#ifndef XBEE_DAEMON_FRONTEND_ROBOT_STATE_H
#define XBEE_DAEMON_FRONTEND_ROBOT_STATE_H

#include "util/byref.h"
#include "xbee/shared/packettypes.h"
#include <cstddef>
#include <ctime>
#include <glibmm.h>
#include <stdint.h>

class client;
class daemon;

/*
 * All the data corresponding to a single robot tracked by this arbiter.
 */
class robot_state : public byref {
	public:
		/**
		 * A pointer to a robot_state.
		 */
		typedef Glib::RefPtr<robot_state> ptr;

		/**
		 * An individual state that a robot can be in is represented by a
		 * subclass of this class, and the current state of the robot is held in
		 * a pointer.
		 */
		class state : public byref, public sigc::trackable {
			public:
				/**
				 * A pointer to a state.
				 */
				typedef Glib::RefPtr<state> ptr;

				/**
				 * Switches the robot into raw mode.
				 *
				 * \param cli the client who requested the claim
				 */
				virtual void enter_raw_mode(client *cli) = 0;

				/**
				 * Attempts to switch the robot into drive mode. May throw
				 * resource_allocation_error if insufficient resources are
				 * available.
				 *
				 * \param cli the client who requested the claim
				 */
				virtual void enter_drive_mode(client *cli) = 0;

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
				virtual void on_feedback(uint8_t rssi, const xbeepacket::FEEDBACK_DATA &packet, const timespec &latency) = 0;

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

		class idle_state;
		class raw_state;
		class setting16_state;
		class settingrdo_state;
		class alive_state;
		class releasing16_state;
		class bootloading_high_state;
		class bootloading_low_state;
		class bootloading_low_to_setting16_state;

		/**
		 * Constructs a new state corresponding to a dead, unclaimed robot.
		 *
		 * \param address64 the 64-bit address of the robot to create
		 *
		 * \param daemon the dæmon used to communicate with this robot
		 *
		 * \return A new robot_state object
		 */
		static ptr create(uint64_t address64, class daemon &daemon);

		/**
		 * Switches the robot into raw mode.
		 *
		 * \param cli the client who requested the claim
		 */
		void enter_raw_mode(client *cli);

		/**
		 * Attempts to switch the robot into drive mode. May throw
		 * resource_allocation_error if insufficient resources are available.
		 *
		 * \param cli the client who requested the claim
		 */
		void enter_drive_mode(client *cli);

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
		void on_feedback(uint8_t rssi, const xbeepacket::FEEDBACK_DATA &packet, const timespec &latency);

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
		state::ptr state_;
		class daemon &daemon;

		robot_state(uint64_t address64, class daemon &daemon);

		friend class idle_state;
		friend class raw_state;
		friend class setting16_state;
		friend class settingrdo_state;
		friend class alive_state;
		friend class releasing16_state;
		friend class bootloading_high_state;
		friend class bootloading_low_state;
		friend class bootloading_low_to_setting16_state;
};

#endif

