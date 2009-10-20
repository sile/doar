#ifndef DOAR_DYNAMIC_ALLOCATOR_H
#define DOAR_DYNAMIC_ALLOCATOR_H

#include "types.h"
#include "node.h"
#include <vector>
#include <cassert>

namespace Doar {
  using std::max; // NOTE: for Windows macro

  class DynamicAllocator {
    struct Link {
      Link(uint32 p, uint32 n) : prev(p), next(n) {}

      uint32 prev;
      uint32 next;
    };
    typedef std::vector<Link> LinkList;
    typedef std::vector<bool> Bset;
    static const uint32 TRY_ALLOC_THRESHOLD=0x80;

  public:
    DynamicAllocator() { init(); }
    
    void clear() { 
      lnk.clear();
      bset.clear();
    }

    void init(uint32 init_size=0x10000) {
      assert(init_size > 1);
      
      lnk.clear();
      lnk.push_back(Link(0,0));
      resize_link(init_size);
      
      bset.clear();
      bset.resize(init_size);
      bset[1].flip();
      
      beg_idx=CODE_LIMIT;
    }
    
    void restore_condition(const Node* base, const Chck* chck, uint32 node_size) {
      init(static_cast<uint32>(node_size*1.5));

      for(NodeIndex i=0; i < node_size; i++) {
	if(!base[i].is_leaf())  // XXX: INVALID is leaf という条件に依存している。 TODO: ドキュメント化
	  bset[base[i].base()].flip();
	
	if(chck[i].vacant()==false)
	  alloc(i);
      }
    }

    void x_free(NodeIndex idx) {
      assert(bset[idx]);
      bset[idx].flip(); 
    }

    void alloc(NodeIndex node) {
      while (node >= lnk.size()-1) // 最後のnodeは特別なので-1する
	resize_link();

      if(node==beg_idx)
	beg_idx=lnk[beg_idx].prev;

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
      assert(beg_idx>=CODE_LIMIT);
      
      NodeIndex cur = lnk[beg_idx].next;
      for(uint32 cnt=0;;  cur=lnk[cur].next, cnt++) {
	assert(cur);
	
	NodeIndex x = cur-codes.front();
	if(!bset[x] && can_allocate(codes, x)) {
	  bset[x].flip();
	  if(cnt > TRY_ALLOC_THRESHOLD)
	    beg_idx=lnk[beg_idx].next;
	  
	  for(std::size_t i=0; i<codes.size(); i++)
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
      for(std::size_t i=1; i < codes.size(); i++)
	if(x+codes[i] < lnk.size() && lnk[x+codes[i]].next==0)
	  return false;
      return true;
    }

    void resize_link(NodeIndex hint=0) {
      lnk.back().next=static_cast<uint32>(lnk.size());
      NodeIndex end = max(hint,static_cast<NodeIndex>(lnk.size()*2));
      bset.resize(end);

      for(NodeIndex i=static_cast<NodeIndex>(lnk.size()); i < end; i++) 
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
