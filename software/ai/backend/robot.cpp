#include "ai/backend/robot.h"
#include <stdexcept>
#include "ai/backend/backend.h"

using AI::BE::Robot;

namespace
{
const Predictor<double>::Timediff TIME_CONSTANT(
    std::chrono::duration_cast<Predictor<double>::Timediff>(
        std::chrono::duration<double>(99.0)));
}

Robot::Robot(unsigned int pattern)
    : pattern_(pattern),
      avoid_distance_(AI::Flags::AvoidDistance::MEDIUM),
      pred(
          1.3e-3, 2, TIME_CONSTANT, Angle::of_radians(1.3e-3),
          Angle::of_radians(2), TIME_CONSTANT)
{
}

void Robot::add_field_data(Point pos, Angle ori, AI::Timestamp ts)
{
    pred.add_measurement(
        pos, ori,
        ts - std::chrono::duration_cast<AI::Timediff>(
                 std::chrono::duration<double>(AI::BE::LOOP_DELAY)));
    update_caches();
}

ObjectStore &Robot::object_store() const
{
    return object_store_;
}

unsigned int Robot::pattern() const
{
    return pattern_;
}

Point Robot::position(double delta) const
{
    return pred.value(delta).first.first;
}

Point Robot::position_stdev(double delta) const
{
    return pred.value(delta).second.first;
}

Angle Robot::orientation(double delta) const
{
    return pred.value(delta).first.second;
}

Angle Robot::orientation_stdev(double delta) const
{
    return pred.value(delta).second.second;
}

Point Robot::velocity(double delta) const
{
    return pred.value(delta, 1).first.first;
}

Point Robot::velocity_stdev(double delta) const
{
    return pred.value(delta, 1).second.first;
}

Angle Robot::avelocity(double delta) const
{
    return pred.value(delta, 1).first.second;
}

Angle Robot::avelocity_stdev(double delta) const
{
    return pred.value(delta, 1).second.second;
}

bool Robot::has_display_path() const
{
    return false;
}

const std::vector<Point> &Robot::display_path() const
{
    throw std::logic_error("This robot has no path");
}

void Robot::pre_tick()
{
    avoid_distance_ = AI::Flags::AvoidDistance::MEDIUM;
}

void Robot::lock_time(AI::Timestamp now)
{
    pred.lock_time(now);
    update_caches();
}

Visualizable::Colour Robot::visualizer_colour() const
{
    return Visualizable::Colour(1.0, 0.0, 0.0);
}

Glib::ustring Robot::visualizer_label() const
{
    return Glib::ustring::format(pattern());
}

bool Robot::highlight() const
{
    return false;
}

Visualizable::Colour Robot::highlight_colour() const
{
    return Visualizable::Colour(0.0, 0.0, 0.0);
}

unsigned int Robot::num_bar_graphs() const
{
    return 0;
}

double Robot::bar_graph_value(unsigned int) const
{
    throw std::logic_error("This robot has no graphs");
}

Visualizable::Colour Robot::bar_graph_colour(unsigned int) const
{
    throw std::logic_error("This robot has no graphs");
}

void Robot::update_caches()
{
    position_cached    = position(0.0);
    orientation_cached = orientation(0.0);
    velocity_cached    = velocity(0.0);
    avelocity_cached   = avelocity(0.0);
}

bool Robot::replace(double x, double y, double dir, int id, bool is_yellow)
{
    this->replace_robot_x         = x;
    this->replace_robot_y         = y;
    this->replace_robot_dir       = dir;
    this->replace_robot_id        = id;
    this->replace_robot_is_yellow = is_yellow;
    return true;
}

void Robot::encode_replacements(grSim_RobotReplacement &replacement)
{
    Robot::replace_robot(replacement);
}

void Robot::replace_robot(grSim_RobotReplacement &replacement)
{
    replacement.set_x(this->replace_robot_x);
    replacement.set_y(this->replace_robot_y);
    replacement.set_dir(this->replace_robot_dir);
    replacement.set_id(this->replace_robot_id);
    replacement.set_yellowteam(this->replace_robot_is_yellow);
}