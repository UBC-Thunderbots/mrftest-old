#include "ai/common/playtype.h"

const Glib::ustring AI::Common::PlayType::DESCRIPTIONS_GENERIC[COUNT] = {
	"Halt",
	"Stop",
	"Play",
	"Prep Kickoff Friendly",
	"Kickoff Friendly",
	"Prep Kickoff Enemy",
	"Kickoff Enemy",
	"Prep Penalty Friendly",
	"Penalty Friendly",
	"Prep Penalty Enemy",
	"Penalty Enemy",
	"Direct Free Friendly",
	"Indirect Free Friendly",
	"Direct Free Enemy",
	"Indirect Free Enemy",
};

const Glib::ustring AI::Common::PlayType::DESCRIPTIONS_WEST[COUNT] = {
	"Halt",
	"Stop",
	"Play",
	"Prep Kickoff West",
	"Kickoff West",
	"Prep Kickoff East",
	"Kickoff East",
	"Prep Penalty West",
	"Penalty West",
	"Prep Penalty East",
	"Penalty East",
	"Direct Free West",
	"Indirect Free West",
	"Direct Free East",
	"Indirect Free East",
};

const AI::Common::PlayType::PlayType AI::Common::PlayType::INVERT[COUNT] = {
	HALT,
	STOP,
	PLAY,
	PREPARE_KICKOFF_ENEMY,
	EXECUTE_KICKOFF_ENEMY,
	PREPARE_KICKOFF_FRIENDLY,
	EXECUTE_KICKOFF_FRIENDLY,
	PREPARE_PENALTY_ENEMY,
	EXECUTE_PENALTY_ENEMY,
	PREPARE_PENALTY_FRIENDLY,
	EXECUTE_PENALTY_FRIENDLY,
	EXECUTE_DIRECT_FREE_KICK_ENEMY,
	EXECUTE_INDIRECT_FREE_KICK_ENEMY,
	EXECUTE_DIRECT_FREE_KICK_FRIENDLY,
	EXECUTE_INDIRECT_FREE_KICK_FRIENDLY,
};

