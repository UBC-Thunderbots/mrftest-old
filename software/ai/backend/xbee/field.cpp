#include "ai/backend/xbee/field.h"

using namespace AI::BE::XBee;

Field::Field() : valid_(false), length_(0), total_length_(0), width_(0), total_width_(0), goal_width_(0), centre_circle_radius_(0), defense_area_radius_(0), defense_area_stretch_(0) {
}

void Field::update(const SSL_GeometryFieldSize &packet) {
	Field old(*this);

	valid_ = true;
	length_ = packet.field_length() / 1000.0;
	total_length_ = length_ + (2.0 * packet.boundary_width() + 2.0 * packet.referee_width()) / 1000.0;
	width_ = packet.field_width() / 1000.0;
	total_width_ = width_ + (2.0 * packet.boundary_width() + 2.0 * packet.referee_width()) / 1000.0;
	goal_width_ = packet.goal_width() / 1000.0;
	centre_circle_radius_ = packet.center_circle_radius() / 1000.0;
	defense_area_radius_ = packet.defense_radius() / 1000.0;
	defense_area_stretch_ = packet.defense_stretch() / 1000.0;

	if (*this != old) {
		signal_changed.emit();
	}
}

bool Field::operator==(const Field &other) const {
	static const double (Field::* const DOUBLE_FIELDS[]) = {
		&Field::length_,
		&Field::total_length_,
		&Field::width_,
		&Field::total_width_,
		&Field::goal_width_,
		&Field::centre_circle_radius_,
		&Field::defense_area_radius_,
		&Field::defense_area_stretch_,
	};
	if (valid_ != other.valid_) {
		return false;
	}
	for (std::size_t i = 0; i < G_N_ELEMENTS(DOUBLE_FIELDS); ++i) {
		if (this->*DOUBLE_FIELDS[i] != other.*DOUBLE_FIELDS[i]) {
			return false;
		}
	}
	return true;
}

bool Field::operator!=(const Field &other) const {
	return !(*this == other);
}

