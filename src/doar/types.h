#ifndef DOAR_TYPES_H
#define DOAR_TYPES_H

#include <string>

#include <algorithm>
#include "mmap_t.h"
#include "vector.h"

namespace Doar {
  // NOTE: Please redef ...
  typedef int       int32;
  typedef unsigned uint32;
  
  class Node;
  class Chck;
  typedef uint32         NodeIndex;
  typedef uint32         TailIndex; // TODO: 用語説明。二重性 base[i] and tind[i]
  typedef uint32         Code;
  
  typedef Vector<Code>      CodeList;
  typedef Vector<uint32>  TindList;    // TailIndex list?
  typedef std::string            Tail;

  struct header {
    uint32 node_size; // BASE and CHECK array size
    uint32 tind_size; // TAIL index array size  # TIND[BASE[i]] -> TAIL index
    uint32 tail_size; // TAIL array size
  };

  static const Code CODE_LIMIT    = 0xFF;
  static const Code TERMINAL_CODE = 0x00;
  static const Code VACANT_CODE   = CODE_LIMIT;
}

#endif
