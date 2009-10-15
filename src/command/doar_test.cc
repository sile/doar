#include <iostream>
#include "../doar/searcher.h"
#include "../util/read_line.h"
#include "../util/gettime.h"

int main(int argc, char** argv) {
  if(argc < 3) {
    std::cerr <<"Usage: "<<argv[0]<<" <doar-index-file> <word-list-file> [loop-count]" << std::endl;
    return 1;
  }

  Doar::Searcher srch(argv[1]);
  if(!srch) {
    std::cerr <<"Can't open file: "<<argv[1] << std::endl;
    return 1;
  }

  ReadLine rl(argv[2]);
  if(!rl) {
    std::cerr <<"Can't open file: "<<argv[2] << std::endl;
    return 1;
  }

  const int TIMES=argc<=3 ? 1 : atoi(argv[3]);
  const int interval = TIMES*rl.size()/10;

  int cnt=0, err=0;
  const char* line;
  double beg_t = gettime(); 

  for(int i=0; i < TIMES; i++) {
    rl.reset();
    while((line=rl.read())) {
      cnt++;
      if(cnt % interval == 0)
	std::cerr << " # " << cnt << std::endl;

      if(!srch.search(line).valid())
	err++;
    }
  }
  
  std::cerr << "Not Found: " << err << "/" << cnt << std::endl;
  std::cerr << "Elapsed: " << gettime()-beg_t << " sec" << std::endl;
  return 0;
}

