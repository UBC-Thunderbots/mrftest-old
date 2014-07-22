#include "geom/particle/ParticleFilter.h"
#include "geom/angle.h"
#include <cassert>
#include <random>
#include <cmath>
#include <vector>
#include <algorithm>
#include <chrono>

DoubleParam PARTICLE_FILTER_STDDEV(u8"Particle Filter Standard Deviation", u8"Backend", 0.1, 0.0, 2.0);
DoubleParam PARTICLE_FILTER_DECAYRATE(u8"Particle Filter Decay Rate", u8"Backend", 0.3, 0, 1.0);

ParticleFilter::ParticleFilter(double length, double offset, unsigned int numPartitions)
{
	weight_ = new double[numPartitions]; // weight of the partitions

	for (unsigned int i = 0; i < numPartitions; i++)
	{
		weight_[i] = 0;
	}

	velocity_ = 0;
	accel_ = 0;
	estimate_ = 0;
	prevEstimate_ = 0;
	numPartitions_ = numPartitions; // number of time to partition the length. particles are assigned to one of these partitions
	estimateValid_ = false;
	length_ = length;
	offset_ = offset; // starting offset from 0 (ie length goes from -2 to 2 -> offset = -2)

	//std::cout << "Length: " << length_ << "; Offset: " << offset_ << "; numPartitions: " << numPartitions_ << std::endl;

	add(0, 100);
}

ParticleFilter::~ParticleFilter()
{
	delete[] weight_;
}

void ParticleFilter::update(double timeDelta)
{
	// uniform random generator - use timestamp as seed
	unsigned int seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine generator (seed);
	double partitionSize = (double)length_/numPartitions_;

	// DECAY
	for (unsigned int i = 0; i < numPartitions_; i++)
	{
		// binomial distribution w/ p = decayrate
		std::binomial_distribution<int> dist(weight_[i], PARTICLE_FILTER_DECAYRATE);
		weight_[i] = dist(generator);
	}

	// MOVE REMAINING PARTICLES
	updateEstimatedPartition();

	double curVelocity = (estimate_ - prevEstimate_)/timeDelta; // find out how "fast" we are going
	double acceleration = (curVelocity - velocity_)/timeDelta;
	double estimatedVelocity = curVelocity - accel_*timeDelta; // use previous acceleration and current velocity to find estimated velocity

	if (estimatedVelocity < 0)
	{
		// blank out the section in the back where we don't think the ball was
		if (numPartitions_ + 1 < abs(estimatedVelocity*timeDelta/partitionSize))
		{
			//std::cout << "LARGE NEGATIVE VELOCITY - CLEARING FIELD" << std::endl;
			//assert(numPartitions_ + 1 >= estimatedVelocity*timeDelta);

			clearWeights(0, numPartitions_);
		}
		else
		{
			// shift everything back
			for (int i = estimatedVelocity*timeDelta/partitionSize; i < numPartitions_; i++)
			{
				weight_[i-(int)(abs(estimatedVelocity*timeDelta/partitionSize))] = weight_[i];
			}
		}

		clearWeights(numPartitions_ - (estimatedVelocity*timeDelta/partitionSize) + 1, numPartitions_);
	}
	else if (estimatedVelocity > 0)
	{
		// blank out everything in the front where we don't think the ball was
		if (estimatedVelocity*timeDelta/partitionSize > numPartitions_)
		{
			//std::cout << "LARGE POSITIVE VELOCITY - CLEARING FIELD" << std::endl;
			//assert(estimatedVelocity*timeDelta <= numPartitions_);

			clearWeights(0, numPartitions_);
		}
		else
		{
			// shift everything forward
			for (int i = numPartitions_ - estimatedVelocity*timeDelta/partitionSize - 1; i >= 0; i--)
			{
				weight_[i+(int)(estimatedVelocity*timeDelta/partitionSize)] = weight_[i];
			}

			clearWeights(0, estimatedVelocity*timeDelta/partitionSize);
		}
	}

	clearWeights(0, numPartitions_);

	// SUM UP EVERYTHING AND REDUCE IF TOO MANY
	int sum = 0;

	for (int i = 0; i < numPartitions_; i++)
	{
		sum += weight_[i];
	}

	if (sum > 1000)
	{
		for (int i = 0; i < numPartitions_; i++)
		{
			weight_[i] *= (double)(1000/sum);
		}
	}

	// update values
	velocity_ = curVelocity;
	accel_ = acceleration;
	prevEstimate_ = estimate_;
}

void ParticleFilter::toString()
{
	int sum = 0;

	for (unsigned int i = 0; i < numPartitions_; i++)
	{
		std::cout << weight_[i] << "\n";
		sum += weight_[i];
	}

	std::cout << " - " << sum << " particles " << std::endl;
}

void ParticleFilter::add(double input, unsigned int numParticles)
{
	// uniform random generator - use timestamp as seed
	unsigned int seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine generator (seed);

	std::normal_distribution<double> normal(input - offset_, PARTICLE_FILTER_STDDEV);
	double value;
	double partitionSize = (double)length_/numPartitions_;
	int partition;
	int count = 0;

	// ADD PARTICLES

	for (unsigned int i = 0; i < numParticles; i++)
	{
		// generate normal distribution of particles
		value = normal(generator); // how far into the field we are
		if ((value >= 0) && (value < length_))
		{
			// calculate which partition the particle falls in
			partition = value/partitionSize;

			assert(partition >= 0);
			assert(partition < (int)numPartitions_);
			weight_[partition] += 1;
			count++;
		}
	}

	// ESTIMATE IS NO LONGER VALID
	estimateValid_ = false;
}

double ParticleFilter::getEstimate()
{
	updateEstimatedPartition();
	//return estimate_;
	return offset_ + (estimate_ + 0.5)*(length_/numPartitions_);
}

double ParticleFilter::getLength()
{
	return length_;
}
double ParticleFilter::getOffset()
{
	return offset_;
}

void ParticleFilter::updateEstimatedPartition()
{
	std::vector<std::pair<int, int> > weightIndexPairs;

	if (!estimateValid_)
	{
		unsigned int sum = 0;

		// FIND HYPOTHESIS WITH MOST WEIGHT
		for (unsigned int i = 0; i < numPartitions_; i++)
		{
			sum += weight_[i];
		}

		estimate_ = 0;

		for (unsigned int i = 0; i < numPartitions_; i++)
		{
			estimate_ += (weight_[i]/sum)*i;
		}

		estimateValid_ = true;
	}
}

void ParticleFilter::clearWeights(unsigned int startIndex, unsigned int endIndex)
{
	if (endIndex > numPartitions_)
	{
		std::cout << "Trying to clear past bounds!" << std::endl;
		std::cout << "Start Index: " << startIndex << " End Index: " << endIndex << std::endl;
		assert(0);
	}

	for (unsigned int i = startIndex; i < endIndex; i++)
	{
		weight_[i] = 0;
	}
}

bool pairCompare(const std::pair<int, int>& a, const std::pair<int, int>& b) {
  return a.second < b.second;
}
