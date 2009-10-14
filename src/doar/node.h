#ifndef DOAR_NODE_H
#define DOAR_NODE_H

namespace Doar{
  struct Node {
    Node() : data(0xFFFFFFFF) {}
    unsigned id()  const { return tail_index(); }
    bool valid()   const { return data!=0xFFFFFFFF; }
    bool is_leaf() const { return data&0x80000000; } 
    static const Node INVALID;

  private:
    friend class Builder;
    friend class Searcher;
    friend class DoubleArray;

    explicit Node(unsigned d) : data(d) {}
    
    NodeIndex base() const  { return data; }
    NodeIndex next_index(Code cd) const  { return base()+cd; }
    TailIndex tail_index() const  { return data&0x7FFFFFFF; }
    
    void set_base(NodeIndex idx) { data=idx; }
    void set_tail_index(TailIndex idx) { data= idx | 0x80000000; }
    
    unsigned data;
  };
  const Node Node::INVALID = Node();
}
#endif
