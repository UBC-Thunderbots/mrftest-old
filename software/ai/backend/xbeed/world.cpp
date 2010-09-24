#if 0
bool World::on_vision_readable(Glib::IOCondition) {
}

void World::override_playtype(PlayType::PlayType pt) {
	if (pt != playtype_override || !playtype_override_active) {
		playtype_override = pt;
		playtype_override_active = true;
		update_playtype();
	}
}

void World::clear_playtype_override() {
	if (playtype_override_active) {
		playtype_override_active = false;
		update_playtype();
	}
}

double World::playtype_time() const {
	timespec now;
	timespec_now(now);
	timespec diff;
	timespec_sub(now, playtype_time_, diff);
	return timespec_to_double(diff);
}

void World::update_playtype() {
	PlayType::PlayType old_pt = playtype_;
	if (refbox_yellow_) {
		old_pt = PlayType::INVERT[old_pt];
	}
	PlayType::PlayType new_pt = compute_playtype(old_pt);
	if (refbox_yellow_) {
		new_pt = PlayType::INVERT[new_pt];
	}
	if (new_pt != playtype_) {
		LOG_DEBUG(Glib::ustring::compose("Play type changed to %1.", PlayType::DESCRIPTIONS_GENERIC[new_pt]));
		playtype_ = new_pt;
		signal_playtype_changed.emit();

		timespec_now(playtype_time_);
	}
}

PlayType::PlayType World::compute_playtype(PlayType::PlayType old_pt) {
	if (playtype_override_active) {
		return playtype_override;
	}

	switch (refbox_.command()) {
		case 'H': // HALT
		case 'h': // HALF TIME
		case 't': // TIMEOUT YELLOW
		case 'T': // TIMEOUT BLUE
			return PlayType::HALT;

		case 'S': // STOP
		case 'z': // END TIMEOUT
			return PlayType::STOP;

		case ' ': // NORMAL START
			switch (old_pt) {
				case PlayType::PREPARE_KICKOFF_FRIENDLY:
					playtype_arm_ball_position = ball_.position();
					return PlayType::EXECUTE_KICKOFF_FRIENDLY;

				case PlayType::PREPARE_KICKOFF_ENEMY:
					playtype_arm_ball_position = ball_.position();
					return PlayType::EXECUTE_KICKOFF_ENEMY;

				case PlayType::PREPARE_PENALTY_FRIENDLY:
					playtype_arm_ball_position = ball_.position();
					return PlayType::EXECUTE_PENALTY_FRIENDLY;

				case PlayType::PREPARE_PENALTY_ENEMY:
					playtype_arm_ball_position = ball_.position();
					return PlayType::EXECUTE_PENALTY_ENEMY;

				case PlayType::EXECUTE_KICKOFF_FRIENDLY:
				case PlayType::EXECUTE_KICKOFF_ENEMY:
				case PlayType::EXECUTE_PENALTY_FRIENDLY:
				case PlayType::EXECUTE_PENALTY_ENEMY:
					if ((ball_.position() - playtype_arm_ball_position).len() > BALL_FREE_DISTANCE) {
						return PlayType::PLAY;
					} else {
						return old_pt;
					}

				default:
					return PlayType::PLAY;
			}

		case 'f': // DIRECT FREE KICK YELLOW
			if (old_pt == PlayType::PLAY) {
				return PlayType::PLAY;
			} else if (old_pt == PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY) {
				if ((ball_.position() - playtype_arm_ball_position).len() > BALL_FREE_DISTANCE) {
					return PlayType::PLAY;
				} else {
					return PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY;
				}
			} else {
				playtype_arm_ball_position = ball_.position();
				return PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY;
			}

		case 'F': // DIRECT FREE KICK BLUE
			if (old_pt == PlayType::PLAY) {
				return PlayType::PLAY;
			} else if (old_pt == PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY) {
				if ((ball_.position() - playtype_arm_ball_position).len() > BALL_FREE_DISTANCE) {
					return PlayType::PLAY;
				} else {
					return PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY;
				}
			} else {
				playtype_arm_ball_position = ball_.position();
				return PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY;
			}

		case 'i': // INDIRECT FREE KICK YELLOW
			if (old_pt == PlayType::PLAY) {
				return PlayType::PLAY;
			} else if (old_pt == PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY) {
				if ((ball_.position() - playtype_arm_ball_position).len() > BALL_FREE_DISTANCE) {
					return PlayType::PLAY;
				} else {
					return PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY;
				}
			} else {
				playtype_arm_ball_position = ball_.position();
				return PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY;
			}

		case 'I': // INDIRECT FREE KICK BLUE
			if (old_pt == PlayType::PLAY) {
				return PlayType::PLAY;
			} else if (old_pt == PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY) {
				if ((ball_.position() - playtype_arm_ball_position).len() > BALL_FREE_DISTANCE) {
					return PlayType::PLAY;
				} else {
					return PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY;
				}
			} else {
				playtype_arm_ball_position = ball_.position();
				return PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY;
			}

		case 's': // FORCE START
			return PlayType::PLAY;

		case 'k': // KICKOFF YELLOW
			return PlayType::PREPARE_KICKOFF_ENEMY;

		case 'K': // KICKOFF BLUE
			return PlayType::PREPARE_KICKOFF_FRIENDLY;

		case 'p': // PENALTY YELLOW
			return PlayType::PREPARE_PENALTY_ENEMY;

		case 'P': // PENALTY BLUE
			return PlayType::PREPARE_PENALTY_FRIENDLY;

		case '1': // BEGIN FIRST HALF
		case '2': // BEGIN SECOND HALF
		case 'o': // BEGIN OVERTIME 1
		case 'O': // BEGIN OVERTIME 2
		case 'a': // BEGIN PENALTY SHOOTOUT
		case 'g': // GOAL YELLOW
		case 'G': // GOAL BLUE
		case 'd': // REVOKE GOAL YELLOW
		case 'D': // REVOKE GOAL BLUE
		case 'y': // YELLOW CARD YELLOW
		case 'Y': // YELLOW CARD BLUE
		case 'r': // RED CARD YELLOW
		case 'R': // RED CARD BLUE
		case 'c': // CANCEL
		default:
			return old_pt;
	}
}
#endif

