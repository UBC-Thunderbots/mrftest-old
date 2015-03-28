#ifndef GEOM_PARTICLE_PARTICLE_FILTER_H
#define GEOM_PARTICLE_PARTICLE_FILTER_H

#include <random>
#include <utility>
#include <vector>

/**
 * \brief Implements the basic mathematics of a Particle filter.
 */
class ParticleFilter final {
	public:
		explicit ParticleFilter(double length, double partitionSize);

		void update(double timeDelta);

		void add(double value, unsigned int numParticles);

		double getEstimate();
		double getLength();
		double getOffset();
		int getNumParticles();

	private:
		void updateEstimatedPartition();
		void clearWeights(unsigned int startIndex, unsigned int endIndex);

		double velocity_;
		double accel_;
		double estimate_;
		double prevEstimate_;
		unsigned int numPartitions_;
		double length_;
		double offset_;
		bool estimateValid_;
		bool prevEstimateValid_;
		bool velocityValid_;
		std::vector<int> weight_;
		std::default_random_engine random_generator_;
		std::binomial_distribution<int> random_binom_distribution_;
		std::normal_distribution<double> random_normal_distribution_;
};

#endif
