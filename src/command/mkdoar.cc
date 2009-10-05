#include <iostream>
#include "../doar/builder.h"

int main(int argc, char** argv) {
  if(argc != 3) {
    std::cerr <<"Usage: "<<argv[0]<<" <word-list-file> <doar-index-file>" << std::endl;
    return 1;
  }
  
  Doar::Builder bld;
  bld.build(argv[1]);
  bld.save(argv[2]);

  return 0;
}
