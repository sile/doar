#ifndef DOAR_NODE_H
#define DOAR_NODE_H

#include "types.h"

namespace Doar {
  /********/
  /* Base */
  class Base {
  public:
    Base() : data(0xFFFFFFFF) {}
    Base (const Base& src) : data(src.data){}
    
    NodeIndex base() const  { return data; }
    uint32    id()   const  { return data&0x7FFFFFFF; } 
    NodeIndex next_index(Code cd) const  { return base()+cd; }
    bool is_leaf() const { return (data&0x80000000) != 0; }  
    
    void set_base(NodeIndex idx) { data=idx; }
    void set_id(uint32 id)       { data= id | 0x80000000; }

  protected:
    uint32 data;
  };
  typedef Vector<Base,CODE_LIMIT> BaseList; 

  /********/
  /* Node */
  // NOTE: Actually, Node is the same to Base. 
  //       But the former provide convenient (restricted?) interface for user.
  class Node : protected Base {
  public:
    Node() : Base() {}
    
    uint32   id()   const { return Base::id(); }
    operator bool() const { return data!=0xFFFFFFFF; }
    bool is_leaf()  const { return Base::is_leaf(); }
    
  private:
    friend class SearcherBase;
    Node(Base b) : Base(b) {}
  };
  
  /********/
  /* Chck */
  class Chck {
  public:    
    Chck() : data(VACANT_CODE) {}
    bool in_use() const { return data!=VACANT_CODE; }
    bool trans_by(Code cd) const { return cd==data; }

    void set_chck(Code cd) { data=cd; }
  private:
    unsigned char data;
  };
  typedef Vector<Chck,CODE_LIMIT> ChckList;
}
#endif
