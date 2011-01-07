#ifndef PARAMS_H
#define PARAMS_H

/**
 * \file
 *
 * \brief Provides the operational parameters block.
 */

#include "../shared/params.h"

/**
 * \brief The RAM shadow of the operational parameters block.
 */
extern params_t params;

/**
 * \brief Loads the parameters from flash into RAM.
 *
 * \return \c true on success, or \c false on failure (e.g. if the parameters are corrupt).
 */
BOOL params_load(void);

#endif

