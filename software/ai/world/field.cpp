#include "ai/world/field.h"

Field::Field() : valid_(false), length_(0), total_length_(0), width_(0), total_width_(0), goal_width_(0), centre_circle_radius_(0), defense_area_radius_(0), defense_area_stretch_(0) {
}

void Field::update(const SSL_GeometryFieldSize &packet) {
	valid_ = true;
	length_ = packet.field_length() / 1000.0;
	total_length_ = length_ + (2.0 * packet.boundary_width() + 2.0 * packet.referee_width()) / 1000.0;
	width_ = packet.field_width() / 1000.0;
	total_width_ = width_ + (2.0 * packet.boundary_width() + 2.0 * packet.referee_width()) / 1000.0;
	goal_width_ = packet.goal_width() / 1000.0;
	centre_circle_radius_ = packet.center_circle_radius() / 1000.0;
	defense_area_radius_ = packet.defense_radius() / 1000.0;
	defense_area_stretch_ = packet.defense_stretch() / 1000.0;
	signal_changed.emit();
}

