#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../firmware/main/simulate.h"
#include "../../firmware/main/primitives/move.h"
#include "../../firmware/main/primitives/primitive.h"
#include "../../firmware/main/primitives/shoot.h"
#include "../../firmware/main/simulate.h"
#include "../../firmware/main/spline.h"

#define DELTA_T 0.0001
#define ROBOT_TICK_T 0.005
#define MAX_SIM_T 15.0f
#define HIST_TICK_T 0.03
#define NUM_PARAMS 3
#define NUM_ATTEMPTS 1

static const unsigned HIST_SIZE = MAX_SIM_T / HIST_TICK_T + 1;
const float X_BALL              = -1.0;
const float Y_BALL              = -2.0;

double metric(dr_data_t hist[HIST_SIZE], unsigned histPos)
{
    float cost;
    for (unsigned i = 0; i++; i < histPos)
    {
        cost += hist[i].x * hist[i].x + hist[i].y * hist[i].y;
    }
    cost = cost / histPos;
    return cost;
}
void prim_start(unsigned primNum, primitive_params_t *p)
{
    switch (primNum)
    {
        case 1:
            move_start(p);
            break;
        case 3:
            shoot_start(p);
            break;
        default:
            printf("this primitive not yet implemented in fwsim");
    }
}
*/

unsigned runSim(char *logFile, const int primNum, const primitive_params_t *p)
{
    sim_reset();
    //prim_start(primNum, &p);
    primitive_start(primNum, p);
    sim_log_start(logFile);
    log_record_t *notUsedLog = NULL;

    float time            = 0.0;
    float last_robot_tick = 0.0;
    float last_log_tick   = 0.0;
    while (time < MAX_SIM_T)
    {
        time += DELTA_T;
        sim_tick(DELTA_T);

        if (time - last_robot_tick >= ROBOT_TICK_T)
        {
            //prim_tick(primNum);
        	primitive_tick(notUsedLog);
            last_robot_tick = time;
        }

        if (time - last_log_tick >= LOG_TICK_T)
        {
            sim_log_tick(time);
            last_log_tick = time;
        }
    }
    sim_log_end();
    return 0;
}

int main(int argc, char **argv)
{
    // printf("\n\n\npparam0: %f", argv[4]);
    if (argc < 7)
    {
        printf("Need more arguments: logfile, prim num, prim params 0:3");
        return 10;
    }

    primitive_params_t p = {
        .params = {atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6])}};
    return runSim(argv[1], atoi(argv[2]), &p);
}
