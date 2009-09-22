#ifndef SIMULATOR_WINDOW_H
#define SIMULATOR_WINDOW_H

//
// The user interface for the simulator.
//
class simulator_window_impl;
class simulator_window {
	public:
		simulator_window();
		~simulator_window();

	private:
		simulator_window_impl *impl;
};

#endif

