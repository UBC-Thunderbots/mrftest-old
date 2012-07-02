#include "util/object_store.h"

namespace {
	bool compare_typeinfos(const std::type_info *x, const std::type_info *y) {
		return x->before(*y);
	}
}

ObjectStore::Element::~Element() = default;

ObjectStore::ObjectStore() : data(&compare_typeinfos) {
}

ObjectStore::Element::Ptr &ObjectStore::operator[](const std::type_info &tid) {
	return data[&tid];
}

void ObjectStore::clear() {
	data.clear();
}

