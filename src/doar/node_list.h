#ifndef DOAR_NODE_LIST_H
#define DOAR_NODE_LIST_H

#include "types.h"
#include "node.h"
#include <vector>

// TODO: 整理
namespace Doar {
  class BaseList : public std::vector<Node> {
  public:
    BaseList() : std::vector<Node>(0x10000) {}
    
    Node& at(uint32 index) {
      for(; index >= size()-CODE_LIMIT; resize(size()*2));
      return operator[](index);
    }

    void clear() {
      std::vector<Node>::clear();
      resize(0x10000);
    }
  };

  class ChckList : public std::vector<Chck> {
  public:
    ChckList() { resize(0x10000); }

    Chck& at(uint32 index) {
      for(; index >= size()-CODE_LIMIT; resize(size()*2));
      return operator[](index);
    }

    void clear() {
      std::vector<Chck>::clear();
      resize(0x10000);
    }
    
    void resize(std::size_t new_size) { std::vector<Chck>::resize(new_size, VACANT_CODE); }
  };
}

#endif
