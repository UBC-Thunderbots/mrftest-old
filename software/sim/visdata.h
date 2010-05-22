#ifndef SIM_VISDATA_H
#define SIM_VISDATA_H

#include "uicomponents/visualizer.h"
#include <vector>

class simulator;

/**
 * An implementation of visualizable that displays data from the simulator.
 */
class simulator_visdata : public visualizable {
	public:
		/**
		 * Constructs a new simulator_visdata.
		 *
		 * \param sim the simulator to visualize
		 */
		simulator_visdata(const simulator &sim);

		const class visualizable::field &field() const;
		visualizable::ball::ptr ball() const;
		std::size_t size() const;
		visualizable::robot::ptr operator[](unsigned int index) const;

	private:
		class sim_field : public visualizable::field {
			public:
				bool valid() const;
				double length() const;
				double total_length() const;
				double width() const;
				double total_width() const;
				double goal_width() const;
				double centre_circle_radius() const;
				double defense_area_radius() const;
				double defense_area_stretch() const;
		};

		const simulator &sim;
		sim_field fld;
		std::vector<visualizable::robot::ptr> robots;

		void init();

		friend class simulator;
};

#endif

