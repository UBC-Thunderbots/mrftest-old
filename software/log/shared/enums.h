#ifndef LOG_SHARED_ENUMS_H
#define LOG_SHARED_ENUMS_H

#include "ai/flags.h"
#include "ai/common/playtype.h"
#include "ai/common/team.h"
#include "proto/log_record.pb.h"

namespace Log {
	namespace Util {
		namespace Colour {
			/**
			 * \brief Converts an AI colour to a Protobuf colour.
			 *
			 * \param[in] clr the AI colour to convert.
			 *
			 * \return the Protobuf colour.
			 */
			Log::Colour to_protobuf(AI::Common::Colour clr);

			/**
			 * \brief Converts a Protobuf colour to an AI colour.
			 *
			 * \param[in] clr the Protobuf colour to convert.
			 *
			 * \return the AI colour.
			 */
			AI::Common::Colour of_protobuf(Log::Colour clr);
		}

		namespace PlayType {
			/**
			 * \brief Converts an AI play type to a Protobuf play type.
			 *
			 * \param[in] clr the AI play type to convert.
			 *
			 * \return the Protobuf play type.
			 */
			Log::PlayType to_protobuf(AI::Common::PlayType pt);

			/**
			 * \brief Converts a Protobuf play type to an AI play type.
			 *
			 * \param[in] clr the Protobuf play type to convert.
			 *
			 * \return the AI play type.
			 */
			AI::Common::PlayType of_protobuf(Log::PlayType pt);
		}

		namespace MoveType {
			/**
			 * \brief Converts an AI movement type to a Protobuf movement type.
			 *
			 * \param[in] clr the AI movement type to convert.
			 *
			 * \return the Protobuf movement type.
			 */
			Log::MoveType to_protobuf(AI::Flags::MoveType pt);

			/**
			 * \brief Converts a Protobuf movement type to an AI movement type.
			 *
			 * \param[in] clr the Protobuf movement type to convert.
			 *
			 * \return the AI movement type.
			 */
			AI::Flags::MoveType of_protobuf(Log::MoveType pt);
		}

		namespace MovePrio {
			/**
			 * \brief Converts an AI movement priority to a Protobuf movement priority.
			 *
			 * \param[in] clr the AI movement priority to convert.
			 *
			 * \return the Protobuf movement priority.
			 */
			Log::MovePrio to_protobuf(AI::Flags::MovePrio pt);

			/**
			 * \brief Converts a Protobuf movement priority to an AI movement priority.
			 *
			 * \param[in] clr the Protobuf movement priority to convert.
			 *
			 * \return the AI movement priority.
			 */
			AI::Flags::MovePrio of_protobuf(Log::MovePrio pt);
		}
	}
}

#endif

