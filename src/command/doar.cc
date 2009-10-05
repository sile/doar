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

  Doar::NodeIndex node_idx=0;
  int level=0;
  while(getline(std::cin,word)) {
    Doar::ID id;
    
    if(word.empty()) {
      std::cerr << "Please input: key or key+ or key* or key-" << std::endl;
      continue;
    }

    switch(word[word.size()-1]) {
    case '*': 
      // prefix search
      {
	Doar::Searcher::Range range;
	if(srch.prefix_search(word.substr(0,word.size()-1).c_str(),range))
	  std::cout << " #"<<range.begin<<" <= "<<word<<" < "<<range.end<< std::endl;
	level=node_idx=0;
      }
      break;
    case '-':
      // 
      if((id=srch.search(word.substr(0,word.size()-1).c_str(),node_idx))!=srch.NOT_FOUND) {
	for(int i=0; i < level; i++) 
	  std::cout << "  ";
	std::cout << " #"<<id<<": "<<word<<std::endl;
	level++;
      } else {
	level=node_idx=0;
      }
      break;

    case '+':
      // common prefix search
      {
	Doar::Searcher::Log log;
	srch.search(word.substr(0,word.size()-1).c_str(),log);
	for(int i=0; i < log.size(); i++)
	  std::cout << " #"<<log[i].id<<": "<<word.substr(0,log[i].key_pos)<<std::endl;
	level=node_idx=0;
      }
      
    default:
      // default search
      if((id=srch.search(word.c_str()))!=srch.NOT_FOUND)
	std::cout <<" #"<<id<<": "<<word<<std::endl;
	level=node_idx=0;
    }
  }
  return 0;
}
