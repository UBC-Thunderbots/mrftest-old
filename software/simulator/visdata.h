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
		 * \param[in] sim the Simulator to visualize.
		 */
		SimulatorVisData(const Simulator &sim);

		/**
		 * Returns the field to visualize.
		 *
		 * \return the field.
		 */
		const Visualizable::Field &field() const;

		/**
		 * Returns the ball to visualize.
		 *
		 * \return the ball.
		 */
		const Visualizable::Ball &ball() const;

		/**
		 * Returns the number of robots.
		 *
		 * \return the number of robots.
		 */
		std::size_t size() const;

		/**
		 * Fetches a robot.
		 *
		 * \param[in] index the number of the robot to fetch.
		 *
		 * \return the robot.
		 */
		const Visualizable::Robot &operator[](unsigned int index) const;

	private:
		class SimulatorVisField : public Visualizable::Field {
			public:
				SimulatorVisField();
				~SimulatorVisField();
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

		class SimulatorVisRobot : public Visualizable::Robot {
			public:
				SimulatorVisRobot(SimulatorRobot::Ptr bot);
				~SimulatorVisRobot();
				Point position() const;
				double orientation() const;
				bool visualizer_visible() const;
				Visualizable::RobotColour visualizer_colour() const;
				Glib::ustring visualizer_label() const;
				bool has_destination() const;
				Point destination() const;

			private:
				SimulatorRobot::Ptr bot;
		};

		class SimulatorVisBall : public Visualizable::Ball {
			public:
				SimulatorVisBall(SimulatorBall::Ptr ball);
				~SimulatorVisBall();
				Point position() const;
				Point velocity() const;

			private:
				SimulatorBall::Ptr ball;
		};

		const Simulator &sim;
		SimulatorVisField fld;
		SimulatorVisBall ball_;
		std::vector<SimulatorVisRobot> robots;

		void init();

		friend class Simulator;
};

#endif

