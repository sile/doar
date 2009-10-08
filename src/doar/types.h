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

  struct header {
    unsigned node_size; // BASE and CHECK array size
    unsigned tind_size; // TAIL index array size  # TIND[BASE[i]] -> TAIL index
    unsigned tail_size; // TAIL array size
  };

  static const Code CODE_LIMIT    = 0xFF;
  static const Code TERMINAL_CODE = 0x00;
  static const Code VACANT_CODE   = CODE_LIMIT;
}

#endif
