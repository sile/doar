#include <iostream>
#include <fstream>
#include "../doar/merger.h"
#include "../util/gettime.h"

// TODO: help message
int main(int argc, char** argv) {
  Doar::Merger mg;
  std::cout << mg.merge(argv[1],argv[2]) << std::endl;
  mg.save(argv[3]);
  return 0;
}
