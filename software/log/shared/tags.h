#ifndef LOG_SHARED_TAGS_H
#define LOG_SHARED_TAGS_H

namespace Log {
	/**
	 * The possible packet tag bytes in a log file.
	 */
	enum Tag {
		/**
		 * Tags the start of the log.
		 *
		 * No payload.
		 */
		T_START,

		/**
		 * Tags the end of the log.
		 *
		 * As payload:
		 * \li 1 byte reason code
		 * \li n bytes detail block, format specific to reason
		 */
		T_END,

		/**
		 * Tags output of a debug message.
		 *
		 * As payload:
		 * \li 1 byte log level
		 * \li n bytes string message
		 */
		T_DEBUG,

		/**
		 * Tags activation or deactivation of an annunciator message.
		 *
		 * As payload:
		 * \li 1 byte nonzero for activate message or zero for deactivate
		 * \li n bytes string message
		 */
		T_ANNUNCIATOR,

		/**
		 * Tags a boolean parameter value.
		 *
		 * As payload:
		 * \li 1 byte nonzero for parameter set or zero for cleared
		 * \li n bytes string parameter name
		 */
		T_BOOL_PARAM,

		/**
		 * Tags an integer parameter value.
		 *
		 * As payload:
		 * \li 8 bytes big endian integer new value of parameter
		 * \li n bytes string parameter name
		 */
		T_INT_PARAM,

		/**
		 * Tags a double parameter value (deprecated).
		 *
		 * As payload:
		 * \li 20 bytes string new value of parameter, encoded in ASCII, right-justified, padded with spaces
		 * \li n bytes string parameter name
		 */
		T_DOUBLE_PARAM_OLD,

		/**
		 * Tags reception of an SSL-Vision packet.
		 *
		 * As payload:
		 * \li n bytes raw packet data
		 */
		T_VISION,

		/**
		 * Tags reception of a referee box packet.
		 *
		 * As payload:
		 * \li n bytes raw packet data
		 */
		T_REFBOX,

		/**
		 * Tags reception of new field geometry.
		 *
		 * As payload:
		 * \li 4 bytes big endian integer length from goal to goal, in µm
		 * \li 4 bytes big endian integer total length including boundary and referee area, in µm
		 * \li 4 bytes big endian integer width from sideline to sideline, in µm
		 * \li 4 bytes big endian integer total width including boundary and referee area, in µm
		 * \li 4 bytes big endian integer width of goal from goalpost to goalpost, in µm
		 * \li 4 bytes big endian integer radius of centre circle, in µm
		 * \li 4 bytes big endian integer radius of defense area arcs, in µm
		 * \li 4 bytes big endian integer width of straight parts of defense areas, in µm
		 */
		T_FIELD,

		/**
		 * Tags a change of ball filter.
		 *
		 * As payload:
		 * \li n bytes string name of ball filter implementation
		 */
		T_BALL_FILTER,

		/**
		 * Tags a change of coach.
		 *
		 * As payload:
		 * \li n bytes string name of coach implementation
		 *
		 * Deprecated.
		 */
		T_COACH,

		/**
		 * Tags a change of strategy.
		 *
		 * As payload:
		 * \li n bytes string name of strategy implementation
		 *
		 * Deprecated.
		 */
		T_STRATEGY,

		/**
		 * Tags a change of robot controller.
		 *
		 * As payload:
		 * \li n bytes string name of robot controller implementation
		 */
		T_ROBOT_CONTROLLER,

		/**
		 * Tags a change of play type.
		 *
		 * As payload:
		 * \li 1 byte playtype index
		 */
		T_PLAYTYPE,

		/**
		 * Tags a change of score.
		 *
		 * As payload:
		 * \li 1 byte friendly score
		 * \li 1 byte enemy score
		 */
		T_SCORES,

		/**
		 * Tags information about a friendly player at the start of an AI tick.
		 *
		 * As payload:
		 * \li 1 byte integer robot pattern index
		 * \li 4 bytes big endian integer X position of robot, in µm
		 * \li 4 bytes big endian integer Y position of robot, in µm
		 * \li 4 bytes big endian integer orientation of robot, in µrad
		 * \li 4 bytes big endian integer X velocity of robot, in µm/s
		 * \li 4 bytes big endian integer Y velocity of robot, in µm/s
		 * \li 4 bytes big endian integer angular velocity of robot, in µrad/s
		 * \li 4 bytes big endian integer X acceleration of robot, in µm/s^2
		 * \li 4 bytes big endian integer Y acceleration of robot, in µm/s^2
		 * \li 4 bytes big endian integer angular acceleration of robot, in µrad/s^2
		 * \li 4 bytes big endian integer X position of target, in µm
		 * \li 4 bytes big endian integer Y position of target, in µm
		 * \li 4 bytes big endian integer orientation of target, in µrad
		 * \li 8 bytes big endian integer movement flags mask
		 * \li 1 byte integer movement type code
		 * \li 1 byte integer movement priority
		 * \li 2 bytes big endian integer wheel 1 speed, in quarters of a degree of motor shaft rotation per five ms
		 * \li 2 bytes big endian integer wheel 2 speed, in quarters of a degree of motor shaft rotation per five ms
		 * \li 2 bytes big endian integer wheel 3 speed, in quarters of a degree of motor shaft rotation per five ms
		 * \li 2 bytes big endian integer wheel 4 speed, in quarters of a degree of motor shaft rotation per five ms
		 */
		T_FRIENDLY_ROBOT,

