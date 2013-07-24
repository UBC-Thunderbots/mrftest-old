#include "util/object_store.h"

ObjectStore::Element::~Element() = default;

ObjectStore::Element::Ptr &ObjectStore::operator[](const std::type_info &tid) {
	return data[std::type_index(tid)];
}

void ObjectStore::clear() {
	data.clear();
}

