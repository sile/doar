#ifndef DOAR_DOUBLE_ARRAY_H
#define DOAR_DOUBLE_ARRAY_H

#include "types.h"
#include "key_stream.h"
#include "node.h"
#include "dynamic_allocator.h"
#include "builder.h"
#include "searcher.h"
#include "shrink_tail.h"

namespace Doar {
  using std::min; // NOTE: for Windows macro
    
  class DoubleArray {
    typedef DynamicAllocator Allocator;

  public:
    DoubleArray () { init(); }

    /**********/
    /* insert */
    bool insert(const char* key) {
      KeyStream in(key);
      NodeIndex idx=0;

      for(Code cd=in.read();; cd=in.read()) {
	const NodeIndex next_idx = base[idx].next_index(cd);
	const Node next = base.at(next_idx);
	
	if(alloca.is_free(next_idx)) {
	  alloca.alloc(next_idx);
	  set_check_and_insert_tail(in,cd,next_idx);
	  return true;
	} else if(chck[next_idx].verify(cd)) {
	  if(next.is_leaf()) {
	    if(in.eos() || key_exists(in, next))
	      return false;
	    
	    tail_collision_case(in, next_idx);
	    return true;
	  } 
	  idx = next_idx;
	} else {
	  collision_case(in, cd, idx);
	  return true;
	}
      }
    }

    /**********/
    /* search */
    Node root_node() const { return srch().root_node(); }
    Node search(const char* key) const { return srch().search(key); }
    Node search(const char* key, Node& root_node) const { return srch().search(key,root_node); }
    
    template<typename Callback>
    void common_prefix_search(const char* key, Node root_node, const Callback& fn) const
    { return srch().common_prefix_search(key,root_node,fn); }

    template<typename Callback>
    void common_prefix_search(const char* key, const Callback& fn) const
    { return srch().common_prefix_search(key,fn); }

    template<typename Callback>
    void children(Node parent, const Callback& fn) const { return srch().children(parent,fn); }
      
    
    /*****************/
    /* save and load */
    bool save(const char* path) {
      ShrinkTail(tail,tind).shrink();

      Builder bld;
      bld.build(base,chck,tind,tail);
      return bld.save(path,false);
    }

    void clear() { init(); }

    bool load(const char* path) {
      init();

      mmap_t mm(path);
      if(!mm) 
	return false;

      // TODO: format check      
      header h;
      memcpy(&h,mm.ptr,sizeof(header));

      void* beg=static_cast<char*>(mm.ptr)+sizeof(header);
      beg = assign(tind, static_cast<uint32 *>(beg), h.tind_size); 
      beg = assign(base, static_cast<Node*>(beg), h.node_size);
      beg = assign(chck, static_cast<Chck*>(beg), h.node_size);
      beg = assign(tail, static_cast<char*>(beg), h.tail_size);

      alloca.restore_condition(base.data(),chck.data(),h.node_size);
      return true;
    }

    /*********/
    /* other */
    std::size_t size() const { return tind.size(); }

  private:
    // TODO: 
    void init() {
      base.clear();
      base.resize(0x10000);
      chck.clear();
      chck.resize(0x10000);

      base[0].set_base(1);  // XXX: ???
      
      tind.clear();
      tail.clear();
      tail += '\0';
      tail.reserve(0x10000);
    }

    const SearcherBase srch() const {
      return SearcherBase(base.data(),chck.data(),tind.data(),tail.data());
    }

    bool key_exists(const KeyStream in, const Node n) const {
      return strcmp(in.rest(), tail.data()+tind[n.tail_index()])==0;
    }

    // struct: Tail_Collision_Case RESULT
    struct TCC_RESULT {
      NodeIndex last_node;
      TailIndex tail_idx;
      CodeList  codes;
      
      TCC_RESULT() : codes(2) {}
    };
    
    void tail_collision_case(KeyStream in, NodeIndex idx) {
      TCC_RESULT rlt;
      NodeIndex old_tail_idx = base[idx].tail_index();

      set_common_node(in,idx,rlt);

      NodeIndex x_node = alloca.x_check_two(rlt.codes[0],rlt.codes[1]);
      idx = set_node(rlt.codes[1], rlt.last_node, x_node);
      base[idx].set_tail_index(old_tail_idx);
      tind[old_tail_idx] = rlt.tail_idx;
      
      insert_tail(in, set_node(rlt.codes[0], rlt.last_node, x_node));
    }
    
    void set_common_node(KeyStream& in, NodeIndex idx,TCC_RESULT& rlt) {
      KeyStream tin(tail.data()+tind[base[idx].tail_index()]);
      
      Code c  = in.read();
      Code tc = tin.read();
      for(; c == tc; c=in.read(), tc=tin.read())
	idx=set_node(c, idx, alloca.x_check_one(c));
      
      rlt.last_node = idx;
      rlt.tail_idx = tin.eos() ? 0 : static_cast<TailIndex>(tin.rest()-tail.data());
      rlt.codes[0] = c;
      rlt.codes[1] = tc;
    }

    NodeIndex set_node(Code code, NodeIndex prev, NodeIndex x_node) {
      NodeIndex next = x_node+code;
      base.at(prev).set_base(x_node);
      chck.at(next).set_chck(code);
      return next;
    }

    void insert_tail(KeyStream in, NodeIndex idx) {
      base.at(idx).set_tail_index(static_cast<TailIndex>(tind.size()));
      if(in.eos()) {
	tind.push_back(0); // NOTE: tail[0]=='\0'
	return;
      }
      
      tind.push_back(static_cast<TailIndex>(tail.size()));
      tail += in.rest();
      tail += '\0';
    }

    void set_check_and_insert_tail(KeyStream in, Code code, NodeIndex idx) {
      chck.at(idx).set_chck(code);
      insert_tail(in, idx);
    }

    void collision_case(KeyStream in, Code code, NodeIndex idx) {
      CodeList codes;
      
      correspond_codes(base[idx], codes);
      
      codes.push_back(code);
      NodeIndex x = alloca.x_check(codes);
      codes.pop_back();

      modify_nodes(idx, x, codes);
      
      set_check_and_insert_tail(in, code, base[idx].next_index(code));
    }

    void correspond_codes(Node node, CodeList& result) const {
      NodeIndex beg = node.base(); 
      NodeIndex end = min(beg+CODE_LIMIT, static_cast<NodeIndex>(chck.size()-1));
      
      for(NodeIndex i=beg; i < end; i++)
	if(chck[i].verify(i-beg))
	  result.push_back(i-beg);
    }
    
    void modify_nodes(NodeIndex idx, NodeIndex new_base, const CodeList& codes) {
      NodeIndex old_base = base[idx].base();
      alloca.x_free(old_base);

      base[idx].set_base(new_base);
      
      CodeList::const_iterator itr = codes.begin();
      
      for(; itr != codes.end(); ++itr) {
	NodeIndex old_node = old_base + *itr;
	NodeIndex new_node = new_base + *itr;
	
	base.shift(old_node, new_node);
	chck.shift(old_node, new_node);
	alloca.free(old_node);
      }
    }

    template <class T>
    typename T::value_type* assign(T& array, typename T::value_type* beg, uint32 size) {
      typename T::value_type* end = beg+size;
      array.assign(beg,end);
      return end;
    }

  private:
    Allocator alloca;
    BaseList base;
    ChckList chck;
    TindList tind;
    Tail     tail;     
  };
}

#endif
