#include <iostream>
#include <string>
#include "../doar/double_array.h"
#include "../doar/builder.h"

int main(int argc, char** argv) {
  if(argc < 2) {
    std::cerr <<"Usage: "<<argv[0]<<" [--extend=[extended-index-file]] <doar-index-file>" << std::endl;
    return 1;
  }
  
  Doar::DoubleArray trie;
  std::string line;

  const char* save_file_name=argv[1];
  
  if(strstr(argv[1], "--extend")) {
    // xxx:
    std::cerr << "=== LOAD EXISTING DATA ===" << std::endl;
    trie.load(argv[2]);

    if(strchr(argv[1],'=')) {
      save_file_name = strchr(argv[1],'=')+1;
    } else {
      save_file_name = argv[2];
    }
  }

  std::cerr << "=== INSERT ===" << std::endl;
  while(getline(std::cin,line)) {
    trie.insert(line.c_str());
    //break;
  }

  std::cerr << "=== SAVE ===" << std::endl;
  trie.save(save_file_name);

  std::cerr << "DONE" << std::endl;
  
  return 0;
}
