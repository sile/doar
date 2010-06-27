#ifndef DOAR_STATIC_ALLOCATOR_H
#define DOAR_STATIC_ALLOCATOR_H

#include "types.h"
#include <vector>
#include <cassert>

namespace Doar {
  class StaticAllocator {
  private:
    /***********************/
    /* auxiliary structure */
    struct ForwardLink {
      ForwardLink(NodeIndex i) : idx(i), next(NULL) {}
      
      NodeIndex append(NodeIndex beg_idx, uint32 num) {
	ForwardLink* cur=this;
	while(cur->next) cur=cur->next;
	
	for(NodeIndex i=beg_idx; i < beg_idx+num; i++)
	  cur = cur->next = new ForwardLink(i);
	return beg_idx+num-1;
      }
      
      void remove_next() {
	assert(next);
	ForwardLink* tmp=next; next=next->next; delete tmp;
      }
      
      void delete_all_tail() {
	for(ForwardLink* tmp=next; next; tmp=next) {
	  next=next->next;
	  delete tmp;
	}
      }
      
      NodeIndex    idx;
      ForwardLink* next;
    };
    
  public:
    typedef std::vector<bool> Bitset;
    static const uint32 PER_LINK_SIZE=0x10000;
    static const uint32 TRY_ALLOC_THRESHOLD=0x80;
    
  public:
    // NOTE: invariant: bset.size() % PER_LINK_SIZE == 0
    StaticAllocator() : root(0),bset(PER_LINK_SIZE) {
	last_idx=root.append(1,PER_LINK_SIZE);

	// NOTE: For data compatibility with class DynamicAllocator, 
	//       the following code ensure that base[CODE_LIMIT] is always unused.
	ForwardLink* tmp=&root;
	for(;; tmp=tmp->next)
	  if(tmp->next->idx==CODE_LIMIT) {
	    tmp->remove_next();
	    break;
	  }

	bset[0].flip();
    }
    ~StaticAllocator() {
      root.delete_all_tail();
    }
    
    // NOTE: This method assume that variable codes is already sorted.
    NodeIndex x_check(const CodeList& codes, ForwardLink* prev=NULL) {
      if(!prev) prev=&root;
      if(!prev->next) {
	last_idx = prev->append(last_idx+1, PER_LINK_SIZE);
	if(last_idx >= bset.size())
	  bset.resize(bset.size()*2);
      }
      
      ForwardLink *cur = prev->next;
      Code min_cd = codes.front();
      
      for(; cur->idx <= min_cd; prev=cur, cur=cur->next) assert(cur);
      for(uint32 cnt=0; cur->next; prev=cur,cur=cur->next,cnt++) {
	NodeIndex x = cur->idx - min_cd;
	if(!bset[x] && can_allocate(cur, codes, x)) {
	  if(cnt > TRY_ALLOC_THRESHOLD)
	    root.remove_next(); // reduce too redundant loop
	  
	  bset[x].flip();
	  alloc(prev,codes,x);
	  return x;
	}
      }
      
      return x_check(codes, cur);
    }
    
  private:
    bool can_allocate(ForwardLink* cur, const CodeList& codes, NodeIndex x) const {
      cur=cur->next;
      for(uint32 i=1;i < codes.size(); i++) {
	for(; cur && cur->idx < x+codes[i]; cur=cur->next);
	if(!cur || cur->idx > x+codes[i])
	  return false;
      }
      return true;
    }
    
    void alloc(ForwardLink* prev, const CodeList& codes, NodeIndex x) {
      prev->remove_next();
      for(uint32 i=1; i < codes.size(); i++) {
	for(; prev->next->idx < x+codes[i]; prev=prev->next);
	assert(prev->next->idx == x+codes[i]);
	prev->remove_next();
      }    
    }
    
  private:
    ForwardLink root;
    NodeIndex   last_idx;
    Bitset      bset;
  };
}

#endif
