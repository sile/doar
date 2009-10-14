#ifndef DOAR_FORWARD_LINK_ALLOCATOR_H
#define DOAR_FORWARD_LINK_ALLOCATOR_H

#include "types.h"
#include <vector>
#include <assert.h>

namespace Doar {
  namespace ForwardLink {
    class Allocator {
    private:
      struct Link {
	Link(NodeIndex i) : idx(i), next(NULL) {}
	
	NodeIndex append(NodeIndex beg_idx, unsigned num) {
	  Link* cur=this;
	  while(cur->next) cur=cur->next;
	  
	  for(NodeIndex i=beg_idx; i < beg_idx+num; i++)
	    cur = cur->next = new Link(i);
	  return beg_idx+num-1;
	}
	
	void remove_next() {
	  assert(next);
	  Link* tmp=next; next=next->next; delete tmp;
	}
	
	void delete_all_tail() {
	  for(Link* tmp=next; next; tmp=next) {
	    next=next->next;
	    delete tmp;
	  }
	}
	
	NodeIndex idx;
	Link*     next;
      };
      
      typedef std::vector<bool> Bitset;
      static const int PER_LINK_SIZE=0x10000;
      static const int TRY_ALLOC_THRESHOLD=0x80;
      
    public:
      // MEMO: bset.size()はPER_LINK_SIZEの倍数である必要がある
      Allocator() : root(0),bset(PER_LINK_SIZE) {
	last_idx=root.append(1,PER_LINK_SIZE);
	bset[0].flip();
      }
      ~Allocator() {
	root.delete_all_tail();
      }
      
      // MEMO: codesはソートされていることが前提
      NodeIndex x_check(const CodeList& codes, Link* prev=NULL) {
	if(!prev) prev=&root;
	Link *cur = prev->next;
	Code min_cd = codes.front();
	
	for(; cur->idx <= min_cd; prev=cur, cur=cur->next) assert(cur);
	for(unsigned cnt=0; cur; prev=cur,cur=cur->next,cnt++) {
	  NodeIndex x = cur->idx - min_cd;
	  if(!bset[x] && can_allocate(cur, codes, x)) {
	    if(cnt>TRY_ALLOC_THRESHOLD)
	      root.remove_next(); // MEMO: 過度に冗長なループを省く
	    
	    bset[x].flip();
	    alloc(prev,codes,x);
	    return x;
	  }
	}
	last_idx = prev->append(last_idx+1, PER_LINK_SIZE);
	
	if(last_idx >= bset.size())
	  bset.resize(bset.size()*2);
	
	return x_check(codes, prev);
      }
      
    private:
      bool can_allocate(Link* cur, const CodeList& codes, NodeIndex x) const {
	cur=cur->next;
	for(unsigned i=1;i < codes.size(); i++) {
	  for(; cur && cur->idx < x+codes[i]; cur=cur->next);
	  if(!cur || cur->idx > x+codes[i])
	    return false;
	}
	return true;
      }
      
      void alloc(Link* prev, const CodeList& codes, NodeIndex x) {
	prev->remove_next();
	for(unsigned i=1; i < codes.size(); i++) {
	  for(; prev->next->idx < x+codes[i]; prev=prev->next);
	  assert(prev->next->idx == x+codes[i]);
	  prev->remove_next();
	}    
      }
      
    private:
      Link      root;
      NodeIndex last_idx;
      Bitset    bset;
    };
  }
}

#endif
