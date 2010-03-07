#ifndef WAKAME_MORPHEME_NODE
#define WAKAME_MORPHEME_NODE

#include <vector>
#include "word.h"

#include "allocator.h"

namespace Wakame {
  struct MorphemeNode {
    MorphemeNode(const Word& w, unsigned length)
      : cost(0),length(length),prev(NULL),word(w) { }
    
    int cost;
    unsigned length;
    const MorphemeNode *prev;
    const Word& word;
  };

  // TODO: -> stack
  class ForwardList {
  public:
    typedef MorphemeNode T;
    
    struct Node {
      Node(const Word& w, unsigned length) : value(w, length),next(NULL) {}

      T value;
      Node* next;
    };
    // XXX: for dev
    static json::chunk mem;
    static json::allocator<Node> alloc;

  public:
    ForwardList() : _front(NULL) {}

    void push_front(const Word& w, unsigned length) {
      Node* tmp = new (alloc.allocate()) Node(w,length);
      tmp->next = _front;
      _front = tmp;
    }
    
    Node* front() const { return _front; }

    bool empty() const { return !_front; }

  private:
    Node* _front;
  };  
  json::chunk ForwardList::mem;
  json::allocator<ForwardList::Node> ForwardList::alloc(ForwardList::mem);
  typedef ForwardList MorphemeNodes;  
}

#endif
