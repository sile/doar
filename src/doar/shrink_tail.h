#ifndef DOAR_SHRINK_TAIL_H
#define DOAR_SHRINK_TAIL_H

#include "types.h"
#include <vector>
#include <algorithm>

namespace Doar {
  class ShrinkTail {
  public:
    ShrinkTail(Tail& tail_, TindList& tind_) 
      : tail(tail_), tind(tind_) {}

    void shrink() {
      std::vector<ShrinkRecord> terminal_indices;
      terminal_indices.reserve(tind.size());

      for(uint32 i=0; i < tind.size(); i++)
	terminal_indices.push_back(ShrinkRecord(i,tail.data()+tind[i]));
      
      std::sort(terminal_indices.begin(), terminal_indices.end(), tail_gt);
      
      Tail new_tail;
      new_tail.reserve(tail.size()/2);
      new_tail += '\0';

      for(uint32 i=0; i < terminal_indices.size(); i++) {
	const ShrinkRecord& p = terminal_indices[i];
	
	TailIndex tail_idx = static_cast<TailIndex>(new_tail.size());
	if(i>0 && can_share(terminal_indices[i-1], p)) {
	  tail_idx -= p.tail_len+1; // +1 is necessary for last '\0' character
	} else {
	  new_tail += p.tail;
	  new_tail += '\0';
	}
	tind[p.tind_idx] = tail_idx;
      }
      tail = new_tail;
    }

  private:
    struct ShrinkRecord {
      ShrinkRecord(uint32 i,const char* t) 
        : tind_idx(i),tail(t),tail_len(static_cast<int>(strlen(t))) {}
      uint32      tind_idx;
      const char* tail;
      int tail_len;
    };

    // Is lft including rgt ?
    bool can_share(const ShrinkRecord& lft, const ShrinkRecord& rgt) const {
      const char* lp = lft.tail;
      const char* rp = rgt.tail;

      for(int li=lft.tail_len-1, ri=rgt.tail_len-1;; li--, ri--) {
	if(ri < 0)                return true;  // NOTE: ri must be checked before li.
	else if(li < 0)           return false;
	else if(lp[li] != rp[ri]) return false;
      }
    }
    
    // Is lft greater than rgt ?
    static bool tail_gt (const ShrinkRecord& lft, const ShrinkRecord& rgt) {
      const char* lp = lft.tail;
      const char* rp = rgt.tail;
      
      for(int li=lft.tail_len-1, ri=rgt.tail_len-1;; li--, ri--) {
	if(li < 0)               return false;
	else if(ri < 0)          return true;
	else if(lp[li] > rp[ri]) return true;
	else if(lp[li] < rp[ri]) return false;
      }
    }

  private:
    Tail     &tail;
    TindList &tind;
  };
}

#endif
