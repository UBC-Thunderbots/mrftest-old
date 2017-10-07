#ifndef GEOM_PARTICLE_PARTICLE_FILTER_H
#define GEOM_PARTICLE_PARTICLE_FILTER_H

//#include <vector>
#include "geom/point.h"
#include "util/param.h"

namespace AI
{
namespace BE
{
namespace Vision
{
namespace Particle
{
const unsigned int PARTICLE_FILTER_NUM_PARTICLES = 500;
const double PARTICLE_GENERATION_VARIANCE_LARGE  = 0.05;
const double PARTICLE_GENERATION_VARIANCE_SMALL  = 0.01;
const double WITHIN_FIELD_THRESHOLD =
    0.05;  // How close points must be to the field to be valid
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

/**
 * Defines a particle used by the particle filter.
 *
 * A particle is a point with an associated confidence value of how good the
 * particle filter thinks it is.
 */
struct Particle
{
    Particle(Point p = Point(), double c = 0.0) : position(p), confidence(c)
    {
    }  // Default values for Particles
    Point position;
    double confidence;

    /**
     * Overrides the < (less-than) operator.
     *
     * Used with std::sort to sort a vector of Particles by their confidence
     * values
     */
    inline bool operator<(const Particle& p) const
    {
        return confidence < p.confidence;
    }
};

/**
 * \brief Implements the basic mathematics of a Particle filter.
 */
class ParticleFilter final
{
   public:
    /**
     * The constructor for the particle filter.
     *
     * @param length the length of the field the particle filter is operating on
     * @param width the width of the field the particle filter is operating on
     */
    explicit ParticleFilter(double length, double width);

    /**
     * Adds a point to the Particle Filter
     *
     * Adds a particle with the given position to the Particle Filter. These
     * points are used as the
     * basepoints when generating new particles.
     *
     * @param pos the position of the particle to be added
     */
    void add(Point pos);

    /**
     * Updates the state of the Particle Filter, generating new particles and
     * re-evaluating them.
     *
     * Steps:
     * - Generate new particles around the given basepoints
     * - Evaluate each new particle and update its confidence value
     * - Select new basepoints from the particles with the highest confidence
     * - Repeat the above steps for the number of condensations. This should
     * cause the particles and basepoints
     *   to converge to the most confident position (the ball).
     * - Finally, average the final basepoints to get the ball's position
     *
     * @param ballPredictedPos an optional parameter for the ball's predicted
     * position. The Particle Filter
     *                         uses this Point to help evaluate the particles,
     * since particles that are closer
     *                         to the ball's predicted position are more likely
     * to be the ball
     */
    void update(Point ballPredictedPos = TMP_POINT);

    /**
     * Returns the ball's estimated position
     *
     * @return the ball's estimated position
     */
    Point getEstimate();

    /**
     * Returns the variance corresponding to the ball's estimated position
     *
     * @return the variance corresponding to the ball's estimated position
     */
    double getEstimateVariance();

   private:
    std::vector<Particle>
        particles;  // Holds the list of particles the filter uses

    unsigned int seed;  // The seed for the random number generators
    std::default_random_engine generator;  // The generator used with the
                                           // normal_distrubution to generate
                                           // values with a gaussian
                                           // distribution
    std::minstd_rand0
        linearGenerator;  // The generator used to generate random linear values
    std::normal_distribution<double> distributionLarge;  // Used with the
                                                         // generator to
                                                         // generate values with
                                                         // a gaussian
                                                         // distrubution
    std::normal_distribution<double> distributionSmall;  // Used with the
                                                         // generator to
                                                         // generate values with
                                                         // a gaussian
                                                         // distrubution

    std::vector<Point> detections;  // detections by vision
    std::vector<Point>
        basepoints;  // The points around which new particles are generated

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
     * Generates new particles areound the given basepoints
     *
     * Generates PARTICLE_FILTER_NUM_PARTICLES Particles in gaussian
     * distributions around the
     * given basepoints. If no basepoints are given, generates the Particles
     * randomly across the
     * whole field. The particles will try to be generated within the bounds of
     * the field
     *
     * @param smallDistribution whether or not the particles should be generated
     * with the small distribution or not. The default is to use the larger
     * distribution
     */
    void generateParticles(
        const std::vector<Point>& basepoints, bool smallDistribution);

    /**
     * Updates the confidence of each Particle in the list of particles
     *
     * Evaluates each particle in the filter's list of particles and assigns a
     * new confidence
     * value for each.
     */
    void updateParticleConfidences();

    /**
     * Increments or decrements the ball's confidence value by val, keeping the
     * value clamped
     * between 0 and MAX_BALL_CONFIDENCE
     *
     * @param val the amount to update the confidence by
     */
    void updateBallConfidence(double val);

    /**
     * Evaluates the given Point and returns a score based on how likely is it
     * that the ball is at that location
     *
     * Evaluation factors:
     * - Distance from a vision detection (closer is better)
     * - Distance from the ball's previous position (closer is better)
     * - Distance from the ball's previous predicted location (closer is better)
     *
     * @param pos the position of the particle to be evaluated
     */
    double evaluateParticle(const Point& pos);

    /**
     * Returns the Detection Weight as a function of distance from the ball's
     * previous location.
     *
     * @param dist the distance from the ball's last known location
     */
    double getDetectionWeight(const double dist);

    /**
     * Return true if the Point is within the field plus the given threshold,
     * otherwise return false
     *
     * @param p the point
     * @param threshold how far outside the field the point can be before the
     * function returns false
     * @return true if p is within the field plus the tolerance, and false
     * otherwise
     */
    bool isInField(const Point& p, double threshold = 0.0);

/**
 * Returns the mean of a list of points
 *
 * @param points the vector of points
 * @return the mean point of points
 */
#warning put this in evaluation
    Point getPointsMean(const std::vector<Point>& points);

/**
 * Returns the variance of a list of Points
 *
 * @param points the vector of points
 * @return the variance of the list of points
 */
#warning put this in evaluation
    double getPointsVariance(const std::vector<Point>& points);
};
}
}
}
}

#endif
