#include "util/random.h"
#include <cstdlib>
#include <random>

unsigned int Random::generate_seed()
{
    static std::random_device rdev;
    return rdev();
}

void Random::seed()
{
    srand48(generate_seed());
    std::srand(generate_seed());
}
