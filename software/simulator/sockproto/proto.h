#ifndef SIMULATOR_SOCKPROTO_PROTO_H
#define SIMULATOR_SOCKPROTO_PROTO_H

#include "ai/common/playtype.h"
#include "util/time.h"

/**
 * The filename of the socket, in the user's cache directory.
 */
#define SIMULATOR_SOCKET_FILENAME "thunderbots-simulator-socket"

namespace Simulator {
	namespace Proto {
		/**
		 * The maximum number of players per team.
		 */
		const unsigned int MAX_PLAYERS_PER_TEAM = 10;

		/**
		 * A collection of orders for a single player.
		 */
		struct A2SPlayerInfo {
			/**
			 * The pattern index of the robot to whom the orders are directed,
			 * or std::numeric_limits<unsigned int>::max() if this structure is unused.
			 */
			unsigned int pattern;

			/**
			 * Whether to kick the ball.
			 */
			bool kick;

			/**
			 * Whether to chip the ball.
			 */
			bool chip;

			/**
			 * The power level to use kicking or chipping.
			 */
			double chick_power;

			/**
			 * The target speeds of the four wheels,
			 * in quarters of a degree of motor shaft rotation per five milliseconds.
			 */
			int wheel_speeds[4];
		};

		/**
		 * Information about how to drag an object.
		 */
		struct A2SDragInfo {
			/**
			 * The pattern of the robot to drag, in the A2S_PACKET_DRAG_PLAYER case.
			 */
			unsigned int pattern;

			/**
			 * The X position to drag to.
			 */
			double x;

			/**
			 * The Y position to drag to.
			 */
			double y;
		};

		/**
		 * The possible speed modes.
		 */
		enum SpeedMode {
			/**
			 * The simulator is running at real-time speed.
			 */
			SPEED_MODE_NORMAL,

			/**
			 * The simulator is running as quickly as possible.
			 */
			SPEED_MODE_FAST,

			/**
			 * The simulator is running more slowly than real time.
			 */
			SPEED_MODE_SLOW,
		};

		/**
		 * The possible packet types that can be sent from the AI to the simulator.
		 */
		enum A2SPacketType {
			/**
			 * Carries orders issued by the AI to the players.
			 * There must be a one-to-one correspondence between packets of this type and packets of type S2A_PACKET_TICK.
			 * The AI must always wait until it receivse an S2A_PACKET_TICK, then reply with exactly one A2S_PACKET_PLAYERS.
			 */
			A2S_PACKET_PLAYERS,

			/**
			 * Requests that the simulator create and place a new player for the team.
			 * The pattern must not already exist on the team.
			 * Packets of this type, A2S_PACKET_REMOVE_PLAYER, and A2S_SET_STATE should be limited to one per S2A_PACKET_TICK.
			 */
			A2S_PACKET_ADD_PLAYER,

			/**
			 * Requests that the simulator remove a player from the team.
			 * The pattern must exist on the team.
			 * Packets of this type, A2S_PACKET_ADD_PLAYER, and A2S_SET_STATE should be limited to one per S2A_PACKET_TICK.
			 */
			A2S_PACKET_REMOVE_PLAYER,

			/**
			 * Requests that the simulator switch to a new speed mode.
			 * This may be sent at any time.
			 */
			A2S_PACKET_SET_SPEED,

			/**
			 * Requests that the simulator change play types.
			 * This may be sent at any time.
			 */
			A2S_PACKET_PLAY_TYPE,

			/**
			 * Requests that the simulator drag the ball to a new position.
			 * This may be sent at any time.
			 */
			A2S_PACKET_DRAG_BALL,

			/**
			 * Requests that the simulator drag a friendly player to a new position.
			 * This may be sent at any time.
			 */
			A2S_PACKET_DRAG_PLAYER,

			/**
			 * Requests that the simulator load the current state of the world from an opaque block of bytes in a file.
			 * The state must have been saved by a simulator running the same engine.
			 * Packets of this type, A2S_PACKET_ADD_PLAYER, and A2S_REMOVE_PLAYER should be limited to one per S2A_PACKET_TICK.
			 * This packet must be associated with an ancillary file descriptor for the file.
			 */
			A2S_PACKET_LOAD_STATE,

