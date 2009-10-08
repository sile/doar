#include <iostream>
#include <string>
#include "../doar/searcher.h"

// XXX: name
struct stt {
  stt(const Doar::Searcher& sh,std::string s,unsigned beg_=0) : srch(sh),orig(s),beg(beg_) {}
  void operator()(const char* key, unsigned i, unsigned id) {
    std::cout << " #"<<id<<": "
	      << orig.substr(0,beg) <<"\033[31m\033[1m"
	      << orig.substr(beg,i) <<"\033[0m"
	      << orig.substr(beg+i) << std::endl;
    
    stt yyy(srch,orig,beg+i);
    srch.common_prefix_search(key+i,yyy);
  }
  const Doar::Searcher& srch;
  unsigned beg;
  std::string orig;
};


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
	while((node=srch.common_prefix_search(key.c_str(), offset, root_node)).valid()) {
	  std::cout << " #"<<node.id()<<": "<<word.substr(0,offset)<<" ["<<offset<<"]"<< std::endl;	  
	}
      }
      break;
    case '~':
      {
	std::string key =  word.substr(0,word.size()-1);
	stt xxx(srch,key);
	srch.common_prefix_search(key.c_str(), xxx);
      }
      break;
    default:
      // default search
      if((node=srch.search(word.c_str())).valid())
	std::cout <<" #"<<node.id()<<": "<<word<<std::endl;
    }
  }
  return 0;
}
