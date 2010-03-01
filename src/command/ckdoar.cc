#include <iostream>
#include "../doar/searcher.h"
#include "../util/read_line.h"
#include "../util/gettime.h"

int main(int argc, char** argv) {
  if(argc < 3) {
    std::cerr <<"Usage: ckdoar <index-file> <word-list-file> [loop-count]" << std::endl;
    return 1;
  }
  
  double beg_t = gettime(); 
  std::cerr << "=== Load from " << argv[1] << " ===" << std::endl;
  Doar::Searcher srch(argv[1]);
  if(!srch) {
    std::cerr <<"Can't open file: "<<argv[1] << std::endl;
    return 1;
  }
  std::cerr << "Key set size: " << srch.size() << std::endl;
  std::cerr << "Elapsed: " << gettime()-beg_t << " sec" << std::endl << std::endl;


  ReadLine rl(argv[2]);
  if(!rl) {
    std::cerr <<"Can't open file: "<<argv[2] << std::endl;
    return 1;
  }

  const int TIMES=argc<=3 ? 1 : atoi(argv[3]);
  const int interval = static_cast<int>(TIMES*rl.size()/10);

  int cnt=0, err=0;
  const char* line;

  std::cerr << "=== Search ===" << std::endl;
  beg_t = gettime(); 
  for(int i=0; i < TIMES; i++) {
    rl.reset();
    while((line=rl.read())) {
      cnt++;
      if(cnt % interval == 0)
	std::cerr << " # " << cnt << std::endl;

      if(!srch.search(line))
	err++;
    }
  }
  
  std::cerr << "Not Found: " << err << "/" << cnt << std::endl;
  std::cerr << "Elapsed: " << gettime()-beg_t << " sec" << std::endl << std::endl;
  std::cerr << "DONE" << std::endl;
  return 0;
}

