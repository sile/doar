#ifndef DOAR_DYNAMIC_ALLOCATOR_H
#define DOAR_DYNAMIC_ALLOCATOR_H

#include "types.h"
#include <vector>
#include <assert.h>

namespace Doar {
  class DynamicAllocator {
    struct Link {
      Link(unsigned p, unsigned n) : prev(p), next(n) {}

      unsigned prev;
      unsigned next;
    };
    typedef std::vector<Link> LinkList;
    typedef vector<bool> Bset;
    static const int TRY_ALLOC_THRESHOLD=0x80;

  public:
    DynamicAllocator() {
      lnk.push_back(Link(0,0));
      resize_link(0xFFFF);
      
      bset.resize(0x10000);
      bset[1].flip();
      
      beg_idx=CODE_LIMIT;
    }

    void x_free(NodeIndex idx) {
      assert(bset[idx]);
      bset[idx].flip(); 
    }

    void alloc(NodeIndex node) {
      while (node >= lnk.size()-1) // 最後のnodeは特別なので-1する
	resize_link();
      alloc_impl(node);
    }

    bool is_free(NodeIndex idx) const {
      // MEMO:
      return idx!=CODE_LIMIT && lnk[idx].next!=0;
    }

    void free(NodeIndex idx) {
      NodeIndex cur=lnk[idx].prev;

      for(; lnk[cur].next==0; cur=lnk[cur].prev);
      for(; idx > cur; cur=lnk[cur].next) assert(cur);
      assert(idx < cur);

      NodeIndex prev = lnk[cur].prev;
      
      lnk[cur].prev = lnk[prev].next = idx;
      lnk[idx].prev = prev;
      lnk[idx].next = cur;
    }
    
    // XXX: Below code assume that CODES is already sorted.
    NodeIndex x_check(const CodeList& codes) {
      for(; lnk[beg_idx].next==0; beg_idx=lnk[beg_idx].prev);
      assert(beg_idx>=CODE_LIMIT);
      
      NodeIndex cur = lnk[beg_idx].next;
      for(unsigned cnt=0;;  cur=lnk[cur].next, cnt++) {
	assert(cur);
	
	NodeIndex x = cur-codes.front();
	if(!bset[x] && can_allocate(codes, x)) {
	  bset[x].flip();
	  if(cnt > TRY_ALLOC_THRESHOLD)
	    beg_idx=lnk[beg_idx].next;
	  
	  for(unsigned i=0; i<codes.size(); i++)
	    alloc(x+codes[i]);
	
	  return x;
	}
      }
    }
    
    NodeIndex x_check_two(Code cd1, Code cd2) {
      CodeList codes(2);
      if(cd1<cd2) codes[0]=cd1, codes[1]=cd2;
      else        codes[1]=cd1, codes[0]=cd2;
      return x_check(codes);
    }

    NodeIndex x_check_one(Code cd) {
      NodeIndex cur = lnk[CODE_LIMIT].next;
      NodeIndex x = cur-cd;
      for(; bset[x]; cur=lnk[cur].next, x=cur-cd) assert(cur);
      bset[x].flip();
      alloc(cur);
      return x;
    }    
    
  private:
    bool can_allocate(const CodeList& codes, NodeIndex x) const {
      for(unsigned i=1; i < codes.size(); i++)
	if(x+codes[i] < lnk.size() && lnk[x+codes[i]].next==0)
	  return false;
      return true;
    }

    void resize_link(NodeIndex hint=0) {
      lnk.back().next=lnk.size();
      NodeIndex end = std::max(hint,lnk.size()*2);
      bset.resize(end);

      for(NodeIndex i=lnk.size(); i < end; i++) 
	lnk.push_back(Link(i-1,i+1));
      lnk.back().next=0; // 末尾 -> 先頭
    }

    void alloc_impl(NodeIndex node) {
      assert(lnk[node].next!=0);

      lnk[lnk[node].prev].next = lnk[node].next;
      lnk[lnk[node].next].prev = lnk[node].prev;
      lnk[node].next=0;
    }

  private:
    LinkList lnk;
    Bset bset;

    NodeIndex beg_idx;
  };
}

#endif