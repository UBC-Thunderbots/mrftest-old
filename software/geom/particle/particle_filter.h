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

		double position_[2];
		double positionVar_[2];
		bool positionValid_[2];

		double velocity_[2];
		double velocityVar_[2];
		bool velocityValid_[2];

		unsigned int numPartitions_;
		double length_;
		double offset_;

		std::vector<int> weight_;
		std::default_random_engine random_generator_;
		std::binomial_distribution<int> random_binom_distribution_;
		std::normal_distribution<double> random_normal_distribution_;
};

#endif
