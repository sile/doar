#ifndef WAKAME_WORD_H
#define WAKAME_WORD_H

#include <vector>

namespace Wakame {
  struct Word {
    typedef short COST;
    typedef unsigned short ID;
    
    Word(ID lid, ID rid, COST cst, unsigned idx) 
      : index_of_info(idx), lft_id(lid), rgt_id(rid), cost(cst) {}
    unsigned index_of_info;
    ID lft_id;
    ID rgt_id;
    COST cost;
  };
  const Word BOS_EOS_WORD(0,0,0,0);
  
  typedef std::vector<Word> Words;
}

#endif
