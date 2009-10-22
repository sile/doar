#ifndef DOAR_TYPES_H
#define DOAR_TYPES_H

#include <string>
#include "vector.h"

namespace Doar {
  // NOTE: Portability note
  //   Please redefine 32bit(4byte) unsigned integer type
  //   if sizeof(unsigned) != 8 on your environment.
  typedef unsigned uint32;

  //class Node;   
  //class Base;
  //class Chck;
  //class BaseList;
  //class ChckList;

  typedef uint32 NodeIndex;
  typedef uint32 ElemID;
  typedef uint32 TailIndex;
  typedef uint32 Code;
  
  typedef Vector<Code>      CodeList;
  typedef Vector<TailIndex> TindList;  // Tail INDex List
  typedef std::string       Tail;
  
  struct Header {
    uint32 node_size;  // BASE and CHECK array size
    uint32 tind_size; 
    uint32 tail_size; 
  };
  
  const Code CODE_LIMIT    = 0xFF;
  const Code TERMINAL_CODE = 0x00;
  const Code VACANT_CODE   = 0xFF;
}

#endif
