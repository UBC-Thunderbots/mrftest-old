#include "util/locale.h"
#include <clocale>

namespace {
	bool initialized = false;
}

void initialize_locale() {
	if (!initialized) {
		std::setlocale(LC_ALL, "");
		initialized = true;
	}
}

