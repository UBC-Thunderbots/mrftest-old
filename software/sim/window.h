#ifndef SIMULATOR_WINDOW_H
#define SIMULATOR_WINDOW_H

#include "sim/simulator.h"

//
// The user interface for the simulator.
//
class simulator_window_impl;
class simulator_window {
	public:
		simulator_window(simulator &sim, clocksource &simclk, clocksource &uiclk);
		~simulator_window();
		void show_fps(unsigned int fps);

	private:
		simulator_window_impl *impl;
};

#endif

