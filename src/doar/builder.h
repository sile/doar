#ifndef DOAR_BUILDER_H
#define DOAR_BUILDER_H

#include "types.h"
#include "key_stream.h"
#include "node_list.h"
#include "static_allocator.h"
#include "shrink_tail.h"

// XXX:
#include "double_array.h"

namespace Doar {
  class Builder {
    typedef StaticAllocator Allocator;
    
  public:
    bool build(const char* filepath) {
      Allocator alloca;
      KeyStreamList keys(filepath);
      if(!keys)
	return false;
      // TODO: sort check
      //

      init(keys.size());
      build_impl(keys,alloca,0,keys.size(),0);
      return true;
    }
    bool build(const char** strs, unsigned str_count) {
      Allocator alloca;
      KeyStreamList keys(strs, str_count);
      // TODO: sort check

      init(keys.size());
      
      build_impl(keys,alloca,0,keys.size(),0);
      return true;
    }
    bool build(const DoubleArray& trie) {
      // TOOD: buildする前に、trieを軽量化したい -> build後に使えなくする? -> name
      Allocator alloca;
      init(trie.tind.size());
      tind=trie.tind;
      tail=trie.tail;
      build_impl(trie,alloca,trie.base[0],0);
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
      
      ShrinkTail(tail,tind).shrink();

      header h={0,tind.size(),tail.size()};
      
      for(int i=chck.size()-1; i>=0; i--)
	if(chck[i]!=VACANT_CODE) {
	  h.node_size=i+1;
	  break;
	}

      // 範囲外アクセスを防ぐために調整する
      for(unsigned i=0; i < h.node_size; i++) {
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
    void build_impl(KeyStreamList& keys, Allocator& alloca, unsigned beg, unsigned end, NodeIndex root_idx) {
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

    // XXX: for dev
    void build_impl(const DoubleArray& trie, Allocator& alloca, Node old_root, NodeIndex new_root_idx) {
      if(old_root.is_leaf()) {
	// TODO:
	insert_tail(new_root_idx, old_root.tail_index());
	return;
      }
      
      CodeList cs;
      trie.correspond_codes(old_root,cs);
      
      NodeIndex x = alloca.x_check(cs);
      for(unsigned i=0; i < cs.size(); i++)
	build_impl(trie, alloca, trie.base[old_root.next_index(cs[i])], set_node(cs[i],new_root_idx,x));
    }

    NodeIndex set_node(Code code, NodeIndex prev, NodeIndex x_node) {
      NodeIndex next = x_node+code;
      base.at(prev).set_base(x_node);
      chck.at(next) = code;
      return next;
    }

    // XXX:
    void insert_tail(NodeIndex node, unsigned tind_idx) {
      base.at(node).set_tail_index(tind_idx);
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

    void init(unsigned key_num) {
      base.clear();
      base.resize(key_num*2);
      chck.clear();
      chck.resize(key_num*2);

      tind.clear();
      tind.reserve(key_num);
      tail.clear();
      tail += '\0';
      tail.reserve(key_num);
    }

  private:
    BaseList base;
    ChckList chck;
    TindList tind;
    Tail     tail; 
  };
}
#endif
