#include "geom/particle/particle_filter_2d.h"

ParticleFilter2D::ParticleFilter2D(Point length, Point offset, unsigned int numPartitions) {
	xFilter = new ParticleFilter(length.x, offset.x, numPartitions);
	yFilter = new ParticleFilter(length.y, offset.y, numPartitions);
}

ParticleFilter2D::~ParticleFilter2D() {
	xFilter->~ParticleFilter();
	yFilter->~ParticleFilter();
}

void ParticleFilter2D::update(double timeDelta) {
	xFilter->update(timeDelta);
	yFilter->update(timeDelta);
}

void ParticleFilter2D::add(Point p, unsigned int numParticles) {
	xFilter->add(p.x, numParticles);
	yFilter->add(p.y, numParticles);
}

void ParticleFilter2D::toString() {
	xFilter->toString();
	yFilter->toString();
}

Point ParticleFilter2D::getEstimate() {
	return Point(xFilter->getEstimate(), yFilter->getEstimate());
}

