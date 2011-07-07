#include "ai/hl/stp/param.h"

DoubleParam AI::HL::STP::shoot_accuracy("Angle threshold (in degrees) that defines shoot accuracy, bigger is more accurate", "STP/shoot", 0.0, -180.0, 180.0);

DoubleParam AI::HL::STP::shoot_width("Shoot accuracy (for various purposes)", "STP/shoot", 5, 0.0, 180.0);

DoubleParam AI::HL::STP::min_pass_dist("Minimum distance for pass play", "STP/Pass", 0.5, 0.0, 5.0);

DoubleParam AI::HL::STP::min_shoot_region("minimum region available for baller_can_shoot to be true (degrees)", "STP/param", 0.1 / M_PI * 180.0, 0, 180);

DoubleParam AI::HL::STP::Action::alpha("Decay constant for the ball velocity", "STP/Action/shoot", 0.1, 0.0, 1.0);

DoubleParam AI::HL::STP::Action::passer_angle_threshold("Angle threshold (in degrees) that defines passing accuracy, smaller is more accurate", "STP/Action/shoot", 5, 0.0, 90.0);

DoubleParam AI::HL::STP::passee_angle_threshold("Angle threshold (in degrees) that the passee must be with respect to passer when shot, smaller is more accurate", "STP/Pass", 80.0, 0.0, 90.0);

DoubleParam AI::HL::STP::Action::pass_speed("kicking speed for making a pass", "STP/Pass", 4.75, 1.0, 10.0);

DoubleParam AI::HL::STP::Action::target_region_param(" the buffer (meters) in which passee must be with repect to target region before valid ", "STP/Tactic/pass", 0.0, 0.0, 5.0);

BoolParam AI::HL::STP::Tactic::random_penalty_goalie("Whether the penalty goalie should choose random points", "STP/Tactic/penalty_goalie",  false);
