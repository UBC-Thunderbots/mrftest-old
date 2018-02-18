#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
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

unsigned runSim(double params[NUM_PARAMS], dr_data_t hist[HIST_SIZE])
{
    sim_reset();
    primitive_params_t p;
    p.params[0] = (int16_t)(X_BALL * 1000);  // final x position
    p.params[1] = (int16_t)(Y_BALL * 1000);  // final y position
    p.params[2] = (int16_t)(0.0 * 100);      // final rotation angle
    p.params[3] = (int16_t)(0.0 * 1000);
    shoot_start(&p);
    sim_log_start();

    float time            = 0.0;
    float last_robot_tick = 0.0;
    float last_log_tick   = 0.0;
    float last_hist_tick  = 0.0;
    unsigned histPos      = 0;
    float x;
    while (time < MAX_SIM_T)
    {
        time += DELTA_T;
        sim_tick(DELTA_T);

        if (time - last_robot_tick >= ROBOT_TICK_T)
        {
            prim_tick(primNum);
            last_robot_tick = time;
        }

        if (time - last_log_tick >= LOG_TICK_T)
        {
            sim_log_tick(time);
            last_log_tick = time;
        }
        if (time - last_hist_tick >= HIST_TICK_T)
        {
            dr_get(&(hist[histPos]));
            histPos++;
            last_hist_tick = time;
        }

        // x = get_pos_x();
        // float end = abs((int) (p.params[0] / 1000));
        // if (abs(x * 1000) / 1000.0f >= end) {
        // 	break;
        // }

        float time            = 0.0;
        float last_robot_tick = 0.0;
        float last_log_tick   = 0.0;
        while (time < MAX_SIM_T)
        {
            time += DELTA_T;
            sim_tick(DELTA_T);

            if (time - last_robot_tick >= ROBOT_TICK_T)
            {
                prim_tick(primNum);
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
    //sim_log_end();
    //return 0;
//}

    int main(int argc, char **argv)
    {
        // printf("\n\n\npparam0: %f", argv[4]);
        if (argc < 7)
        {
            printf("Need more arguments: logfile, prim num, prim params 0:3");
            return 10;
        }

        primitive_params_t p = {.params = {atoi(argv[3]), atoi(argv[4]),
                                           atoi(argv[5]), atoi(argv[6])}};

        return runSim(argv[1], atoi(argv[2]), p);
    }

    primitive_params_t p = {
        .params = {atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6])}};
    //primitive_init();
    primitive_start(2, &p);
    return 1;
    //return runSim(argv[1], atoi(argv[2]), &p);
}
