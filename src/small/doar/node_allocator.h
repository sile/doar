#ifndef DOAR_NODE_ALLOCATOR_H
#define DOAR_NODE_ALLOCATOR_H

#include "types.h"
#include <bitset>
#include <vector>
#include <assert.h>

namespace Doar {
  class NodeAllocator {
    static const unsigned BITSET_SIZE=0x1000000;
    typedef std::bitset<BITSET_SIZE> Bitset;

    struct Link {
      Link(unsigned n) : next(n) {}
      unsigned next;
    };
    typedef std::vector<Link> LinkList;

  public:
    NodeAllocator() {
      lnk.push_back(Link(0));
      resize_link(0xFFFF);

      bset.set(0);
    }

    NodeIndex alloc(NodeIndex prev, NodeIndex node) {
      while (node >= lnk.size()-1) // 最後のnodeは特別なので-1する
	resize_link();
      return alloc_impl(prev, node);
    }

    NodeIndex x_check(const CodeList& codes) {
      NodeIndex prev = 0;
      NodeIndex cur  = lnk[prev].next;

      for(; cur!=0 && cur <= codes.front(); prev=cur, cur=lnk[cur].next); 
      for(; cur!=0; prev=cur, cur=lnk[cur].next) {
	NodeIndex x = cur-codes.front();
	if(!bset.test(x) && can_allocate(cur, codes, x)) {
	  bset.set(x);
	  
	  for(unsigned i=0; i<codes.size(); i++) 
	    prev=alloc(prev, x+codes[i]);

	  return x;
	}
      }
      assert(false);
    }

  private:
    bool can_allocate(NodeIndex cur, const CodeList& codes, NodeIndex base) const {
      for(unsigned i=1; i < codes.size(); i++) {
	NodeIndex node = base+codes[i];
	if(node < lnk.size() && lnk[node].next==0)
	  return false;
      }
      return true;
    }

    void resize_link(NodeIndex hint=0) {
      assert(lnk.size() < 0xFFFFFF-KeyStream::MAX_CODE);

      lnk.back().next=lnk.size();
      NodeIndex end = std::max(hint,lnk.size()*2); 
      
      for(NodeIndex i=lnk.size(); i < end; i++) 
	lnk.push_back(Link(i+1));
      lnk.back().next=0; // 末尾 -> 先頭
    }

    NodeIndex alloc_impl(NodeIndex prev, NodeIndex node) {
      assert(lnk[node].next!=0);
      while(lnk[prev].next < node)
	prev = lnk[prev].next;
      lnk[prev].next = lnk[node].next;
      lnk[node].next = 0;
      return prev;
    }

  private:
    LinkList lnk;
    Bitset   bset;
  };
}
#endif
