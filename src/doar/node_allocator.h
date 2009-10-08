#ifndef DOAR_NODE_ALLOCATOR_H
#define DOAR_NODE_ALLOCATOR_H

#include "types.h"
#include <vector>
#include <assert.h>

namespace Doar {
  class NodeAllocator {
    typedef std::vector<bool> Bitset;

    struct Link {
      Link(unsigned n) : next(n) {}
      unsigned next;
    };
    typedef std::vector<Link> LinkList;

  public:
    NodeAllocator() {
      lnk.push_back(Link(0));
      resize_link(0xFFFF);

      bset.resize(0xFFFF);
      bset[0].flip();
    }

    NodeIndex alloc(NodeIndex prev, NodeIndex node) {
      while (node >= lnk.size()-1) // 最後のnodeは特別なので-1する
	resize_link();
      return alloc_impl(prev, node);
    }

    // MEMO: codesはソートされていることが前提
    NodeIndex x_check(const CodeList& codes) {
      if(codes.size()==1)
	return x_check_one(codes[0]);

      NodeIndex prev = 0;
      NodeIndex cur  = lnk[prev].next;

      for(; cur <= codes.front(); prev=cur, cur=lnk[cur].next) assert(cur); 
      for(;; prev=cur, cur=lnk[cur].next) {
	assert(cur);
	NodeIndex x = cur-codes.front();
	if(!bset[x] && can_allocate(cur, codes, x)) {
	  bset[x].flip();
	  
	  for(unsigned i=0; i<codes.size(); i++) 
	    prev=alloc(prev, x+codes[i]);

	  return x;
	}
      }
    }
    NodeIndex x_check_one(Code cd) {
      NodeIndex cur = lnk[0].next;
      NodeIndex x;
      for(; cur <= cd; cur=lnk[cur].next)                 assert(cur); 
      for(x=cur-cd; bset[x]; cur=lnk[cur].next, x=cur-cd) assert(cur);
      bset[x].flip();
      alloc(0,cur);
      return x;
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
      lnk.back().next=lnk.size();
      NodeIndex end = std::max(hint,lnk.size()*2); 
      bset.resize(end);

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
