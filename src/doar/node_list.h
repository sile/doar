#ifndef DOAR_NODE_LIST_H
#define DOAR_NODE_LIST_H

#include "node.h"
#include <vector>

namespace Doar {
  class BaseList : public std::vector<Node> {
  public:
    BaseList() : std::vector<Node>(0x10000) {}
    
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

  class ChckList : public std::vector<unsigned char> {
  public:
    ChckList() { resize(0x10000); }

    unsigned char& at(unsigned index) {
      while(index >= size()) {
	resize(size()*2);
      }
      return operator[](index);
    }

    void clear() {
      std::vector<unsigned char>::clear();
      resize(0x10000);
    }
    
    void resize(std::size_t new_size) { std::vector<unsigned char>::resize(new_size, VACANT_CODE); }
  };
}

#endif
