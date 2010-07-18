#ifndef SIM_VISDATA_H
#define SIM_VISDATA_H

#include "uicomponents/visualizer.h"
#include <vector>

class Simulator;

/**
 * An implementation of Visualizable that displays data from the simulator.
 */
class SimulatorVisData : public Visualizable {
	public:
		/**
		 * Constructs a new SimulatorVisData.
		 *
		 * \param sim the Simulator to visualize
		 */
		SimulatorVisData(const Simulator &sim);

		const Visualizable::Field &field() const;
		Visualizable::Ball::ptr ball() const;
		std::size_t size() const;
		Visualizable::Robot::ptr operator[](unsigned int index) const;

	private:
		class SimulatorVisField : public Visualizable::Field {
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

		const Simulator &sim;
		SimulatorVisField fld;
		std::vector<Visualizable::Robot::ptr> robots;

		void init();

		friend class Simulator;
};

#endif

