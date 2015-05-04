#include "log/shared/enums.h"
#include <cstdlib>
#include <stdexcept>

Log::PlayType Log::Util::PlayType::to_protobuf(AI::Common::PlayType pt) {
	switch (pt) {
		case AI::Common::PlayType::HALT:
			return Log::PLAY_TYPE_HALT;

		case AI::Common::PlayType::STOP:
			return Log::PLAY_TYPE_STOP;

		case AI::Common::PlayType::PLAY:
			return Log::PLAY_TYPE_PLAY;

		case AI::Common::PlayType::PREPARE_KICKOFF_FRIENDLY:
			return Log::PLAY_TYPE_PREPARE_KICKOFF_FRIENDLY;

		case AI::Common::PlayType::EXECUTE_KICKOFF_FRIENDLY:
			return Log::PLAY_TYPE_EXECUTE_KICKOFF_FRIENDLY;

		case AI::Common::PlayType::PREPARE_KICKOFF_ENEMY:
			return Log::PLAY_TYPE_PREPARE_KICKOFF_ENEMY;

		case AI::Common::PlayType::EXECUTE_KICKOFF_ENEMY:
			return Log::PLAY_TYPE_EXECUTE_KICKOFF_ENEMY;

		case AI::Common::PlayType::PREPARE_PENALTY_FRIENDLY:
			return Log::PLAY_TYPE_PREPARE_PENALTY_FRIENDLY;

		case AI::Common::PlayType::EXECUTE_PENALTY_FRIENDLY:
			return Log::PLAY_TYPE_EXECUTE_PENALTY_FRIENDLY;

		case AI::Common::PlayType::PREPARE_PENALTY_ENEMY:
			return Log::PLAY_TYPE_PREPARE_PENALTY_ENEMY;

		case AI::Common::PlayType::EXECUTE_PENALTY_ENEMY:
			return Log::PLAY_TYPE_EXECUTE_PENALTY_ENEMY;

		case AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY:
			return Log::PLAY_TYPE_EXECUTE_DIRECT_FREE_KICK_FRIENDLY;

		case AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY:
			return Log::PLAY_TYPE_EXECUTE_INDIRECT_FREE_KICK_FRIENDLY;

		case AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY:
			return Log::PLAY_TYPE_EXECUTE_DIRECT_FREE_KICK_ENEMY;

		case AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY:
			return Log::PLAY_TYPE_EXECUTE_INDIRECT_FREE_KICK_ENEMY;

		case AI::Common::PlayType::NONE:
			return Log::PLAY_TYPE_NONE;
	}
	throw std::invalid_argument("Invalid enumeration element.");
}

AI::Common::PlayType Log::Util::PlayType::of_protobuf(Log::PlayType pt) {
	switch (pt) {
		case Log::PLAY_TYPE_HALT:
			return AI::Common::PlayType::HALT;

		case Log::PLAY_TYPE_STOP:
			return AI::Common::PlayType::STOP;

		case Log::PLAY_TYPE_PLAY:
			return AI::Common::PlayType::PLAY;

		case Log::PLAY_TYPE_PREPARE_KICKOFF_FRIENDLY:
			return AI::Common::PlayType::PREPARE_KICKOFF_FRIENDLY;

		case Log::PLAY_TYPE_EXECUTE_KICKOFF_FRIENDLY:
			return AI::Common::PlayType::EXECUTE_KICKOFF_FRIENDLY;

		case Log::PLAY_TYPE_PREPARE_KICKOFF_ENEMY:
			return AI::Common::PlayType::PREPARE_KICKOFF_ENEMY;

		case Log::PLAY_TYPE_EXECUTE_KICKOFF_ENEMY:
			return AI::Common::PlayType::EXECUTE_KICKOFF_ENEMY;

		case Log::PLAY_TYPE_PREPARE_PENALTY_FRIENDLY:
			return AI::Common::PlayType::PREPARE_PENALTY_FRIENDLY;

		case Log::PLAY_TYPE_EXECUTE_PENALTY_FRIENDLY:
			return AI::Common::PlayType::EXECUTE_PENALTY_FRIENDLY;

		case Log::PLAY_TYPE_PREPARE_PENALTY_ENEMY:
			return AI::Common::PlayType::PREPARE_PENALTY_ENEMY;

		case Log::PLAY_TYPE_EXECUTE_PENALTY_ENEMY:
			return AI::Common::PlayType::EXECUTE_PENALTY_ENEMY;

		case Log::PLAY_TYPE_EXECUTE_DIRECT_FREE_KICK_FRIENDLY:
			return AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY;

		case Log::PLAY_TYPE_EXECUTE_INDIRECT_FREE_KICK_FRIENDLY:
			return AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY;

		case Log::PLAY_TYPE_EXECUTE_DIRECT_FREE_KICK_ENEMY:
			return AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY;

		case Log::PLAY_TYPE_EXECUTE_INDIRECT_FREE_KICK_ENEMY:
			return AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY;

		case Log::PLAY_TYPE_NONE:
			return AI::Common::PlayType::NONE;
	}
	throw std::invalid_argument("Invalid enumeration element.");
}

