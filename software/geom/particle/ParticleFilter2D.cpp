#include "geom/particle/ParticleFilter2D.h"

ParticleFilter2D::ParticleFilter2D(double lengthX, double lengthY, double offsetX, double offsetY, unsigned int numPartitions)
{
	xFilter = new ParticleFilter(lengthX, offsetX, numPartitions);
	yFilter = new ParticleFilter(lengthY, offsetY, numPartitions);
}

ParticleFilter2D::~ParticleFilter2D()
{
	xFilter->~ParticleFilter();
	yFilter->~ParticleFilter();
}

void ParticleFilter2D::update(double timeDelta)
{
	xFilter->update(timeDelta);
	yFilter->update(timeDelta);
}

void ParticleFilter2D::add(double valueX, double valueY, unsigned int numParticles)
{
	xFilter->add(valueX, numParticles);
	yFilter->add(valueY, numParticles);
}

void ParticleFilter2D::toString()
{
	xFilter->toString();
	yFilter->toString();
}

double ParticleFilter2D::getXEstimate()
{
	return xFilter->getEstimate();
}

double ParticleFilter2D::getYEstimate()
{
	return yFilter->getEstimate();
}

