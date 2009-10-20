#ifndef DOAR_BUILDER_H
#define DOAR_BUILDER_H

#include "types.h"
#include "key_stream.h"
#include "static_allocator.h"
#include "shrink_tail.h"
#include "node.h"
#include <cstdio>

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
    bool build(const char** strs, uint32 str_count) {
      Allocator alloca;
      KeyStreamList keys(strs, str_count);
      // TODO: sort check

      init(keys.size());
      
      build_impl(keys,alloca,0,keys.size(),0);
      return true;
    }

    // TODO: friend?
    bool build(const BaseList& src_base, const ChckList& src_chck, const TindList& src_tind, const Tail& src_tail) {
      Allocator alloca;
      init(src_tind.size());
      tind=src_tind;
      tail=src_tail;
      build_impl(src_base,src_chck,alloca,src_base[0],0);
      return true;
    }

    bool save(const char* filepath, bool do_shrink_tail=true) {
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
      
      FILE *f;
      if((f=fopen(filepath,"wb"))==NULL)
	return false;
      
      if(do_shrink_tail)
	ShrinkTail(tail,tind).shrink();
      
      // get size
      header h={0,static_cast<uint32>(tind.size()),static_cast<uint32>(tail.size())};
      
      for(int32 i=static_cast<int32>(chck.size()-1); i>=0; i--)
	if(chck[i].vacant()==false) {
	  h.node_size=i+1;
	  break;
	}

      // 範囲外アクセスを防ぐために調整する
      for(uint32 i=0; i < h.node_size; i++) {
	Node n = base[i];
	if(chck[i].vacant()==false && !n.is_leaf())
	  if(n.base()+CODE_LIMIT-1 >= h.node_size)
	    h.node_size = n.base()+CODE_LIMIT-1;
      }
      base.resize(h.node_size);
      chck.resize(h.node_size);

      fwrite(&h, sizeof(header), 1, f);
      fwrite(tind.data(), sizeof(uint32), h.tind_size, f);
      fwrite(base.data(), sizeof(Node), h.node_size, f);
      fwrite(chck.data(), sizeof(Chck), h.node_size, f);
      fwrite(tail.data(), sizeof(char), h.tail_size, f);
      fclose(f);
      return true;
    }

    std::size_t size() const { return tind.size(); }
    
  private:
    void build_impl(KeyStreamList& keys, Allocator& alloca, std::size_t beg, std::size_t end, NodeIndex root_idx) {
      if(end-beg==1) {
	insert_tail(keys[beg],root_idx);
	return;
      }

      std::vector<std::size_t> end_list;
      CodeList cs;
      Code prev=VACANT_CODE;

      // 
      for(std::size_t i=beg; i < end; i++) {
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
      for(std::size_t i=0; i<cs.size(); i++) 
	build_impl(keys, alloca,end_list[i],end_list[i+1], set_node(cs[i],root_idx,x));
    }

    // XXX: for dev
    void build_impl(const BaseList& src_base, const ChckList& src_chck, Allocator& alloca, Node old_root, NodeIndex new_root_idx) {
      if(old_root.is_leaf()) {
	// TODO:
	insert_tail(new_root_idx, old_root.tail_index());
	return;
      }

      CodeList cs;
      {
	NodeIndex beg = old_root.base();
	for(Code c=0; c < CODE_LIMIT; c++)
	  if(src_chck[beg+c].verify(c))
	    cs.push_back(c);
      }
      
      NodeIndex x = alloca.x_check(cs);
      for(std::size_t i=0; i < cs.size(); i++)
	build_impl(src_base, src_chck, alloca, src_base[old_root.next_index(cs[i])], set_node(cs[i],new_root_idx,x));
    }

    NodeIndex set_node(Code code, NodeIndex prev, NodeIndex x_node) {
      NodeIndex next = x_node+code;
      base.at(prev).set_base(x_node);
      chck.at(next).set_chck(code);
      return next;
    }

    // XXX:
    void insert_tail(NodeIndex node, uint32 tind_idx) {
      base.at(node).set_tail_index(tind_idx);
    }

    void insert_tail(KeyStream in, NodeIndex node) {
      base.at(node).set_tail_index(static_cast<TailIndex>(tind.size()));
      if(in.eos()) {
	tind.push_back(0); // NOTE: tail[0]=='\0'
	return;
      }

      tind.push_back(tail.size());
      tail += in.rest();
      tail += '\0';
    }

    void init(std::size_t key_num) {
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
    TindList tind; // TODO: replace to data array [chckに入れる?]
    Tail     tail; 
  };
}
#endif
