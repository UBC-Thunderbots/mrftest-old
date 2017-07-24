#ifndef GEOM_PARTICLE_PARTICLE_FILTER_H
#define GEOM_PARTICLE_PARTICLE_FILTER_H

//#include <vector>
#include "geom/point.h"
#include "util/param.h"

namespace AI {
	namespace BE {
		namespace Vision {
			namespace Particle {

				const unsigned int PARTICLE_FILTER_NUM_PARTICLES = 500;
				const double PARTICLE_GENERATION_VARIANCE = 0.05;
				const double WITHIN_FIELD_THRESHOLD = 0.05; // How close points must be to the field to be valid
				const double MAX_BALL_CONFIDENCE = 100.0;

				extern IntParam PARTICLE_FILTER_NUM_CONDENSATIONS;
				extern DoubleParam TOP_PERCENTAGE_OF_PARTICLES;
				extern DoubleParam MAX_DETECTION_WEIGHT;
				extern DoubleParam DETECTION_WEIGHT_DECAY;
				extern DoubleParam PREVIOUS_BALL_WEIGHT;
				extern DoubleParam PREDICTION_WEIGHT;
				extern DoubleParam BALL_DIST_THRESHOLD;
				extern DoubleParam BALL_CONFIDENCE_THRESHOLD;
				extern DoubleParam BALL_VALID_DIST_THRESHOLD;
				extern DoubleParam BALL_CONFIDENCE_DELTA;

				// This is used as a placeholder point for when we don't have real data
				const Point TMP_POINT = Point(-99.99, -99.99);

				/*
				 * Defines a particle used by the particle filter. A particle is just a point with an associated
				 * confidence value of how good the particle filter thinks it is.
				 */
				struct Particle {
					Particle(Point p = Point(), double c = 0.0) : position(p), confidence(c) {} // Default values for Particles
					Point position;
					double confidence;

					// Used with std::sort to sort a vector of Particles by their confidence values
					inline bool operator < (const Particle& p) const {
						return confidence < p.confidence;
					}
				};

				/**
				 * \brief Implements the basic mathematics of a Particle filter.
				 */
				class ParticleFilter final {
					public:
						explicit ParticleFilter(double length, double width);

						/**
						 * Adds a point to the Particle Filter's list of points. These points are used as basepoints
						 * for particle generation
						 */
						void add(Point ballLocation);

						/**
						 * Updates the state of the Particle Filter, generating new particles and re-evaluating them.
						 * Steps:
						 * - Generate new particles around the given basepoints
						 * - Evaluate each new particle and update its confidence value
						 * - Select new basepoints from the particles with the highest confidence
						 * - Repeat the above steps for the number of condensations. This should cause the particles and basepoints
						 *   to converge to the most confident position (the ball).
						 * - Finally, average the final basepoints to get the ball's position
						 */
						void update(Point ballPredictedPos = TMP_POINT);

						/**
						 * Returns the ball's estimated position
						 */
						Point getEstimate();

						/**
						 * Returns the variance corresponding to the ball's estimated position
						 */
						double getEstimateVariance();

					private:
						std::vector<Particle> particles; // Holds the list of particles the filter uses

						unsigned int seed; // The seed for the random number generators
						std::default_random_engine generator; // The generator used with the normal_distrubution to generate values with a gaussian distribution
						std::minstd_rand0 linearGenerator; // The generator used to generate random linear values
						std::normal_distribution<double> distribution; // Used with the generator to generate values with a gaussian distrubution

						std::vector<Point> detections; // detections by vision
						std::vector<Point> basepoints; // The points around which new particles are generated

						// These maintain some state for the ball
						Point ballPosition;
						Point ballPredictedPosition;
						double ballPositionVariance;

						// The confidence we have in the ball, from 0 to 100
						double ballConfidence;

						// The filter stores the field size
						double length_;
						double width_;

						/**
						 * Generates PARTICLE_FILTER_NUM_PARTICLES Particles in gaussian distributions around the
						 * given basepoints. If no basepoints are given, generates the Particles randomly across the
						 * whole field.
						 */
						void generateParticles(const std::vector<Point>& basepoints);

						/**
						 * Updates the confidence of each Particle in the list of particles
						 */
						void updateParticleConfidences();

						/**
						 * Increase or decreases the ball's confidence value by val, keeping it withing the range
						 * of 0 to MAX_BALL_CONFIDENCE
						 */
						void updateBallConfidence(double val);

						/**
						 * Evaluate the given particle's position and returns a score based on how good the particle is (the higher the better).
						 * Evaluation factors:
						 * - Distance from a vision detection (closer is better)
						 * - Distance from the ball's previous position (closer is better)
						 * - Distance from the ball's previous predicted location (closer is better)
						 */
						double evaluateParticle(const Point& particle);

						/**
						 * Returns the Detection Weight as a function of distance from the ball.
						 * Detections that are close to the ball's last known position should be
						 * weighted higher.
						 */
						double getDetectionWeight(const double dist);

						/**
						 * Return true if the Point is within the field plus the given threshold,
						 * otherwise return false
						 */
						bool isInField(const Point& p, double threshold = 0.0);

						/**
						 * Returns the mean of a list of points
						 */
						// TODO: put this in Evaluation
						Point getPointsMean(const std::vector<Point>& points);

						/**
						 * Returns the variance of a list of Points
						 */
						// TODO: put this in Evaluation
						double getPointsVariance(const std::vector<Point>& points);
				};
			}
		}
	}
}

#endif
