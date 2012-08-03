#include "util/random.h"
#include <chrono>
#include <cstdlib>

void Random::seed() {
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	std::chrono::system_clock::duration diff = now.time_since_epoch();
	long seconds = static_cast<long>(std::chrono::duration_cast<std::chrono::seconds>(diff).count());
	srand48(seconds);
	std::srand(static_cast<unsigned int>(static_cast<unsigned long>(seconds)));
}

