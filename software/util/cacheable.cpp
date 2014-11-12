#include "util/cacheable.h"
#include <algorithm>
#include <vector>

namespace {
	std::vector<CacheableBase *> &vec() {
		static std::vector<CacheableBase *> v;
		return v;
	}
}

CacheableBase::CacheableBase() {
	vec().push_back(this);
}

CacheableBase::~CacheableBase() {
	vec().erase(std::remove(vec().begin(), vec().end(), this), vec().end());
}

void CacheableBase::flush_all() {
	std::for_each(vec().begin(), vec().end(), [](CacheableBase *c) { c->flush(); });
}

