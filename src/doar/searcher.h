#ifndef DOAR_SEARCHER_H
#define DOAR_SEARCHER_H

#include <cassert>
#include "types.h"
#include "key_stream.h"
#include "node.h"
#include "../util/mmap_t.h"

namespace Doar {
  class Searcher {
  public:
    Searcher(const char* filepath) : mm(filepath) {
      if(!mm) 
	return;

      // TODO: format check
      memcpy(&h,mm.ptr,sizeof(header));
      base = reinterpret_cast<const Node*>(static_cast<char*>(mm.ptr)+sizeof(header));
      chck = reinterpret_cast<const unsigned char*>(base+h.node_size);
      tind = reinterpret_cast<const unsigned*>(chck+h.node_size);
      tail = reinterpret_cast<const char*>(tind + h.tind_size);
    }

    operator bool() const { return (bool)mm; }
    
    Node search(const char* key, Node node) const {
      assert(!node.is_terminal());
      
      KeyStream in(key); 
      for(Code cd=in.read();; cd=in.read()) {
	const NodeIndex idx = node.next_index(cd);
	node = base[idx];
	
	if(cd==chck[idx])
	  if(!node.is_terminal())
	    continue;
	  else if (key_exists(in, node))
	    return node;
	return Node::INVALID;
      } 
    }
    Node search(const char* key) const { return search(key,root_node()); }
    Node search(char* key, unsigned len) const { return search(key,len,root_node()); }
    Node search(char* key, unsigned len, Node root_node) const{
      char c = key[len];              key[len]='\0';
      Node rlt=search(key,root_node); key[len]=c;
      return rlt;
    }

    Node search_non_greedy(const char* key, unsigned& key_offset, Node& root_node) const {
      assert(!root_node.is_terminal());

      Node node = root_node;
      root_node = Node::INVALID;

      bool first=true;
      KeyStream in(key+key_offset);
      for(Code cd=in.read();; cd=in.read(), key_offset++, first=false) {
	if(!first && cd != 1) {
	  const NodeIndex other_idx = node.next_index(1);
	  if(1==chck[other_idx]) {
	    root_node=node;
	    return base[other_idx];
	  }
	}

	const NodeIndex idx = node.next_index(cd);
	node = base[idx];
	
	if(cd==chck[idx])
	  if(!node.is_terminal())
	    continue;
	  else if (key_including(in, node, key_offset))
	    return node;
	return Node::INVALID;
      }
    }

    unsigned children(Node parent, NodeList& result) const {
      if(parent.is_terminal())
	return 0;
      
      const NodeIndex base_idx = parent.base();
      for(Code cd=1; cd <= KeyStream::MAX_CODE; cd++)
	if(cd==chck[base_idx+cd])
	  result.push_back(base[base_idx+cd]);
      return result.size();
    } 

    Node root_node() const { return base[0]; }
    unsigned size() const { return h.tind_size; }

  private:
    bool key_exists(const KeyStream in, const Node n) const {
      return in.eos() || strcmp(in.rest(), tail+tind[n.tail_index()])==0;
    }
    bool key_including(const KeyStream in, const Node n, unsigned& key_offset) const {
      if(in.eos())
	return true;
      const char* ptr=tail+tind[n.tail_index()];
      unsigned len = strlen(ptr);
      key_offset += len + 1;
      return strncmp(in.rest(), ptr, len)==0;
    }
    
  private:
    const mmap_t mm;
    header h;
    const Node*          base;
    const unsigned char* chck;
    const unsigned*      tind;
    const char*          tail;
  };
}
#endif