			/**
			 * Requests that the simulator send the current state of the world as an opaque block of bytes in a file.
			 * This may be sent at any time.
			 * This packet must be associated with an ancillary file descriptor for the file.
			 */
			A2S_PACKET_SAVE_STATE,
		};

		/**
		 * The layout of a packet sent from the AI to the simulator.
		 */
		struct A2SPacket {
			/**
			 * The type of packet.
			 */
			A2SPacketType type;

			union {
				/**
				 * The orders given to the players, in the case of A2S_PACKET_PLAYERS.
				 */
				A2SPlayerInfo players[MAX_PLAYERS_PER_TEAM];

				/**
				 * The pattern index of the robot to add or remove,
				 * in the case of A2S_PACKET_ADD_PLAYER or A2S_PACKET_REMOVE_PLAYER.
				 */
				unsigned int pattern;

				/**
				 * The speed mode, in the case of A2S_PACKET_SPEED_MDOE.
				 */
				SpeedMode speed_mode;

				/**
				 * The play type, in the case of A2S_PACKET_PLAY_TYPE.
				 */
				AI::Common::PlayType::PlayType playtype;

				/**
				 * Information about dragging an object, in the case of A2S_PACKET_DRAG_BALL or A2S_PACKET_DRAG_PLAYER.
				 */
				A2SDragInfo drag;
			};
		};

		/**
		 * The information about the ball sent from the simulator to an AI.
		 */
		struct S2ABallInfo {
			/**
			 * The X position of the ball.
			 */
			double x;

			/**
			 * The Y position of the ball.
			 */
			double y;
		};

		/**
		 * The information about a robot (on either team) sent from the simulator to an AI.
		 */
		struct S2ARobotInfo {
			/**
			 * The pattern index of the player,
			 * or std::numeric_limits<unsigned int>::max() if this structure is unused.
			 */
			unsigned int pattern;

			/**
			 * The X position of the player.
			 */
			double x;

			/**
			 * The Y position of the player.
			 */
			double y;

			/**
			 * The orientation of the player.
			 */
			double orientation;
		};

		/**
		 * The information about a friendly player sent from the simulator to an AI.
		 */
		struct S2APlayerInfo {
			/**
			 * The basic information common to robots on both teams.
			 */
			S2ARobotInfo robot_info;

			/**
			 * Whether or not the player currently has the ball.
			 */
			bool has_ball;
		};

		/**
		 * The possible packet types that can be sent from the simulator to an AI.
		 */
		enum S2APacketType {
			/**
			 * Carries the state of the world and orders the AI to begin a time tick.
			 */
			S2A_PACKET_TICK,

			/**
			 * Indicates which speed mode is currently active.
			 * This packet is sent when the client connects and whenever the speed mode changes.
			 */
			S2A_PACKET_SPEED_MODE,

			/**
			 * Indicates the current play type.
			 * This packet is sent when the client connects and whenever the play type changes.
			 */
			S2A_PACKET_PLAY_TYPE,
		};

		/**
		 * The layout of a packet sent from the simulator to an AI.
		 */
		struct S2APacket {
			/**
			 * The type of packet.
			 */
			S2APacketType type;

			union {
				/**
				 * The state of the world, in the case of S2A_PACKET_TICK.
				 */
				struct {
					/**
					 * The state of the ball.
					 */
					S2ABallInfo ball;

					/**
					 * The states of the friendly players.
					 */
					S2APlayerInfo friendly[MAX_PLAYERS_PER_TEAM];

					/**
					 * The states of the enemy robots.
					 */
					S2ARobotInfo enemy[MAX_PLAYERS_PER_TEAM];

					/**
					 * The monotonic timestamp of the tick.
					 */
					timespec stamp;

					/**
					 * The friendly team's score.
					 */
					unsigned int friendly_score;

					/**
					 * The enemy team's score.
					 */
					unsigned int enemy_score;
				} world_state;

				/**
				 * The current speed mode, in the case of S2A_PACKET_SPEED_MODE.
				 */
				SpeedMode speed_mode;

				/**
				 * The current play type, in the case of S2A_PACKET_PLAY_TYPE.
				 */
				AI::Common::PlayType::PlayType playtype;
			};
		};
	}
}

#endif

