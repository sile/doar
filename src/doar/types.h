#ifndef DOAR_TYPES_H
#define DOAR_TYPES_H

#include <string>
#include "vector.h"

// NOTE: G++ of a recent version has been implementing std::vector.data() method 
//       with below macro constant.
//       if your compiler also has been implementing this, please define it.
#ifdef _GLIBCXX_RESOLVE_LIB_DEFECTS 
# define VECTOR_HAS_MEMBER_NAMED_DATA
#endif

namespace Doar {
  // NOTE: Please redefine 32bit(4byte) unsigned integer type
  //       if sizeof(unsigned) != 4 on your environment.
  typedef unsigned uint32;

  typedef uint32 NodeIndex;
  typedef uint32 ElemID;
  typedef uint32 TailIndex;
  typedef uint32 Code;
  
  typedef Vector<Code>      CodeList;
  typedef Vector<TailIndex> TindList;  // Tail INDex List
  typedef std::string       Tail;
  
  const char* MAGIC_STRING="doar-000";
  struct Header {
    char magic_s[8];  // 'doar-xxx'
    uint32 node_size; // BASE and CHECK array size
    uint32 tind_size; 
    uint32 tail_size; 
  };
  
  const Code CODE_LIMIT    = 0xFF;
  const Code TERMINAL_CODE = 0x00;
  const Code VACANT_CODE   = 0xFF;
}

#endif
