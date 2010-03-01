#include <iostream>
#include "../doar/merger.h"
#include "../util/gettime.h"

int main(int argc, char** argv) {
  if(argc != 4) {
    std::cerr << "Usage: merge-doar <source-index-file-1> <source-index-file-2> <output-index-file>" << std::endl;
    return 1;
  }
  
  std::cerr << "=== Merge " << argv[1] << ", " << argv[2] << " ===" << std::endl;
  double beg_t = gettime();

  Doar::Merger mg;
  if(mg.merge(argv[1],argv[2]) != Doar::Status::OK) {
    std::cerr << "Merge failed" << std::endl;
    return 1;
  }
  std::cerr << "Elapsed: " << gettime()-beg_t << " sec" << std::endl << std::endl;

  std::cerr << "=== Save to " << argv[3] << " ===" << std::endl;
  beg_t = gettime();
  mg.save(argv[3]);
  std::cerr << "Elapsed: " << gettime()-beg_t << " sec" << std::endl << std::endl;

  return 0;
}
