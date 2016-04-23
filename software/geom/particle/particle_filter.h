#ifndef GEOM_PARTICLE_PARTICLE_FILTER_H
#define GEOM_PARTICLE_PARTICLE_FILTER_H

#include <cstdlib>
#include <cmath>
#include <random>
#include <iostream>
#include <utility>
#include <vector>

#include "geom/point.h"
#include "util/param.h"

namespace AI {
	namespace BE {
		namespace Vision {
			namespace Particle {

				extern DoubleParam PARTICLE_FILTER_VAR_THRESH;
				extern DoubleParam PARTICLE_FILTER_DECAYRATE;

				class Particle {
					public:
						Particle(Point position, Point velocity,
								Point acceleration);

						Point getPosition();

						void updatePosition(double timeDelta);
						void updateVelocity(double timeDelta);

					private:
						Point position_;
						Point velocity_;
						Point acceleration_;
				};

				/**
				 * \brief Implements the basic mathematics of a Particle filter.
				 */
				class ParticleFilter final {
					public:
						explicit ParticleFilter(double length, double width);

						void update(double timeDelta);

						void add(Point ballLocation);

						void updateEstimatedPartition();

						Point getEstimate();
						Point getEstimateVariance();

					private:

						std::vector<Particle> particles_;

						Point position_[2];
						Point positionVar_[2];

						Point velocity_[2];
						Point velocityVar_[2];

						Point acceleration_;
						Point accelerationVar_;

						bool estimateValid_;

						double length_;
						double width_;
				};
			}
		}
	}
}

#endif
