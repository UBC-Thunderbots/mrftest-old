#include "geom/particle/particle_filter.h"
#include "geom/angle.h"
#include "util/param.h"
#include "util/random.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <random>
#include <vector>

namespace {
	DoubleParam PARTICLE_FILTER_STDDEV(u8"Particle Filter Standard Deviation", u8"AI/Backend/Vision/Particle", 0.1, 0.0, 2.0);
	DoubleParam PARTICLE_FILTER_DECAYRATE(u8"Particle Filter Decay Rate", u8"AI/Backend/Vision/Particle", 0.3, 0, 1.0);
}

ParticleFilter::ParticleFilter(double length, double partitionSize) :
		position_{},
		positionVar_{},
		positionValid_{},
		velocity_{},
		velocityVar_{},
		velocityValid_{},
		numPartitions_(static_cast<unsigned int>(ceil(length/partitionSize))), // number of time to partition the length. particles are assigned to one of these partitions
		length_(length),
		offset_(-(length*0.5)), // starting offset from 0 (ie length goes from -2 to 2 -> offset = -2)
		weight_(numPartitions_, 0),
		random_generator_(Random::generate_seed()),
		random_binom_distribution_(),
		random_normal_distribution_() 
{
	add(0, 100);
}

void ParticleFilter::update(double timeDelta) {
	// DECAY
	for (unsigned int i = 0; i < numPartitions_; i++) {
		// binomial distribution w/ p = decayrate
		std::binomial_distribution<int>::param_type params(weight_[i], PARTICLE_FILTER_DECAYRATE);
		weight_[i] = random_binom_distribution_(random_generator_, params);
	}

	// MOVE REMAINING PARTICLES
	updateEstimatedPartition();

	double estimatedVelocity = 0; // use previous acceleration and current velocity to find estimated velocity
	double estimatedVelocityVar = 0;

	if (positionValid_[0] && positionValid_[1])
	{
		double curVelocity = (position_[0] - position_[1])/timeDelta; // find out how "fast" we are going
		double curVelocityVar = positionVar_[0] + positionVar_[1];

		estimatedVelocity = curVelocity;
		estimatedVelocityVar = curVelocityVar;

		if (velocityValid_[0] && velocityValid_[1])
		{
			double curAcceleration = (velocity_[0] - velocity_[1])/timeDelta;
			double curAccelerationVar = velocityVar_[0] + velocityVar_[1];

			estimatedVelocity = curVelocity + curAcceleration*timeDelta;
			estimatedVelocityVar = curAccelerationVar + curAccelerationVar;
		}
	}

	double estimatePosition = position_[0] + timeDelta*estimatedVelocity;
	double estimatePositionVar = positionVar_[0] + estimatedVelocityVar;

	position_[1] = position_[0];
	positionVar_[1] = positionVar_[0];
	positionValid_[1] = positionValid_[0];

	position_[0] = estimatePosition;
	positionVar_[0] = estimatePositionVar;
	positionValid_[0] = true;

	velocity_[1] = velocity_[0];
	velocityVar_[1] = velocityVar_[0];
	velocityValid_[1] = velocityValid_[0];

	velocity_[0] = estimatedVelocity;
	velocityVar_[0] = estimatedVelocityVar;
	velocityValid_[0] = true;

	// SUM UP EVERYTHING AND REDUCE IF TOO MANY
	int sum = 0;

	for (unsigned int i = 0; i < numPartitions_; i++) {
		sum += weight_[i];
	}

	if (sum > 1000) {
		for (unsigned int i = 0; i < numPartitions_; i++) {
			weight_[i] = weight_[i] * 1000 / sum;
		}
	}
}

void ParticleFilter::add(double input, unsigned int numParticles) {
	std::normal_distribution<double>::param_type normal_params(input - offset_, PARTICLE_FILTER_STDDEV);
	double value;
	double partitionSize = length_/numPartitions_;
	int partition;
	int count = 0;

	// ADD PARTICLES

	for (unsigned int i = 0; i < numParticles; i++) {
		// generate normal distribution of particles
		value = random_normal_distribution_(random_generator_, normal_params); // how far into the field we are
		if ((value >= 0) && (value < length_)) {
			// calculate which partition the particle falls in
			partition = static_cast<int>(value/partitionSize);

			assert(partition >= 0);
			assert(static_cast<unsigned int>(partition) < numPartitions_);
			weight_[partition] += 1;
			count++;
		}
	}

	// ESTIMATE IS NO LONGER VALID
	positionValid_[0] = false;
}

double ParticleFilter::getEstimate() {
	updateEstimatedPartition();
	//return estimate_;
	return offset_ + (position_[0] + 0.5)*(length_/numPartitions_);
}

double ParticleFilter::getLength() {
	return length_;
}
double ParticleFilter::getOffset() {
	return offset_;
}

void ParticleFilter::updateEstimatedPartition() {
	std::vector<std::pair<int, int> > weightIndexPairs;

	if (!positionValid_[0]) {
		unsigned int sum = 0;

		// FIND HYPOTHESIS WITH MOST WEIGHT
		for (unsigned int i = 0; i < numPartitions_; i++) {
			sum += weight_[i];
		}

		position_[0] = 0;

		for (unsigned int i = 0; i < numPartitions_; i++) {
			position_[0] += (static_cast<double>(weight_[i])/sum)*i;
		}

		positionValid_[0] = true;
	}
}

void ParticleFilter::clearWeights(unsigned int startIndex, unsigned int endIndex)
{
	assert(endIndex <= numPartitions_);
	for (unsigned int i = startIndex; i < endIndex; i++) {
		weight_[i] = 0;
	}
}
