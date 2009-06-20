#ifndef TB_CONTROLFILTER_H
#define TB_CONTROLFILTER_H

#include <vector>

class ControlFilter {
public:
	ControlFilter() {}
	virtual double process(double error) = 0;
	virtual void clear() = 0;
};

class MoveFilter : public ControlFilter {
public:
	MoveFilter(const std::vector<double>& ka, const std::vector<double>& kb);
	void clear();
	double process(double input);

private:
	const std::vector<double> a;
	const std::vector<double> b;
	std::vector<double> delayed;
};

class PID : public ControlFilter {
public:
	PID(double Kp, double Ki, double Kd, double decay);
	void clear();
	double process(double error);
	
private:
	const double Kp, Ki, Kd, decay;
	double integral, prev;
	bool hasPrev;
};

#endif

