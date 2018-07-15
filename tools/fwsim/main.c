#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "log.h"
#include "primitive.h"
#include "simulate.h"

#define DELTA_T 0.0001
#define ROBOT_TICK_T 0.005
#define MAX_SIM_T 15.0f
#define HIST_TICK_T 0.03
#define LOG_TICK_T 0.03
#define NUM_PARAMS 3
#define NUM_ATTEMPTS 1

unsigned runSim(char *logFile, const int primNum, const primitive_params_t *p)
{
    sim_reset();
    // prim_start(primNum, &p);
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
            // prim_tick(primNum);
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
    if (argc < 7)
    {
        printf("Need more arguments: logfile, prim num, prim params 0:3");
        return 10;
    }

    primitive_params_t p = {
        .params = {atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6])}};
    return runSim(argv[1], atoi(argv[2]), &p);
}
