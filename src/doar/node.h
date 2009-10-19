#ifndef DOAR_NODE_H
#define DOAR_NODE_H

// TODO: node_list.hとまとめる?
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
}
#endif
