#ifndef DOAR_NODE_H
#define DOAR_NODE_H

#include "types.h"

namespace Doar{
  class Node {
  public:
    Node() : data(0xFFFFFFFF) {}
    uint32   id()  const { return tail_index(); }
    operator bool() const { return data!=0xFFFFFFFF; }
    bool is_leaf() const { return data&0x80000000; } 
    static const Node INVALID;

  private:
    friend class Builder;
    friend class SearcherBase;
    friend class DoubleArray;
    friend class DynamicAllocator;

    explicit Node(uint32 d) : data(d) {}
    
    NodeIndex base() const  { return data; }
    NodeIndex next_index(Code cd) const  { return base()+cd; }
    TailIndex tail_index() const  { return data&0x7FFFFFFF; }
    
    void set_base(NodeIndex idx) { data=idx; }
    void set_tail_index(TailIndex idx) { data= idx | 0x80000000; }
    
    uint32 data;
  };
  const Node Node::INVALID = Node();
  
  typedef Vector<Node,CODE_LIMIT> BaseList; // XXX:

  
  class Chck {
  public:
    Chck() : data(VACANT_CODE) {}
    
    bool vacant() const { return data==VACANT_CODE; }
    bool verify(Code cd) const { return cd==data; }   // XXX:

    void set_chck(Code cd) { data=cd; }
    void set_chck(Chck ch) { data=ch.data; }
  private:
    unsigned char data;
  };

  typedef Vector<Chck,CODE_LIMIT> ChckList;
}
#endif
