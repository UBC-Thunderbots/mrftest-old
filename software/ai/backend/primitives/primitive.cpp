#include "ai/backend/primitives/primitive.h"

using AI::BE::Primitives::Primitive;
using AI::BE::Primitives::PrimitiveDescriptor;

PrimitiveDescriptor::PrimitiveDescriptor() : prim(static_cast<Drive::Primitive>(-1)), params({ -1, -1, -1, -1 }), extra(0) { }

PrimitiveDescriptor::PrimitiveDescriptor(Drive::Primitive prim, double p0, double p1, double p2, double p3, uint8_t extra)
	: prim(prim), params({ p0, p1, p2, p3 }), extra(extra) { }

PrimitiveDescriptor::PrimitiveDescriptor(Drive::Primitive prim, const std::array<double, 4>& arr, uint8_t extra)
	: prim(prim), params(arr), extra(extra) { }

Primitive::Primitive(AI::Common::Player player, PrimitiveDescriptor desc)
	: desc_(desc), error_(nullptr), player_(player), done_(false), active_(false), moved_(false) {

	player_.impl->push_prim(this);
}

Primitive::Primitive(AI::Common::Player player, Drive::Primitive prim, double p0, double p1, double p2, double p3, uint8_t extra)
	: desc_(prim, p0, p1, p2, p3, extra), error_(nullptr), player_(player), done_(false), active_(false), moved_(false) {

	player_.impl->push_prim(this);
}

Primitive::Primitive(Primitive&& other)
	: desc_(other.desc_), error_(std::move(other.error_)), player_(other.player_), done_(other.done_), active_(other.active_), moved_(other.moved_) {

	other.player_.impl->erase_prim(&other);
	other.moved_ = true;
	player_.impl->push_prim(this);
}

Primitive::~Primitive() {
	if (!moved_ && player_) {
		player_.impl->erase_prim(this);
	}
}
