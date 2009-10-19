#ifndef DOAR_SEARCHER_H
#define DOAR_SEARCHER_H

#include <cassert>
#include "types.h"
#include "key_stream.h"
#include "node.h"
#include "../util/mmap_t.h"

namespace Doar {
  // TODO: comment for assumption
  class SearcherBase {
  public:
    SearcherBase(const Node* base, const unsigned char* chck, 
		 const unsigned* tind, const char* tail) 
      : base(base),chck(chck),tind(tind),tail(tail) {}

   Node search(const char* key) const {
      Node node=root_node();
      KeyStream in(key); 
      for(Code cd=in.read();; cd=in.read()) {
	const NodeIndex idx = node.next_index(cd);
	node = base[idx];
	
	if(cd==chck[idx])
	  if(!node.is_leaf())                      continue;
	  else if(in.eos() || key_exists(in,node)) return node;
	return Node::INVALID;
      } 
    }
    
    Node search(const char* key, Node& root_node) const {
      if(root_node.is_leaf())
	return Node::INVALID;
      
      Node node = root_node;
      KeyStream in(key); 
      for(Code cd=in.read();; cd=in.read(), root_node=node) {
	const NodeIndex idx = node.next_index(cd);
	node = base[idx];
	
	if(cd==chck[idx])
	  if(!node.is_leaf())           continue;
	  else if(in.eos())             return node;
	  else if(key_exists(in, node)) return root_node=node;
	return Node::INVALID;
      } 
    }

    template<typename Callback>
    void common_prefix_search(const char* key, const Callback& fn) const {
      Node node = root_node();
      unsigned offset=0;
      KeyStream in(key);
      
      for(Code cd=in.read();; cd=in.read(), offset++) {
	const NodeIndex terminal_idx = node.next_index(TERMINAL_CODE);
	if(TERMINAL_CODE==chck[terminal_idx]) {
	  fn(key,offset,base[terminal_idx].id());
	  if(cd==TERMINAL_CODE)
	    return;
	  }
	
	const NodeIndex idx = node.next_index(cd);
	node = base[idx];
	if(cd==chck[idx])
	  if(!node.is_leaf())                    continue;
	    else if(key_including(in,node,offset)) fn(key,offset,node.id());
	return;
      }      
    }   

    template<typename Callback>
    void common_prefix_search(const char* key, Node root_node, const Callback& fn) const {
      if(root_node.is_leaf())
	return;

      Node node = root_node;
      unsigned offset=0;
      KeyStream in(key);
      
      for(Code cd=in.read();; cd=in.read(), offset++) {
	const NodeIndex terminal_idx = node.next_index(TERMINAL_CODE);
	if(TERMINAL_CODE==chck[terminal_idx]) {
	  fn(key,offset,base[terminal_idx].id(),node);
	  if(cd==TERMINAL_CODE)
	    return;
	  }
	
	const NodeIndex idx = node.next_index(cd);
	node = base[idx];
	if(cd==chck[idx])
	  if(!node.is_leaf())                    continue;
	  else if(key_including(in,node,offset)) fn(key,offset,node.id(),node);
	return;
      }      
    }  

    template<typename Callback>
    void children(Node parent, const Callback& fn) const {
      if(parent.is_leaf())
	return;
      
      for(Code cd=0; cd < CODE_LIMIT; cd++)
	if(cd==chck[parent.next_index(cd)]) {
	  Node node = base[parent.next_index(cd)];
	  fn(static_cast<char>(cd), node, node.is_leaf() ? tail+tind[node.tail_index()] : NULL);
	}
    }

    Node root_node() const { return base[0]; }

  protected:
    SearcherBase() : base(NULL), chck(NULL), tind(NULL), tail(NULL) {}
    
    bool key_exists(const KeyStream in, const Node n) const {
      return strcmp(in.rest(), tail+tind[n.tail_index()])==0;
    }

    bool key_including(const KeyStream in, const Node n, unsigned& key_offset) const {
      const char* ptr=tail+tind[n.tail_index()];
      unsigned len = strlen(ptr);
      key_offset += len + 1;
      return strncmp(in.rest(), ptr, len)==0;
    }
   
  protected:
    const Node*          base; // BASE array
    const unsigned char* chck; // CHECK array
    const unsigned*      tind; // TAIL index array
    const char*          tail; // TAIL array
  };

  class Searcher : public SearcherBase {
  public:
    Searcher(const char* filepath) : mm(filepath) {
      if(!mm) 
	return;

      // TODO: format check
      memcpy(&h,mm.ptr,sizeof(header));
      tind = reinterpret_cast<const unsigned*>(static_cast<char*>(mm.ptr)+sizeof(header));
      base = reinterpret_cast<const Node*>(tind+h.tind_size);
      chck = reinterpret_cast<const unsigned char*>(base+h.node_size);
      tail = reinterpret_cast<const char*>(chck + h.node_size);
    }

    operator bool() const { return (bool)mm; }
    unsigned size() const { return h.tind_size; }

  private:
    const mmap_t mm;
    header h;
  };
}
#endif
