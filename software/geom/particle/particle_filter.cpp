#include "particle_filter.h"

using namespace AI::BE::Vision::Particle;

DoubleParam AI::BE::Vision::Particle::PARTICLE_FILTER_VAR_THRESH(
		u8"Particle Filter Variance Threshold",
		u8"AI/Backend/Vision/Particle", 0.1, 0.0, 2.0);
DoubleParam AI::BE::Vision::Particle::PARTICLE_FILTER_DECAYRATE(
		u8"Particle Filter Decay Rate",
		u8"AI/Backend/Vision/Particle", 0.1, 0, 1.0);

Particle::Particle(Point position, Point velocity, Point acceleration) :
		position_(position), velocity_(velocity), acceleration_(acceleration)
{
}

Point Particle::getPosition()
{
	return position_;
}

void Particle::updateVelocity(double timeDelta)
{
	velocity_.x = velocity_.x + timeDelta*acceleration_.x;
	velocity_.y = velocity_.y + timeDelta*acceleration_.y;
}

void Particle::updatePosition(double timeDelta)
{
	position_.x = position_.x + timeDelta*velocity_.x;
	position_.y = position_.y + timeDelta*velocity_.y;
}

ParticleFilter::ParticleFilter(double length, double width) :
		length_(length),
		width_(width),
		estimateValid_(false)
{
	time_t  timev;

	add(Point(0,0));
	srand(static_cast<unsigned int>(time(&timev)));
}

void ParticleFilter::update(double timeDelta) {
	// Update particle cloud parameters
	position_[1] = position_[0];
	positionVar_[1] = positionVar_[0];

	position_[0] = Point(0,0);
	positionVar_[0] = Point(0,0);

	velocity_[1] = velocity_[0];
	velocityVar_[1] = velocity_[1];

	velocity_[0] = Point(0,0);
	velocityVar_[0] = Point(0,0);

	acceleration_.x = (velocity_[0].x - velocity_[1].x)/timeDelta;
	acceleration_.y = (velocity_[0].y - velocity_[1].y)/timeDelta;
	accelerationVar_.x = velocityVar_[0].x + velocityVar_[1].x;
	accelerationVar_.y = velocityVar_[0].y + velocityVar_[1].y;

	// Update each particle
	for(std::vector<Particle>::iterator it = particles_.begin();
			it != particles_.end();) {
	    if (rand()%100 >= PARTICLE_FILTER_DECAYRATE*100)
	    {
	        // Decay Particles
	    	// Each particle has a probability of PARTICLE_FILTER_DECAYRATE of
	    	// decaying
	    	it = particles_.erase(it);
	    }
	    else
	    {
	    	// Update Particles
	    	it->updateVelocity(timeDelta);
	    	it->updatePosition(timeDelta);
	    	++it;
	    }
	}
}

void ParticleFilter::add(Point ballLocation) {
	if (!isnan(ballLocation.x + ballLocation.y))
	{
		particles_.push_back(Particle(ballLocation, velocity_[1], acceleration_));
		estimateValid_ = false;
	}
}

Point ParticleFilter::getEstimate() {
	if (!estimateValid_)
	{
		updateEstimatedPartition();
	}

	return position_[0];
}

Point ParticleFilter::getEstimateVariance()
{
	if (!estimateValid_)
	{
		updateEstimatedPartition();
	}

	return positionVar_[0];
}

void ParticleFilter::updateEstimatedPartition() {
	Point sum(0, 0);

	// Calculate mean of the points
	for(std::vector<Particle>::iterator it = particles_.begin();
			it != particles_.end(); ++it) {
	    sum.x += it->getPosition().x;
	    sum.y += it->getPosition().y;
	}

	position_[0].x = sum.x / static_cast<double>(particles_.size());
	position_[0].y = sum.y / static_cast<double>(particles_.size());

	// Calculate the variance of the points
	sum.x = 0;
	sum.y = 0;

	for(std::vector<Particle>::iterator it = particles_.begin();
			it != particles_.end(); ++it) {
	    sum.x += (it->getPosition().x - position_[0].x)*(it->getPosition().x -
	    		position_[0].x);
	    sum.y += (it->getPosition().y - position_[0].y)*(it->getPosition().y -
	    		position_[0].y);
	}

	positionVar_[0].x = sum.x / static_cast<double>(particles_.size());
	positionVar_[0].y = sum.y / static_cast<double>(particles_.size());

	estimateValid_ = true;
}
