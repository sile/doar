#ifndef DOAR_NODE_LIST_H
#define DOAR_NODE_LIST_H

#include "node.h"
#include <vector>

namespace Doar {
  class NodeList : public std::vector<Node> {
  public:
    NodeList() : std::vector<Node>(0x10000) {}
    
    Node& at(unsigned index) {
      while(index >= size()) {
	resize(size()*2);
      }
      return operator[](index);
    }

    void clear() {
      std::vector<Node>::clear();
      resize(0x10000);
    }
  };
}

#endif
