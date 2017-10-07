#ifndef UTIL_RANDOM_H
#define UTIL_RANDOM_H

namespace Random
{
/**
 * \brief Generates a word of seed data.
 */
unsigned int generate_seed();

/**
 * \brief Seeds all global random number generators in the system using the
 * current time.
 */
void seed();
}

#endif
