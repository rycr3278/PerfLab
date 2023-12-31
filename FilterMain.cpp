#include <stdio.h>
#include "cs1300bmp.h"
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include "Filter.h"

using namespace std;

#include "rdtsc.h"

//
// Forward declare the functions
//
Filter * readFilter(string filename);
double applyFilter(Filter *filter, cs1300bmp *input, cs1300bmp *output);

int
main(int argc, char **argv)
{

  if ( argc < 2) {
    fprintf(stderr,"Usage: %s filter inputfile1 inputfile2 .... \n", argv[0]);
  }

  //
  // Convert to C++ strings to simplify manipulation
  //
  string filtername = argv[1];

  //
  // remove any ".filter" in the filtername
  //
  string filterOutputName = filtername;
  string::size_type loc = filterOutputName.find(".filter");
  if (loc != string::npos) {
    //
    // Remove the ".filter" name, which should occur on all the provided filters
    //
    filterOutputName = filtername.substr(0, loc);
  }

  Filter *filter = readFilter(filtername);

  double sum = 0.0;
  int samples = 0;

  for (int inNum = 2; inNum < argc; inNum++) {
    string inputFilename = argv[inNum];
    string outputFilename = "filtered-" + filterOutputName + "-" + inputFilename;
    struct cs1300bmp *input = new struct cs1300bmp;
    struct cs1300bmp *output = new struct cs1300bmp;
    int ok = cs1300bmp_readfile( (char *) inputFilename.c_str(), input);

    if ( ok ) {
      double sample = applyFilter(filter, input, output);
      sum += sample;
      samples++;
      cs1300bmp_writefile((char *) outputFilename.c_str(), output);
    }
    delete input;
    delete output;
  }
  fprintf(stdout, "Average cycles per sample is %f\n", sum / samples);

}

class Filter *
readFilter(string filename)
{
  ifstream input(filename.c_str());

  if ( ! input.bad() ) {
    int size = 0;
    input >> size;
    Filter *filter = new Filter(size);
    int div;
    input >> div;
    filter -> setDivisor(div);
    for (int i=0; i < size; i++) {
      for (int j=0; j < size; j++) {
	int value;
	input >> value;
	filter -> set(i,j,value);
      }
    }
    return filter;
  } else {
    cerr << "Bad input in readFilter:" << filename << endl;
    exit(-1);
  }
}


// inline function to check if RGB vals are legal
inline int clamp(int value) {
    if (value < 0) return 0;
    if (value > 255) return 255;
    return value;
}

double applyFilter(class Filter *filter, cs1300bmp *input, cs1300bmp *output) {
    long long cycStart, cycStop;
    cycStart = rdtscll();

    output->width = input->width;
    output->height = input->height;

    // Storing filter values ahead of time to reduce function calls
    // and improve temporal locality
    int filterValues[3][3];
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            filterValues[i][j] = filter->get(i, j); // Using inline function filter->get()
        }
    }
  
    // Computing the divisor once outside the loop to avoid redundant computations
    int filterDivisor = filter->getDivisor(); // Using inline function filter->getDivisor()

    // Changing loop order to row-major for improving spatial locality and cache efficiency
    for (int row = 1; row < input->height - 1; row++) {
        for (int col = 1; col < input->width - 1; col++) {
            for (int plane = 0; plane < 3; plane++) {
                int sum = 0; // Using a single accumulator here, but you could split into multiple if needed

                // Unrolled loop for applying the 3x3 filter
                for (int i = 0; i < 3; i++) {
                    for (int j = 0; j < 3; j++) {
                        sum += input->color[plane][row + i - 1][col + j - 1] * filterValues[i][j];
                    }
                }

                // Applying the clamp function once per pixel after summing
                // to reduce conditional branching inside the loop
                output->color[plane][row][col] = clamp(sum / filterDivisor); // Using inline function clamp()
            }
        }
    }

    cycStop = rdtscll();
    double diff = cycStop - cycStart;
    double diffPerPixel = diff / (output->width * output->height);
    fprintf(stderr, "Took %f cycles to process, or %f cycles per pixel\n",
            diff, diff / (output->width * output->height));
    return diffPerPixel;
}


