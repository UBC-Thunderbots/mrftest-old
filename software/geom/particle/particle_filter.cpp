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
		velocity_(0),
		accel_(0),
		estimate_(0),
		prevEstimate_(0),
		numPartitions_(static_cast<unsigned int>(ceil(length/partitionSize))), // number of time to partition the length. particles are assigned to one of these partitions
		length_(length),
		offset_(-(length*0.5)), // starting offset from 0 (ie length goes from -2 to 2 -> offset = -2)
		estimateValid_(false),
		prevEstimateValid_(false),
		velocityValid_(false),
		weight_(numPartitions_, 0),
		random_generator_(Random::generate_seed()),
		random_binom_distribution_(),
		random_normal_distribution_() {
	add(0, 100);
}

void ParticleFilter::update(double timeDelta) {
	double partitionSize = length_/numPartitions_;

	// DECAY
	for (unsigned int i = 0; i < numPartitions_; i++) {
		// binomial distribution w/ p = decayrate
		std::binomial_distribution<int>::param_type params(weight_[i], PARTICLE_FILTER_DECAYRATE);
		weight_[i] = random_binom_distribution_(random_generator_, params);
	}

	// MOVE REMAINING PARTICLES
	updateEstimatedPartition();

	double curVelocity = (estimate_ - prevEstimate_)/timeDelta; // find out how "fast" we are going
	double acceleration = (curVelocity - velocity_)/timeDelta;
	double estimatedVelocity = curVelocity - accel_*timeDelta; // use previous acceleration and current velocity to find estimated velocity

	if (estimatedVelocity < 0) {
		// blank out the section in the back where we don't think the ball was
		int limit = abs(static_cast<int>(estimatedVelocity*timeDelta/partitionSize));
		if (static_cast<int>(numPartitions_) + 1 < limit) {
			//std::cout << "LARGE NEGATIVE VELOCITY - CLEARING FIELD" << std::endl;
			//assert(numPartitions_ + 1 >= estimatedVelocity*timeDelta);

			clearWeights(0, numPartitions_);
		} else {
			// shift everything back
			for (int i = abs(static_cast<int>(estimatedVelocity*timeDelta/partitionSize)); i < static_cast<int>(numPartitions_); i++)
			{
				assert(i >= 0);
				assert(i < weight_.size());
				assert(i - limit >= 0);
				assert(i - limit < weight_.size());
				weight_[i-limit] = weight_[i];
			}
		}

		clearWeights(numPartitions_ - static_cast<int>(estimatedVelocity*timeDelta/partitionSize) + 1, numPartitions_);
	} else if (estimatedVelocity > 0) {
		// blank out everything in the front where we don't think the ball was
		if (estimatedVelocity*timeDelta/partitionSize > numPartitions_) {
			//std::cout << "LARGE POSITIVE VELOCITY - CLEARING FIELD" << std::endl;
			//assert(estimatedVelocity*timeDelta <= numPartitions_);

			clearWeights(0, numPartitions_);
		} else {
			// shift everything forward
			for (int i = numPartitions_ - static_cast<int>(estimatedVelocity*timeDelta/partitionSize) - 1; i >= 0; i--) {
				weight_[i+static_cast<unsigned int>(estimatedVelocity*timeDelta/partitionSize)] = weight_[i];
			}

			clearWeights(0, static_cast<int>(estimatedVelocity*timeDelta/partitionSize));
		}
	}

	clearWeights(0, numPartitions_);

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

	// update values
	velocity_ = curVelocity;
	accel_ = acceleration;
	prevEstimate_ = estimate_;
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
	estimateValid_ = false;
}

double ParticleFilter::getEstimate() {
	updateEstimatedPartition();
	//return estimate_;
	return offset_ + (estimate_ + 0.5)*(length_/numPartitions_);
}

double ParticleFilter::getLength() {
	return length_;
}
double ParticleFilter::getOffset() {
	return offset_;
}

void ParticleFilter::updateEstimatedPartition() {
	std::vector<std::pair<int, int> > weightIndexPairs;

	if (!estimateValid_) {
		unsigned int sum = 0;

		// FIND HYPOTHESIS WITH MOST WEIGHT
		for (unsigned int i = 0; i < numPartitions_; i++) {
			sum += weight_[i];
		}

		estimate_ = 0;

		for (unsigned int i = 0; i < numPartitions_; i++) {
			estimate_ += (static_cast<double>(weight_[i])/sum)*i;
		}

		estimateValid_ = true;
	}
}

void ParticleFilter::clearWeights(unsigned int startIndex, unsigned int endIndex)
{
	assert(endIndex <= numPartitions_);
	for (unsigned int i = startIndex; i < endIndex; i++) {
		weight_[i] = 0;
	}
}
