#include "particle_filter.h"
#include <math.h>
#include <random>
#include <chrono>
#include <algorithm>

using namespace AI::BE::Vision::Particle;

/* Notes on the weights and evaluation:
 * - PREDICTION_WEIGHT should (probably) always be higher than PREVIOUS_BALL_WEIGHT because
 *   in the case where there is no vision detection, we want to take the prediction position not
 *   the old ball position so the ball keeps moving (we don't want the ball to stop if we lose vision for a second).
 *
 * - If there are vision detections, one of them should be chosen (this should be 99% of all the cases, unless
 *   they are all really far away and therefore likely to be just noise) and the ball position and prediction
 *   weights should just be used to help weight the particles closer to the detection closest to the ball
 *   (which should be the detection for the real ball)
 */

IntParam AI::BE::Vision::Particle::PARTICLE_FILTER_NUM_CONDENSATIONS(
		u8"Particle Filter number of condensations",
		u8"AI/Backend/Vision/Particle", 5, 1, 50);
// TOP_PERCENTAGE_OF_PARTICLES SHOULD ***NEVER*** be > 1.0, OTHERWISE THE FILTER
// WILL TRY SELECT MORE BASEPOINTS THEN THERE ARE PARTICLES
DoubleParam AI::BE::Vision::Particle::TOP_PERCENTAGE_OF_PARTICLES(
		u8"The top fraction of particles that are used as basepoints for the next sample",
		u8"AI/Backend/Vision/Particle", 0.1, 0.0, 1.0);
DoubleParam AI::BE::Vision::Particle::MAX_DETECTION_WEIGHT(
		u8"The weight of vision detections",
		u8"AI/Backend/Vision/Particle", 100.0, 0.0, 1000.0);
DoubleParam AI::BE::Vision::Particle::DETECTION_WEIGHT_DECAY(
		u8"The decay rate (per meter) of the detection weight",
		u8"AI/Backend/Vision/Particle", 200.0, 0.0, 10000.0);
DoubleParam AI::BE::Vision::Particle::PREVIOUS_BALL_WEIGHT(
		u8"The weight of the previous ball's position",
		u8"AI/Backend/Vision/Particle", 1.0, 0.0, 1000.0);
DoubleParam AI::BE::Vision::Particle::PREDICTION_WEIGHT(
		u8"The weight of the previous ball's predicted position",
		u8"AI/Backend/Vision/Particle", 15.0, 0.0, 1000.0);
DoubleParam AI::BE::Vision::Particle::BALL_DIST_THRESHOLD(
		u8"How close a particle must be to the ball to get the extra PREVIOUS_BALL_WEIGHT",
		u8"AI/Backend/Vision/Particle", 0.5, 0.0, 100.0);
DoubleParam AI::BE::Vision::Particle::BALL_CONFIDENCE_THRESHOLD(
		u8"The confidence threshold for being confident or not of the ball's position",
		u8"AI/Backend/Vision/Particle", 60.0, 0.0, 100.0);
DoubleParam AI::BE::Vision::Particle::BALL_VALID_DIST_THRESHOLD(
		u8"How much the detected ball can move per tick without losing confidence",
		u8"AI/Backend/Vision/Particle", 0.1, 0.0, 100.0);
DoubleParam AI::BE::Vision::Particle::BALL_CONFIDENCE_DELTA(
		u8"How much the ball's confidence changes at a time",
		u8"AI/Backend/Vision/Particle", 5.0, 0.0, 100.0);


ParticleFilter::ParticleFilter(double length, double width) {
	length_ = length;
	width_ = width;

	// initialize vector with PARTICLE_FILTER_NUM_PARTICLES Particle() objects
	particles = std::vector<Particle>(PARTICLE_FILTER_NUM_PARTICLES, Particle());

	seed = static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count());

	// These will be used to generate Points with a gaussian distribution
	generator = std::default_random_engine(seed);
	distribution = std::normal_distribution<double>(0.0, PARTICLE_GENERATION_VARIANCE);

	// This will be used to generate Points spread across the whole field
	linearGenerator = std::minstd_rand0(seed);

	// These start with the placeholder so we can tell we haven't detected a valid ball yet
	ballPosition = TMP_POINT;
	ballPredictedPosition = TMP_POINT;
	ballPositionVariance = 0.0;

	ballConfidence = 0.0;

	add(Point(0,0));
}

