#ifndef DOAR_SHRINK_TAIL_H
#define DOAR_SHRINK_TAIL_H

#include <vector>
#include "types.h"

namespace Doar {
  class ShrinkTail {
  public:
    ShrinkTail(Tail& tail_, TindList& tind_) 
      : tail(tail_), tind(tind_) {}

    void shrink() {
      std::vector<ShrinkRecord> terminal_indices;
      std::vector<ShrinkRecord> tmps;
      terminal_indices.reserve(tind.size());

      for(unsigned i=0; i < tind.size(); i++)
	terminal_indices.push_back(ShrinkRecord(i,tail.data()+tind[i]));
      
      std::sort(terminal_indices.begin(), terminal_indices.end(), tail_gt);
      
      //
      Tail new_tail;
      new_tail.reserve(tail.size()/2);
      new_tail += '\0';

      for(unsigned i=0; i < terminal_indices.size(); i++) {
	const ShrinkRecord& p = terminal_indices[i];

	TailIndex tail_idx = new_tail.size();
	if(i>0 && can_share(terminal_indices[i-1], p)) {
	  tail_idx -= p.tail_len+1; // +1は、末尾の'\0'分
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
      ShrinkRecord(unsigned i,const char* t) 
        : tind_idx(i),tail(t),tail_len(strlen(t)) {}
      unsigned    tind_idx;
      const char* tail;
      int         tail_len;
    };

    // lftがrgtを包含しているか?
    bool can_share(const ShrinkRecord& lft, const ShrinkRecord& rgt) const {
      const char* lp = lft.tail;
      const char* rp = rgt.tail;

      for(int li=lft.tail_len-1, ri=rgt.tail_len-1;; li--, ri--) {
	if(ri < 0)                return true;  // MEMO: 先にriをチェックするのは重要
	else if(li < 0)           return false;
	else if(lp[li] != rp[ri]) return false;
      }
    }
    
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
