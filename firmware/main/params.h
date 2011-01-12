#ifndef PARAMS_H
#define PARAMS_H

/**
 * \file
 *
 * \brief Provides the operational parameters block.
 */

#include "../shared/params.h"
#include <stdbool.h>

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

/**
 * \brief Saves the current in-memory parameters to flash.
 *
 * \param[in] erase \c true to erase the region before writing, or \c false if the region is already erased (e.g. after a chip erase).
 */
void params_commit(BOOL erase);

#endif

