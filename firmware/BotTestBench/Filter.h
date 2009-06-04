#ifndef FILTER_H
#define FILTER_H

class Filter {
public:

  // Construct a new filter with no data in it.
  Filter(double B_0, double B_1, double A_1);
  Filter(double B_0, double B_1, double A_1,double B_2,double A_2);

  //executes the magic
  double process(double input);
  void nuke();

private:
  double B_0,B_1,A_1,B_2,A_2;
  double delayed[2];
  bool full;  
};

#endif
