#include <iostream>
#include <string>
#include "../doar/searcher.h"

class print_lattice {
public:
  print_lattice(const Doar::Searcher& sh, const std::string& s,unsigned bg) 
    : srch(sh),str(s),beg(bg) {}
  
  void operator()(const char* key, unsigned offset, unsigned id) const {
    printf(" %02X-%02X #%06X: ",beg,beg+offset,id);         // 一致位置とID
    std::cout << str.substr(0,beg)      <<"\033[31m\033[1m"
      	      << str.substr(beg,offset) <<"\033[0m"         // 一致文字列を赤字で表示
	      << str.substr(beg+offset) << std::endl;
    print_lattice next(srch,str,beg+offset); // 再帰的に処理する
    srch.common_prefix_search(key+offset,next);
  }
private:
  const Doar::Searcher& srch;
  const std::string& str;
  unsigned beg;
};

void print_common_prefix(const char* key, unsigned len, unsigned id) {
  std::cout << " #"<< id <<": "<<std::string(key,key+len)<<" ["<<len<<"]"<< std::endl;	  
}


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
      std::cerr << "Please input 'KEY'(default search) or 'KEY+'(common prefix search) or 'KEY~'(show lattice)" << std::endl;
      continue;
    }
    switch(word[word.size()-1]) {
    case '+':
      // common prefix search
      {
	srch.common_prefix_search(word.c_str(), print_common_prefix);
      }
      break;
    case '~':
      {
	std::string key =  word.substr(0,word.size()-1);
	srch.common_prefix_search(key.c_str(), print_lattice(srch,key,0));
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
