#include "geom/particle/ParticleFilter2D.h"

ParticleFilter2D::ParticleFilter2D(double lengthX, double lengthY, double offsetX, double offsetY, unsigned int numPartitions)
{
	std::cout << "CREATING X FILTER" << std::endl;
	xFilter = new ParticleFilter(lengthX, offsetX, numPartitions);
	std::cout << "CREATING Y FILTER" << std::endl;
	yFilter = new ParticleFilter(lengthY, offsetY, numPartitions);
}

ParticleFilter2D::~ParticleFilter2D()
{
	xFilter->~ParticleFilter();
	yFilter->~ParticleFilter();
}

void ParticleFilter2D::update(double timeDelta)
{
	//std::cout << "UPDATING X Filter" << std::endl;
	xFilter->update(timeDelta, false);
	//std::cout << "UPDATING Y Filter" << std::endl;
	yFilter->update(timeDelta, false);
}

void ParticleFilter2D::add(double valueX, double valueY, unsigned int numParticles)
{
	std::cout << "ADDING TO X FILTER - ";
	xFilter->add(valueX, numParticles, false);
	std::cout << "ADDING TO Y FILTER - ";
	yFilter->add(valueY, numParticles, false);
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

