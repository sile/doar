#include <iostream>
#include <string>
#include "../doar/double_array.h"
#include "../doar/builder.h"

int main(int argc, char** argv) {
  if(argc != 2) {
    std::cerr <<"Usage: "<<argv[0]<<" <doar-index-file>" << std::endl;
    return 1;
  }
  
  Doar::DoubleArray trie;
  std::string line;

  std::cerr << "=== INSERT ===" << std::endl;
  while(getline(std::cin,line))
    trie.insert(line.c_str());

  Doar::Builder bld;
  std::cerr << "=== BUILD ===" << std::endl;
  bld.build(trie);

  std::cerr << "=== SAVE ===" << std::endl;
  bld.save(argv[1]);

  std::cerr << "DONE" << std::endl;
  return 0;
}
