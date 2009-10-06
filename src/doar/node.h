#ifndef DOAR_NODE_H
#define DOAR_NODE_H

#include <stdexcept>
#include <cassert>

namespace Doar{
  struct Node {
    Node() : data(0) {}
    Node(unsigned d) : data(d) {}
    
    NodeIndex base() const  { return data; }
    NodeIndex next_index(Code cd) const  { return base()+cd; }
    TailIndex tail_index() const  { return data&0x7FFFFFFF; }
    
    bool is_terminal() const { return data&0x80000000; }
    
    void set_base(NodeIndex idx) { data=idx; }
    void set_tail_index(TailIndex idx) { data= idx | 0x80000000; }
    
    unsigned data;
  };
}
#endif
