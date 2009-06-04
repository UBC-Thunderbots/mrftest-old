#ifndef FILTER_H
#define FILTER_H

template<byte asize>
class Filter {
public:
  // The types of the parameter arrays.
  typedef double A[asize];
  typedef double B[asize + 1];

  // Construct a new filter with no data in it.
  Filter(const A &a, const B &b) : a(a), b(b) {
    nuke();
  }

  // Adds a value to the filter and returns its current output.
  double process(double input) {
    for (byte i = 0; i < asize; i++)
      input -= a[i] * delayed[i];
      
    double output = b[0] * input;
    for (byte i = 0; i < asize; i++)
      output += b[i + 1] * delayed[i];

    for (byte i = asize - 1; i; --i)
      delayed[i] = delayed[i - 1];
    delayed[0] = input;
    
    return output;
  }
  
  // Clears the filter.
  void nuke() {
    for (byte i = 0; i < asize; i++)
      delayed[i] = 0;
  }

private:
  double delayed[asize];
  const A &a;
  const B &b;
};

#endif
