#ifndef DOAR_DOUBLE_ARRAY_H
#define DOAR_DOUBLE_ARRAY_H

#include "types.h"
#include "key_stream.h"
#include "node_list.h"
#include "dynamic_allocator.h"

namespace Doar {
  class DoubleArray {
    typedef DynamicAllocator Allocator;
    
  public:
    DoubleArray () { init(); col_cnt=0;}

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
	} else if(cd == chck[next_idx]) {
	  if(next.is_leaf()) {
	    if(in.eos() || key_exists(in, next))
	      return false;
	    
	    tail_collision_case(in, next_idx);
	    return true;
	  } 
	  idx = next_idx;
	} else {
	  col_cnt++;
	  collision_case(in, cd, idx);
	  return true;
	}
      }
    }

    Node search(const char* key) const {
      Node node=root_node();
      KeyStream in(key); 
      for(Code cd=in.read();; cd=in.read()) {
	const NodeIndex idx = node.next_index(cd);
	if(idx >= base.size() || idx >= chck.size())
	  return Node::INVALID;

	node = base[idx];
	if(cd==chck[idx]) 
	  if(!node.is_leaf())                      continue;
	  else if(in.eos() || key_exists(in,node)) return node;
	return Node::INVALID;
      } 
    }    

    Node root_node() const { return base[0]; }
    
  private:
    void init() {
      base.clear();
      base.resize(0xFFFF);
      chck.clear();
      chck.resize(0xFFFF);

      base[0].set_base(1);
      
      chck.clear();
      tind.clear();
      tail.clear();
      tail += '\0';
      tail.reserve(0xFFFF);
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
      rlt.tail_idx = tin.rest()-tail.data();
      rlt.codes[0] = c;
      rlt.codes[1] = tc;
    }

    NodeIndex set_node(Code code, NodeIndex prev, NodeIndex x_node) {
      NodeIndex next = x_node+code;
      base.at(prev).set_base(x_node);
      chck.at(next) = code;
      return next;
    }

    void insert_tail(KeyStream in, NodeIndex idx) {
      base.at(idx).set_tail_index(tind.size());
      if(in.eos()) {
	tind.push_back(tail.size()-1); // 便宜的に、一つ前の'\0'を指すようにする
	return;
      }
      
      tind.push_back(tail.size());
      tail += in.rest();
      tail += '\0';
    }

    void set_check_and_insert_tail(KeyStream in, Code code, NodeIndex idx) {
      chck.at(idx) = code;
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
      NodeIndex end = std::min(beg+CODE_LIMIT, chck.size()-1);
      
      for(NodeIndex i=beg; i < end; i++)
	if((i-beg) == chck[i])
	  result.push_back(i-beg);
    }
    
    void modify_nodes(NodeIndex idx, NodeIndex new_base, const CodeList& codes) {
      assert(base[idx].valid()); // XXX: for dev

      NodeIndex old_base = base[idx].base();
      alloca.x_free(old_base);

      base[idx].set_base(new_base);
      
      CodeList::const_iterator itr = codes.begin();
      
      for(; itr != codes.end(); ++itr) {
	NodeIndex old_node = old_base + *itr;
	NodeIndex new_node = new_base + *itr;
	
	base.at(new_node).data = base.at(old_node).data;
	chck.at(new_node)      = chck.at(old_node);
	base[old_node] = Node::INVALID; 
	chck[old_node] = VACANT_CODE;
	
	alloca.free(old_node);
      }
    }
    

  private:
    Allocator alloca;
    BaseList base;
    ChckList chck;
    TindList tind;
    Tail     tail;     
    
  public:
    int col_cnt;
  };
}

#endif
