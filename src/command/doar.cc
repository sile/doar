#include <iostream>
#include <string>
#include "../doar/searcher.h"

int main(int argc, char** argv) {
  if(argc != 2) {
    std::cerr <<"Usage: "<<argv[0]<<" <doar-index-file>" << std::endl;
    return 1;
  }

  Doar::Searcher srch(argv[1]);
  if(!srch) {
    std::cerr <<"Can't open file: "<<argv[1] << std::endl;
    return 1;
  }

  std::string word;

  while(getline(std::cin,word)) {
    Doar::Node node;
    
    if(word.empty()) {
      std::cerr << "Please input 'KEY'(default search) or 'KEY+'(common prefix search)" << std::endl;
      continue;
    }
    switch(word[word.size()-1]) {
    case '+':
      // common prefix search
      {
	Doar::Node root_node = srch.root_node();
	unsigned offset=0;
	std::string key =  word.substr(0,word.size()-1);
	do{
	  node=srch.non_greedy_search(key.c_str(), offset, root_node);
	  if(node.valid())
	    std::cout << " #"<<node.id()<<": "<<word.substr(0,offset)<<std::endl;	  
	} while(root_node.valid());
      }
      
    default:
      // default search
      if((node=srch.search(word.c_str())).valid())
	std::cout <<" #"<<node.id()<<": "<<word<<std::endl;
    }
  }
  return 0;
}
