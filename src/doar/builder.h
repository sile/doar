#ifndef DOAR_BUILDER_H
#define DOAR_BUILDER_H

#include "types.h"
#include "key_stream.h"
#include "node_list.h"
#include "node_allocator.h"

namespace Doar {
  class Builder {
  public:
    bool build(const char* filepath) {
      NodeAllocator alloca;
      KeyStreamList keys(filepath);
      if(!keys)
	return false;
      // TODO: sort check
      //

      init();
      build_impl(keys,alloca,0,keys.size(),0);

      return true;
    }
    bool build(const char** strs, unsigned str_count) {
      NodeAllocator alloca;
      KeyStreamList keys(strs, str_count);
      // TODO: sort check

      init();
      build_impl(keys,alloca,0,keys.size(),0);
      return true;
    }

    bool save(const char* filepath) {
      // [format]
      // header{
      //  node-size: 4byte
      //  tail-size: 4byte
      //  tind-size: 4byte
      // }
      // base: node-size*4byte
      // chck: node-size
      // tind: tind-size*4byte
      // tail: tail-size

      int f = creat(filepath, 0666);
      if(f==-1)
	return false;

      shrink_tail(); 

      header h={0,tind.size(),tail.size()};
      
      for(int i=chck.size()-1; i>=0; i--)
	if(chck[i]!=VACANT_CODE) {
	  h.node_size=i+1;
	  break;
	}

      // 範囲外アクセスを防ぐために調整する
      for(int i=0; i < h.node_size; i++) {
	Node n = base[i];
	if(chck[i]!=VACANT_CODE && !n.is_leaf())
	  if(n.base()+CODE_LIMIT-1 >= h.node_size)
	    h.node_size = n.base()+CODE_LIMIT-1;
      }
      base.resize(h.node_size);
      chck.resize(h.node_size);

      write(f,&h,sizeof(header));
      write(f,tind.data(),h.tind_size*sizeof(unsigned));
      write(f,base.data(),h.node_size*sizeof(Node));
      write(f,chck.data(),h.node_size);
      write(f,tail.data(),h.tail_size);
      close(f);
      return true;
    }

    unsigned size() const { return tind.size(); }
    
  private:
    void build_impl(KeyStreamList& keys, NodeAllocator& alloca, unsigned beg, unsigned end, NodeIndex root_idx) {
      if(end-beg==1) {
	insert_tail(keys[beg],root_idx);
	return;
      }

      std::vector<unsigned> end_list;
      CodeList cs;
      Code prev=VACANT_CODE;

      // 
      for(unsigned i=beg; i < end; i++) {
	Code cur = keys[i].read();
	if(prev != cur) {
	  cs.push_back(cur);
	  prev = cur;

	  end_list.push_back(i);
	}
      }
      end_list.push_back(end);
      
      //
      NodeIndex x = alloca.x_check(cs);
      
      for(unsigned i=0; i<cs.size(); i++) 
	build_impl(keys, alloca,end_list[i],end_list[i+1], set_node(cs[i],root_idx,x));
    }

    NodeIndex set_node(Code code, NodeIndex prev, NodeIndex x_node) {
      NodeIndex next = x_node+code;
      base.at(prev).set_base(x_node);
      chck.at(next) = code;
      return next;
    }

    void insert_tail(KeyStream in, NodeIndex node) {
      base.at(node).set_tail_index(tind.size());
      if(in.eos()) {
	tind.push_back(tail.size()-1); // 便宜的に、一つ前の'\0'を指すようにする
	return;
      }

      tind.push_back(tail.size());
      tail += in.rest();
      tail += '\0';
    }

    struct ShrinkRecord {
      ShrinkRecord(unsigned i,const char* t) 
        : tind_idx(i),tail(t),tail_len(strlen(t)) {}
      unsigned    tind_idx;
      const char* tail;
      int         tail_len;
    };
    void shrink_tail() {
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

    void init() {
      base.clear();
      chck.clear();
      tind.clear();
      tail.clear();
      tail += '\0';
      tail.reserve(0xFFFF);
    }

  private:
    BaseList base;
    ChckList chck;
    TindList tind;
    Tail     tail; 
  };
}
#endif
