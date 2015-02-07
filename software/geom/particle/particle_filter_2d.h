#ifndef GEOM_PARTICLE_PARTICLE_FILTER_2D_H
#define GEOM_PARTICLE_PARTICLE_FILTER_2D_H

#include "util/matrix.h"
#include "util/param.h"
#include "geom/point.h"
#include "particle_filter.h"
#include <iostream>

/**
 * \brief Implements the basic mathematics of a 2D Particle filter.
 */
class ParticleFilter2D final {
	public:
		explicit ParticleFilter2D(Point length, double partitionSize);
		~ParticleFilter2D();

		void update(double timeDelta);

		void add(Point p, unsigned int numParticles);
		void toString();

		Point getEstimate();

	private:

		ParticleFilter *xFilter;
		ParticleFilter *yFilter;
};

#endif

