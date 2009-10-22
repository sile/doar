#ifndef DOAR_BUILDER_H
#define DOAR_BUILDER_H

#include "types.h"
#include "key_stream.h"
#include "static_allocator.h"
#include "shrink_tail.h"
#include "node.h"
#include "../util/gettime.h"
#include <stdexcept>

namespace Doar {
  class Builder {
    typedef StaticAllocator Allocator;
    
  public:
    bool build(const char* filepath) {
      Allocator alloca;
      KeyStreamList keys(filepath);
      if(!keys)
	return false;

      // sort and uniquness check
      for(std::size_t i=0; i < keys.size()-1; i++) 
	if(strcmp(keys[i].rest(), keys[i+1].rest()) >= 0)
	  throw std::invalid_argument(std::string("File is unsorted(or has duplicate lines): ")+filepath);

      init(keys.size());
      build_impl(keys,alloca,0,keys.size(),0);
      return true;
    }
    bool build(const char** strs, uint32 str_count) {
      Allocator alloca;
      KeyStreamList keys(strs, str_count);

      // XXX:
      // sort and uniquness check
      for(std::size_t i=0; i < keys.size()-1; i++) 
	if(strcmp(keys[i].rest(), keys[i+1].rest()) >= 0)
	  throw std::invalid_argument(std::string("Input keys are unsorted or no unique"));

      init(keys.size());
      
      build_impl(keys,alloca,0,keys.size(),0);
      return true;
    }

    // TODO: friend?
    void build(const BaseList& src_base, const ChckList& src_chck, const TindList& src_tind, const Tail& src_tail) {
      Allocator alloca;
      init(src_tind.size());
      tind=src_tind;
      tail=src_tail;
      build_impl(src_base,src_chck,alloca,src_base[0],0);
    }

    bool save(const char* filepath, bool do_shrink_tail=true) {
      FILE *f;
      if((f=fopen(filepath,"wb"))==NULL)
	return false;
      
      if(do_shrink_tail)
	ShrinkTail(tail,tind).shrink();
      
      // get size
      Header h={static_cast<uint32>(chck.size()),static_cast<uint32>(tind.size()),static_cast<uint32>(tail.size())};

      for(; h.node_size > 0 && !chck[h.node_size-1].in_use(); h.node_size--);
      h.node_size += CODE_LIMIT; // NOTE: append padding area (for safe no check access on search time)

      base.resize(h.node_size);
      chck.resize(h.node_size);

      fwrite(&h, sizeof(Header), 1, f);
      fwrite(tind.data(), sizeof(uint32), h.tind_size, f);
      fwrite(base.data(), sizeof(Base), h.node_size, f);
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
    void build_impl(const BaseList& src_base, const ChckList& src_chck, Allocator& alloca, Base old_root, NodeIndex new_root_idx) {
      if(old_root.is_leaf()) {
	// TODO:
	insert_tail(new_root_idx, old_root.id());
	return;
      }

      CodeList cs;
      {
	NodeIndex beg = old_root.base();
	for(Code c=0; c < CODE_LIMIT; c++)
	  if(src_chck[beg+c].trans_by(c))
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
      base.at(node).set_id(tind_idx);
    }

    void insert_tail(KeyStream in, NodeIndex node) {
      base.at(node).set_id(static_cast<uint32>(tind.size()));
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