Log::MoveType Log::Util::MoveType::to_protobuf(AI::Flags::MoveType mt) {
	switch (mt) {
		case AI::Flags::MoveType::NORMAL:
			return Log::MOVE_TYPE_NORMAL;

		case AI::Flags::MoveType::DRIBBLE:
			return Log::MOVE_TYPE_DRIBBLE;

		case AI::Flags::MoveType::CATCH:
			return Log::MOVE_TYPE_CATCH;

		case AI::Flags::MoveType::CATCH_PIVOT:
			return Log::MOVE_TYPE_CATCH_PIVOT;

		case AI::Flags::MoveType::INTERCEPT:
			return Log::MOVE_TYPE_INTERCEPT;

		case AI::Flags::MoveType::INTERCEPT_PIVOT:
			return Log::MOVE_TYPE_INTERCEPT_PIVOT;

		case AI::Flags::MoveType::RAM_BALL:
			return Log::MOVE_TYPE_RAM_BALL;

		case AI::Flags::MoveType::HALT:
			return Log::MOVE_TYPE_HALT;

		case AI::Flags::MoveType::PIVOT:
			return Log::MOVE_TYPE_PIVOT;
	}
	throw std::invalid_argument("Invalid enumeration element.");
}

AI::Flags::MoveType Log::Util::MoveType::of_protobuf(Log::MoveType mt) {
	switch (mt) {
		case Log::MOVE_TYPE_NORMAL:
			return AI::Flags::MoveType::NORMAL;

		case Log::MOVE_TYPE_DRIBBLE:
			return AI::Flags::MoveType::DRIBBLE;

		case Log::MOVE_TYPE_CATCH:
			return AI::Flags::MoveType::CATCH;

		case Log::MOVE_TYPE_CATCH_PIVOT:
			return AI::Flags::MoveType::CATCH_PIVOT;

		case Log::MOVE_TYPE_INTERCEPT:
			return AI::Flags::MoveType::INTERCEPT;

		case Log::MOVE_TYPE_INTERCEPT_PIVOT:
			return AI::Flags::MoveType::INTERCEPT_PIVOT;

		case Log::MOVE_TYPE_RAM_BALL:
			return AI::Flags::MoveType::RAM_BALL;

		case Log::MOVE_TYPE_HALT:
			return AI::Flags::MoveType::HALT;

		case Log::MOVE_TYPE_PIVOT:
			return AI::Flags::MoveType::PIVOT;
	}
	throw std::invalid_argument("Invalid enumeration element.");
}

Log::MovePrio Log::Util::MovePrio::to_protobuf(AI::Flags::MovePrio mp) {
	switch (mp) {
		case AI::Flags::MovePrio::HIGH:
			return Log::MOVE_PRIO_HIGH;

		case AI::Flags::MovePrio::MEDIUM:
			return Log::MOVE_PRIO_MEDIUM;

		case AI::Flags::MovePrio::LOW:
			return Log::MOVE_PRIO_LOW;
	}
	throw std::invalid_argument("Invalid enumeration element.");
}

AI::Flags::MovePrio Log::Util::MovePrio::of_protobuf(Log::MovePrio mp) {
	switch (mp) {
		case Log::MOVE_PRIO_HIGH:
			return AI::Flags::MovePrio::HIGH;

		case Log::MOVE_PRIO_MEDIUM:
			return AI::Flags::MovePrio::MEDIUM;

		case Log::MOVE_PRIO_LOW:
			return AI::Flags::MovePrio::LOW;
	}
	throw std::invalid_argument("Invalid enumeration element.");
}

