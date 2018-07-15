#pragma once
#include "ai/common/playtype.h"
#include "ai/flags.h"
#include "drive/robot.h"
#include "proto/log_record.pb.h"

namespace Log
{
namespace Util
{
namespace PlayType
{
/**
 * \brief Converts an AI play type to a Protobuf play type.
 *
 * \param[in] pt the AI play type to convert.
 *
 * \return the Protobuf play type.
 */
Log::PlayType to_protobuf(AI::Common::PlayType pt);

/**
 * \brief Converts a Protobuf play type to an AI play type.
 *
 * \param[in] pt the Protobuf play type to convert.
 *
 * \return the AI play type.
 */
AI::Common::PlayType of_protobuf(Log::PlayType pt);
}

namespace MovePrio
{
/**
 * \brief Converts an AI movement priority to a Protobuf movement priority.
 *
 * \param[in] mp the AI movement priority to convert.
 *
 * \return the Protobuf movement priority.
 */
Log::MovePrio to_protobuf(AI::Flags::MovePrio mp);

/**
 * \brief Converts a Protobuf movement priority to an AI movement priority.
 *
 * \param[in] mp the Protobuf movement priority to convert.
 *
 * \return the AI movement priority.
 */
AI::Flags::MovePrio of_protobuf(Log::MovePrio mp);
}

namespace Primitive
{
/**
 * \brief Converts an AI primitive to a log primitive.
 */
Log::Tick::FriendlyRobot::HLPrimitive::Primitive to_protobuf(
    Drive::Primitive mp);
}
}
}
