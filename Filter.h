//-*-c++-*-
#ifndef _Filter_h_
#define _Filter_h_

using namespace std;

class Filter {
  int divisor;
  int dim;
  int *data;

public:
  Filter(int _dim);
  inline int get(int r, int c) const { return data[r * dim + c]; }
  void set(int r, int c, int value);

  inline int getDivisor() const { return divisor; }
  void setDivisor(int value);

  inline int getSize() const { return dim; }
  void info();
};

#endif
