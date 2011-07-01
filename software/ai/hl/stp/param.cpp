#include "ai/hl/stp/param.h"

DoubleParam AI::HL::STP::pass_width("Width for passing (robot radius)", "STP/offense", 2.0, 0.0, 9);

DoubleParam AI::HL::STP::min_pass_dist("Minimum distance for pass play", "STP/Pass", 1.0, 0.0, 5.0);

DoubleParam AI::HL::STP::min_shoot_region("minimum region available for baller_can_shoot to be true (degrees)", "STP/param", 0.1 / M_PI * 180.0, 0, 180);

DoubleParam AI::HL::STP::Action::alpha("Decay constant for the ball velocity", "STP/Action/shoot", 0.1, 0.0, 1.0);

DoubleParam AI::HL::STP::Action::pass_threshold("Angle threshold (in degrees) that defines passing accuracy, smaller is more accurate", "STP/Action/shoot", 20.0, 0.0, 90.0);

DoubleParam AI::HL::STP::Action::recieve_threshold("Angle threshold (in degrees) that the reciever must be with respect to passer when shot, smaller is more accurate", "STP/Action/shoot", 80.0, 0.0, 90.0);

DoubleParam AI::HL::STP::Action::pass_speed("kicking speed for making a pass", "STP/Action/shoot", 7.0, 1.0, 10.0);

DoubleParam AI::HL::STP::Action::target_region_param(" the buffer (meters) in which passee must be with repect to target region before valid ", "STP/Tactic/pass", 0.0, 0.0, 5.0);