		/**
		 * Tags a single element of a path assigned by the navigator.
		 *
		 * As payload:
		 * \li 1 byte integer robot pattern index
		 * \li 4 bytes big endian integer X position of path element, in µm
		 * \li 4 bytes big endian integer Y position of path element, in µm
		 * \li 4 bytes big endian integer orientation of path element, in µrad
		 * \li 8 bytes big endian integer count of seconds since the epoch of arrival deadline
		 * \li 4 bytes big endian integer count of nanoseconds since the second of arrival deadline
		 */
		T_PATH_ELEMENT,

		/**
		 * Tags information about an enemy robot at the start of an AI tick.
		 *
		 * As payload:
		 * \li 1 byte integer robot pattern index
		 * \li 4 bytes big endian integer X position of robot, in µm
		 * \li 4 bytes big endian integer Y position of robot, in µm
		 * \li 4 bytes big endian integer orientation of robot, in µrad
		 * \li 4 bytes big endian integer X velocity of robot, in µm/s
		 * \li 4 bytes big endian integer Y velocity of robot, in µm/s
		 * \li 4 bytes big endian integer angular velocity of robot, in µrad/s
		 * \li 4 bytes big endian integer X acceleration of robot, in µm/s^2
		 * \li 4 bytes big endian integer Y acceleration of robot, in µm/s^2
		 * \li 4 bytes big endian integer angular acceleration of robot, in µrad/s^2
		 */
		T_ENEMY_ROBOT,

		/**
		 * Tags information about the ball at the start of an AI tick.
		 *
		 * As payload:
		 * \li 4 bytes big endian integer X position of ball, in µm
		 * \li 4 bytes big endian integer Y position of ball, in µm
		 * \li 4 bytes big endian integer X velocity of ball, in µm/s
		 * \li 4 bytes big endian integer Y velocity of ball, in µm/s
		 * \li 4 bytes big endian integer X acceleration of ball, in µm/s^2
		 * \li 4 bytes big endian integer Y acceleration of ball, in µm/s^2
		 */
		T_BALL,

		/**
		 * Tags the end of an AI time tick.
		 *
		 * Payload:
		 * \li 8 bytes big endian integer count of seconds since the UNIX epoch
		 * \li 4 bytes big endian integer count of nanoseconds since the UNIX second
		 * \li 8 bytes big endian integer count of seconds since the monotonic epoch
		 * \li 4 bytes big endian integer count of nanoseconds since the monotonic second
		 */
		T_AI_TICK,

		/**
		 * Tags the name of the backend in use.
		 *
		 * Payload:
		 * \li n bytes string name of backend.
		 */
		T_BACKEND,

		/**
		 * Tags a double parameter value.
		 *
		 * As payload:
		 * \li 8 bytes IEEE754 double-precision-encoded new value of parameter
		 * \li n bytes string parameter name
		 */
		T_DOUBLE_PARAM,

		/**
		 * Tags a change of high level.
		 *
		 * Payload:
		 * \li n bytes string name of high level implementation.
		 */
		T_HIGH_LEVEL,

		/**
		 * Tags a change of friendly team colour.
		 *
		 * Payload:
		 * \li 1 byte 0 = yellow, 1 = blue
		 */
		T_FRIENDLY_COLOUR,
	};

	/**
	 * The possible major reason codes for an application termination.
	 */
	enum EndMajorReason {
		/**
		 * Process terminated normally (e.g. because user closed window).
		 *
		 * No detail block.
		 */
		ER_NORMAL,

		/**
		 * Process terminated due to receiving a signal.
		 *
		 * As detail block:
		 * \li 1 byte signal number
		 */
		ER_SIGNAL,

		/**
		 * Process terminated due to an uncaught exception.
		 *
		 * As detail block:
		 * \li n bytes exception message, UTF-8, no terminating NUL byte
		 */
		ER_EXCEPTION,
	};
};

#endif

