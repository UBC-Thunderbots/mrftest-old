#include "ai/backend/field.h"

using AI::BE::Field;

Field::Field()
    : valid_(false),
      length_(0),
      total_length_(0),
      width_(0),
      total_width_(0),
      goal_width_(0),
      centre_circle_radius_(0),
      defense_area_radius_(0),
      defense_area_stretch_(0)
{
}

void Field::update(
    double length, double total_length, double width, double total_width,
    double goal_width, double centre_circle_radius, double defense_area_radius,
    double defense_area_stretch)
{
    Field old(*this);

    valid_                = true;
    length_               = length;
    total_length_         = total_length;
    width_                = width;
    total_width_          = total_width;
    goal_width_           = goal_width;
    centre_circle_radius_ = centre_circle_radius;
    defense_area_radius_  = defense_area_radius;
    defense_area_stretch_ = defense_area_stretch;

    if (*this != old)
    {
        signal_changed.emit();
    }
}

bool Field::operator==(const Field &other) const
{
    static const double(Field::*const DOUBLE_FIELDS[]) = {
        &Field::length_,
        &Field::total_length_,
        &Field::width_,
        &Field::total_width_,
        &Field::goal_width_,
        &Field::centre_circle_radius_,
        &Field::defense_area_radius_,
        &Field::defense_area_stretch_,
    };
    if (valid_ != other.valid_)
    {
        return false;
    }
    for (std::size_t i = 0; i < G_N_ELEMENTS(DOUBLE_FIELDS); ++i)
    {
        if (this->*DOUBLE_FIELDS[i] != other.*DOUBLE_FIELDS[i])
        {
            return false;
        }
    }
    return true;
}

bool Field::operator!=(const Field &other) const
{
    return !(*this == other);
}
