#ifndef DOAR_NODE_H
#define DOAR_NODE_H

namespace Doar{
  class Builder;
  class Searcher;

  struct Node {
    Node() : data(0) {}
    unsigned id() const { return tail_index(); }
    bool  valid() const { return data!=0xFFFFFFFF; }
    static const Node INVALID;

  private:
    friend class Builder;
    friend class Searcher;

    explicit Node(unsigned d) : data(d) {}
    
    NodeIndex base() const  { return data; }
    NodeIndex next_index(Code cd) const  { return base()+cd; }
    TailIndex tail_index() const  { return data&0x7FFFFFFF; }
    
    bool is_terminal() const { return data&0x80000000; }
    
    void set_base(NodeIndex idx) { data=idx; }
    void set_tail_index(TailIndex idx) { data= idx | 0x80000000; }
    
    unsigned data;
  };
  const Node Node::INVALID = Node(0xFFFFFFFF);
}
#endif
