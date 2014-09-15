#ifndef GEOM_PARTICLE_PARTICLEFILTER2D_H
#define GEOM_PARTICLE_PARTICLEFILTER2D_H

#include "util/matrix.h"
#include "util/time.h"
#include "util/param.h"
#include "ParticleFilter.h"
#include <iostream>

/**
 * \brief Implements the basic mathematics of a 2D Particle filter.
 */
class ParticleFilter2D {
	public:
		explicit ParticleFilter2D(double lengthX, double lengthY, double offsetX, double offsetY, unsigned int numPartitions);
		~ParticleFilter2D();

		void update(double timeDelta);

		void add(double valueX, double valueY, unsigned int numParticles);
		void toString();

		double getXEstimate();
		double getYEstimate();

	private:

		ParticleFilter *xFilter;
		ParticleFilter *yFilter;
};

#endif

