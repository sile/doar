#ifndef DOAR_TYPES_H
#define DOAR_TYPES_H

#include <vector>
#include <string>

namespace Doar {
  class Node;
  
  typedef unsigned NodeIndex;
  typedef unsigned TailIndex;
  typedef unsigned Code;
  
  typedef std::vector<Code>      CodeList;
  typedef std::vector<unsigned>  TindList;
  typedef std::string            Tail;
  typedef std::vector<NodeIndex> NodeIndexList;

  struct header {
    unsigned node_size; // BASE and CHECK array size
    unsigned tind_size; // TAIL index array size  # TIND[BASE[i]] -> TAIL index
    unsigned tail_size; // TAIL array size
  };
}

#endif