void ParticleFilter::add(const Point ballLocation) {
	if (!std::isnan(ballLocation.x + ballLocation.y) && isInField(ballLocation, WITHIN_FIELD_THRESHOLD)) {
		detections.push_back(ballLocation);
	}
}

void ParticleFilter::update(Point ballPredictedPos) {
	ballPredictedPosition = ballPredictedPos;
	basepoints = detections;

	// Only add the PredictedPosition (not the ball's last position) as a basepoint because the ball is more likely
	// to be there than the ball's old location. If the ball is not moving, the prediction
	// should be the ball's position anyway.
	if(ballPosition != TMP_POINT && ballPredictedPosition != TMP_POINT) {
		basepoints.push_back(ballPredictedPos);
	}

	// Remove basepoints that are out of the field. It's either noise, or we don't care if the ball is there.
	for(auto it = basepoints.begin(); it != basepoints.end();) {
		if(!isInField(*it, WITHIN_FIELD_THRESHOLD)) {
			it = basepoints.erase(it);
		}else {
			it++;
		}
	}

	// This is the main particle filter loop.
	// - Generate particles around the given basepoints
	// - Update the confidences of these new particles
	// - Evaluate each particle, and keep the top percentage of particles to use as
	//   basepoints for the next iteration. These particles should converge to the ball's location.
	for(int i = 0; i < PARTICLE_FILTER_NUM_CONDENSATIONS; i++) {
		generateParticles(basepoints);
		updateParticleConfidences();

		unsigned int numParticlesToKeep = static_cast<unsigned int>(ceil(TOP_PERCENTAGE_OF_PARTICLES * PARTICLE_FILTER_NUM_PARTICLES));

		// make sure we never try keep more particles than we have
		if(numParticlesToKeep > particles.size()) {
			numParticlesToKeep = static_cast<unsigned int>(particles.size());
		}
		// sort the list of Particles by their confidences from least to most confidence
		std::sort(particles.begin(), particles.end());

		// Replace the basepoints with the positions of the most confident particles
		basepoints.clear();
		for(auto it = particles.end() - numParticlesToKeep; it != particles.end(); it++) {
			basepoints.push_back(it->position);
		}
	}

	// Average the final basepoints to get the ball's location. This makes the ball's
	// movement slightly smoother than just taking the single most confident point
	Point newBallPosition = getPointsMean(basepoints);
	double newBallPositionVariance = getPointsVariance(basepoints);

	// If there are no balls detected, lose a bit of confidence in the ball. If we can't see the ball it could
	// be covered, be being moved, or be off the field, and we don't know.
	if(detections.empty()) {
		updateBallConfidence(-5.0);
	}

	// If something indicates we might have noise / be filtering incorrectly, lose confidence.
	// If this happens, don't use the newly calculated position as it might be bad.
	if((newBallPosition - ballPosition).len() > BALL_VALID_DIST_THRESHOLD ||
			newBallPositionVariance > PARTICLE_GENERATION_VARIANCE * 1.5) {
		updateBallConfidence(-BALL_CONFIDENCE_DELTA);
		// TODO: possibly use the ball's predicted position here
	}else {
		ballPosition = newBallPosition;
		ballPositionVariance = newBallPositionVariance;
		updateBallConfidence(BALL_CONFIDENCE_DELTA);
	}

	if(ballConfidence < BALL_CONFIDENCE_THRESHOLD) {
		if(detections.empty()) {
			// Don't update the ball
			// We could also set the position to TMP_POINT, but in some cases this might result
			// in ai thinking the ball has a really high velocity for 1 tick. Probably better to do nothing.
		}else {
			ballPosition = newBallPosition;
			ballPositionVariance = newBallPositionVariance;
			ballConfidence = BALL_CONFIDENCE_THRESHOLD + (MAX_BALL_CONFIDENCE - BALL_CONFIDENCE_THRESHOLD) / 2.0;
		}
	}

	detections.clear(); // Clear the detections for the next tick
}

