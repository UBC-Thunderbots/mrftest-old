#include "util/memory.h"

void ByRef::destroy() {
	delete this;
}

unsigned int ByRef::refs() const {
	return refs_;
}

ByRef::ByRef() : refs_(1) {
}

ByRef::~ByRef() {
}

