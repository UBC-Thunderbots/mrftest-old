#ifndef QUADRATIC_H
#define QUADRATIC_H

#include "physbot.h"
#include "../dr.h"


/**
 * A primitive should call this function to optimize acclerations
 * in the major, minor, and rotational directions. This function
 * sets up the matrices required for the CVXGEN solver by using
 * the helper functions defined in this file then calls the solve()
 * method in CVXGEN to run the optimization.
 * 
 * @param pb a PhysBot that was set up by that setup_bot function 
 * in physbot.c
 * @param state a dr_data_t that contains the bot's current angle, 
 * @param a_req a 3 length array with requested accelerations for major, minor,
 * and rotational accelerations
 * 
 */
void quad_optimize(PhysBot pb, dr_data_t state, float a_req[3]);

#endif