void ParticleFilter::generateParticles(const std::vector<Point>& corePoints) {
	if(corePoints.empty()) {
		// If there are no basepoints, spread random points across the whole field
		for(unsigned int i = 0; i < particles.size(); i++) {
			double x = static_cast<double>(linearGenerator()) / ((double)linearGenerator.max() / length_) - length_ / 2;
			double y = static_cast<double>(linearGenerator()) / ((double)linearGenerator.max() / width_) - width_ / 2;
			particles[i].position = Point(x, y);
		}
	}else {
		// If there are basepoints, generate points around them with a gaussian distribution
		for(unsigned int i = 0; i < particles.size(); i++) {
			Point basepoint = corePoints[static_cast<unsigned int>(i / (particles.size() / corePoints.size()))];
			Point newParticle = Point();
			do {
				double x = distribution(generator) + basepoint.x;
				double y = distribution(generator) + basepoint.y;
				newParticle = Point(x, y);
			}while(!isInField(newParticle, WITHIN_FIELD_THRESHOLD));

			particles[i].position = newParticle;
		}
	}
}

void ParticleFilter::updateBallConfidence(double val) {
	double newConfidence = ballConfidence + val;
	if(newConfidence > MAX_BALL_CONFIDENCE) {
		ballConfidence = MAX_BALL_CONFIDENCE;
	}else if(newConfidence < 0.0) {
		ballConfidence = 0.0;
	}else {
		ballConfidence = newConfidence;
	}
}

void ParticleFilter::updateParticleConfidences() {
	// TODO: maybe just combine this with evaluateParticle into one function
	for(unsigned int k = 0; k < particles.size(); k++) {
		particles[k].confidence = evaluateParticle(particles[k].position);
	}
}

double ParticleFilter::evaluateParticle(const Point& particle) {
	double detectionScore = 0.0;
	for(unsigned int i = 0; i < detections.size(); i++) {
		double detectionDist = (particle - detections[i]).len();
		if(ballPosition != TMP_POINT) {
			detectionScore += getDetectionWeight((detections[i] - ballPosition).len()) * exp(-detectionDist);
		}else {
			detectionScore += MAX_DETECTION_WEIGHT * exp(-detectionDist);
		}
	}

	double previousBallScore = 0.0;
	if(ballPosition != TMP_POINT) {
		double ballDist = (particle - ballPosition).len();
		// This is an older equation from development. It should work also, but the last working test that was
		// done was with the sqrt function so we're using it for now.
//		previousBallScore += PREVIOUS_BALL_WEIGHT * exp(-0.5 * ballDist);

		// This weight will drop to 0 if ballDist is greater than BALL_DIST_THRESHOLD
		previousBallScore += PREVIOUS_BALL_WEIGHT * sqrt(-ballDist + BALL_DIST_THRESHOLD);
	}

	double predictionScore = 0.0;
	if(ballPosition != TMP_POINT && ballPredictedPosition != TMP_POINT) {
		double predictionDist = (particle - ballPredictedPosition).len();
		// This is an older equation from development. It should work also, but the last working test that was
		// done was with the sqrt function so we're using it for now.
//		predictionScore += PREDICTION_WEIGHT * exp(-predictionDist);

		// This weight will drop to 0 if ballDist is greater than BALL_DIST_THRESHOLD. 3 is a (somewhat arbitrary) constant to make sure
		// the weight doesn't drop to 0 unless the ball is even further away. Since the ball could bounce in the opposite direction of the
		// prediction, we still want reasonable bounces to gain weight from this function.
		predictionScore += PREDICTION_WEIGHT * sqrt(-predictionDist + BALL_DIST_THRESHOLD * 3);
	}

	return detectionScore + previousBallScore + predictionScore;
}

double ParticleFilter::getDetectionWeight(const double dist) {
	double weight = MAX_DETECTION_WEIGHT - DETECTION_WEIGHT_DECAY * dist;
	return weight < 0.0 ? 0.0 : weight;
}

bool ParticleFilter::isInField(const Point& p, double threshold) {
	return fabs(p.x) <= length_ / 2 + threshold && fabs(p.y) <= width_ / 2 + threshold;
}

Point ParticleFilter::getPointsMean(const std::vector<Point>& points) {
	Point average = Point(0, 0);
	for(unsigned int i = 0; i < points.size(); i++) {
		average += points[i];
	}

	average /= static_cast<double>(points.size());
	return average;
}

double ParticleFilter::getPointsVariance(const std::vector<Point>& points) {
	Point mean = getPointsMean(points);

	double sum = 0.0;
	for(unsigned int i = 0; i < points.size(); i++) {
		sum += (points[i] - mean).lensq();
	}

	sum /= static_cast<double>(points.size());
	return sqrt(sum);
}

Point ParticleFilter::getEstimate() {
	return ballPosition == TMP_POINT ? Point() : ballPosition;
}

double ParticleFilter::getEstimateVariance()
{
	return ballPositionVariance;
}
