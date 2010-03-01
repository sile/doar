#ifndef DOAR_MERGER_H
#define DOAR_MERGER_H

#include "types.h"
#include "loader.h"
#include "static_allocator.h"
#include "shrink_tail.h"
#include "key_stream.h"
#include "builder.h"
#include <algorithm>

namespace Doar {
  namespace {
    class Loader2 : public Loader {
    public:
      Loader2(const char* filepath, unsigned id_offset) 
	: Loader(filepath), id_offset(id_offset) {}

      const unsigned id_offset;

      // XXX: name , platform check
      void free() {
	munmap(ptr+sizeof(Header)+sizeof(TailIndex)*h.tind_size+sizeof(Base)*h.node_size+sizeof(Chck)*h.node_size, sizeof(char)*h.tail_size);
	munmap(ptr,sizeof(Header)+sizeof(TailIndex)*h.tind_size);
      }
    };
  }

  class Merger {
    typedef StaticAllocator Allocator;

  public:
    int merge(const char* filepath1, const char* filepath2) {
      const Loader2 doar1(filepath1, 0);
      if(!doar1)
	return doar1.status;

      const Loader2 doar2(filepath2, doar1.size());
      if(!doar2)
	return doar2.status;
      
      init(doar1.size()+doar2.size());

      for(std::size_t i=0; i < doar1.header().tind_size; i++) 
	tind.push_back(doar1.tind[i]);
      for(std::size_t i=0; i < doar2.header().tind_size; i++) 
	tind.push_back(doar2.tind[i]==0 ? 0 : doar2.tind[i]+doar1.header().tail_size);
      
      tail.append(doar1.tail, doar1.header().tail_size);
      tail.append(doar2.tail, doar2.header().tail_size);
      doar1.free();
      doar2.free();

      Allocator alloca;
      merge_doarXdoar(doar1,doar1.base[0], 
		      doar2,doar2.base[0],
		      alloca, 0);
      
      return Status::OK;
    }

    bool save(const char* filepath, bool do_shrink_tail=true) {
      return Builder::save(base,chck,tind,tail,filepath,do_shrink_tail);
    }

    std::size_t size() const { return tind.size(); }
    
  private:
    void merge_doarXdoar(const Loader2& doar1, Base root1, 
			 const Loader2& doar2, Base root2,
			 Allocator& alloca, NodeIndex new_root_idx) {
      if(root1.is_leaf()) {
	merge_tindXdoar(root1.id() + doar1.id_offset,
			doar2, root2,
			alloca, new_root_idx);
      } else if(root2.is_leaf()) {
	merge_tindXdoar(root2.id() + doar2.id_offset,
			doar1, root1,
			alloca, new_root_idx);
      } else {
	CodeList cs;
	NodeIndex beg1 = root1.base();
	NodeIndex beg2 = root2.base();

	for(Code c=0; c < CODE_LIMIT; c++) 
	  if(doar1.chck[beg1+c].trans_by(c) ||
	     doar2.chck[beg2+c].trans_by(c))
	    cs.push_back(c);

	NodeIndex x = alloca.x_check(cs);
	for(std::size_t i=0; i < cs.size(); i++)
	  if(doar2.chck[beg2+cs[i]].trans_by(cs[i])==false)
	    merge_doar(doar1, doar1.base[root1.next_index(cs[i])],
		       alloca, set_node(cs[i],new_root_idx,x));
	
	  else if(doar1.chck[beg1+cs[i]].trans_by(cs[i])==false) 
	    merge_doar(doar2, doar2.base[root2.next_index(cs[i])],
		       alloca, set_node(cs[i],new_root_idx,x));
	  else
	    merge_doarXdoar(doar1, doar1.base[root1.next_index(cs[i])], 
			    doar2, doar2.base[root2.next_index(cs[i])], 
			    alloca, set_node(cs[i],new_root_idx,x));
      }
    }

    void merge_tindXdoar(unsigned id, const Loader2& doar, Base root,
			 Allocator& alloca, NodeIndex new_root_idx) {
      if(root.is_leaf()) {
	merge_tindXtind(id, root.id()+doar.id_offset, alloca, new_root_idx);
	return;
      }
      
      Code arc=static_cast<unsigned char>(tail[tind[id]]);
      if(arc != '\0')
	tind[id]++;
      else
	tind[id]=0;

      CodeList cs;
      NodeIndex beg = root.base();
      for(Code c=0; c < CODE_LIMIT; c++)
	if(doar.chck[beg+c].trans_by(c) || c==arc)
	  cs.push_back(c);

      NodeIndex x = alloca.x_check(cs);
      for(std::size_t i=0; i < cs.size(); i++) 
	if(cs[i] != arc) 
	  merge_doar(doar, doar.base[root.next_index(cs[i])],
		     alloca, set_node(cs[i],new_root_idx,x));
	else if(doar.chck[beg+cs[i]].trans_by(cs[i])==false) 
	  base.at(set_node(cs[i],new_root_idx,x)).set_id(id);
	else
	  merge_tindXdoar(id,
			  doar, doar.base[root.next_index(cs[i])],
			  alloca, set_node(cs[i],new_root_idx,x));
    }

    void merge_tindXtind(unsigned id1, unsigned id2, Allocator& alloca, NodeIndex new_root_idx) {
      KeyStream k1(tail.data()+tind[id1]);
      KeyStream k2(tail.data()+tind[id2]);

      if(strcmp(k1.rest(), k2.rest())==0) {
	// k1==k2
	// omit id2
	base.at(new_root_idx).set_id(id1);
	tind[id2]=0;
	return;
      }

      CodeList cs(1);
      Code c1 = k1.read();
      Code c2 = k2.read();
      for(; c1 == c2; c1=k1.read(), c2=k2.read()) {
	cs[0]=c1;
	new_root_idx=set_node(c1, new_root_idx, alloca.x_check(cs));
      }
      tind[id1] = k1.eos() ? 0 : static_cast<TailIndex>(k1.rest()-tail.data());
      tind[id2] = k2.eos() ? 0 : static_cast<TailIndex>(k2.rest()-tail.data());
      
      cs[0]=c1;
      cs.push_back(c2);
      if(cs[0] > cs[1])
	std::swap(cs[0],cs[1]);
      
      NodeIndex x = alloca.x_check(cs);
      base.at(set_node(c1,new_root_idx,x)).set_id(id1);
      base.at(set_node(c2,new_root_idx,x)).set_id(id2);
    }

    void merge_doar(const Loader2& doar, Base root, Allocator& alloca, NodeIndex new_root_idx) {
      if(root.is_leaf()) {
	base.at(new_root_idx).set_id(root.id()+doar.id_offset);
	return;
      }

      CodeList cs;
      NodeIndex beg = root.base();
      for(Code c=0; c < CODE_LIMIT; c++)
	if(doar.chck[beg+c].trans_by(c))
	  cs.push_back(c);

      NodeIndex x = alloca.x_check(cs);
      for(std::size_t i=0; i < cs.size(); i++)
	merge_doar(doar, doar.base[root.next_index(cs[i])],
		   alloca, set_node(cs[i],new_root_idx,x));
    }
    
  private:
    NodeIndex set_node(Code code, NodeIndex prev, NodeIndex x_node) {
      NodeIndex next = x_node+code;
      base.at(prev).set_base(x_node);
      chck.at(next).set_chck(code);
      return next;
    }

    // XXX: There is room for change.
    void init(std::size_t key_num) {
      base.clear();
      base.resize(key_num*2);
      chck.clear();
      chck.resize(key_num*2);

      tind.clear();
      tind.reserve(key_num);
      tail.clear();
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
