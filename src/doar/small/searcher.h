#ifndef DOAR_SMALLSEARCHER_H
#define DOAR_SMALLSEARCHER_H

#include "types.h"
#include "key_stream.h"
#include "node.h"
#include "../util/mmap_t.h"

namespace Doar {
  namespace Small {
  class Searcher {
  public:
    static const ID NOT_FOUND = static_cast<ID>(-1);

    struct Range {
      ID begin;
      ID end;
    };
    
    struct LOG_ELEMENT {
      LOG_ELEMENT(unsigned k, ID i) : key_pos(k), id(i) {}
      unsigned key_pos;
      ID  id;
    };
    typedef std::vector<LOG_ELEMENT> Log;

  public:
    Searcher(const char* filepath) : mm(filepath) {
      if(!mm) 
	return;

      // TODO: format check
      memcpy(&h,mm.ptr,sizeof(header));
      nodes = reinterpret_cast<const Node*>(static_cast<char*>(mm.ptr)+sizeof(header));
      tind = reinterpret_cast<const unsigned*>(nodes+h.node_size);
      tail = reinterpret_cast<const char*>(tind + h.tind_size);
    }

    operator bool() const { return (bool)mm; }
    
    ID search(const char* key) const {
      KeyStream in(key); 
      
      Node node=nodes[0];
      for(Code cd=in.read();; cd=in.read()) {
	const NodeIndex idx = node.next_index(cd);
	const Node next = nodes[idx];
	
	if(cd==next.chck()){
	  if(!next.is_terminal()){
	    node=next;
	    continue;
	  }
	  
	  if(key_exists(in, next))
	    return next.tail_index();
	}
	return NOT_FOUND;
      }     
    }   

    ID search(const char* key, NodeIndex& node_idx) const {
      KeyStream in(key); 
      
      Node node=nodes[node_idx];
      for(Code cd=in.read();; cd=in.read()) {
	const NodeIndex idx = node.next_index(cd);
	const Node next = nodes[idx];
	
	if(cd==next.chck()){
	  if(!next.is_terminal()){
	    node_idx = idx;
	    node=next;
	    continue;
	  }
	  
	  if(key_exists(in, next))
	    return next.tail_index();
	}
	return NOT_FOUND;
      }     
    }   
    
    ID search(const char* key, Log& log, NodeIndex start_node_idx=0) const {
      KeyStream in(key); 
      
      unsigned key_pos=0;
      Node node=nodes[start_node_idx];
      for(Code cd=in.read();; cd=in.read(), key_pos++) {
	const NodeIndex idx = node.next_index(cd);
	const Node next = nodes[idx];
	
	
	{
	  const Node end_node = nodes[node.next_index(1)]; 
	  if(1==end_node.chck())
	    log.push_back(LOG_ELEMENT(key_pos, end_node.tail_index()));
	}
	
	if(cd==next.chck()){
	  if(!next.is_terminal()){
	    node=next;
	    continue;
	  }
	  
	  unsigned len;
	  if(key_including(in, next, len)) {
	    if(!in.eos())
	      log.push_back(LOG_ELEMENT(key_pos+len+1, next.tail_index()));
	    return next.tail_index();
	  }
	}
	return NOT_FOUND;
      }     
    }
    
    bool prefix_search(const char* key, Range& range, NodeIndex start_node_idx=0) const {
      KeyStream in(key); 
      
      NodeIndex idx=start_node_idx;
      Node node=nodes[idx];
      for(Code cd=in.read();; cd=in.read()) {
	if(cd==1) {
	  range.begin = first_id(idx);
	  if(range.begin==-1)
	    return false;
	  
	  range.end   = last_id(idx)+1;
	  return true;
	}

	idx = node.next_index(cd);
	const Node next = nodes[idx];
	
	if(cd==next.chck()){
	  if(!next.is_terminal()){
	    node=next;
	    continue;
	  }
	  
	  if(key_included(in, next)){
	    range.begin = next.tail_index();
	    range.end   = range.begin+1;
	    return true;
	  }
	}
	return false;
      }           
    }

    unsigned size() const { return h.tind_size; }

  private:
    bool key_exists(const KeyStream in, const Node n) const {
      return in.eos() || strcmp(in.rest(), tail+tind[n.tail_index()])==0;
    }
    bool key_included(const KeyStream in, const Node n) const {
      return strncmp(in.rest(), tail+tind[n.tail_index()], strlen(in.rest()))==0;
    }
    int key_including(const KeyStream in, const Node n, unsigned& len) const {
      const char* ptr=tail+tind[n.tail_index()];
      len = strlen(ptr);
      return in.eos() || strncmp(in.rest(), ptr, len)==0;
    }
    
    int first_id (NodeIndex idx) const {
      NodeIndex base = nodes[idx].base();
       
      for(Code cd=1; cd <= KeyStream::MAX_CODE; cd++) {
	const Node next = nodes[base+cd];
	
	if(cd==next.chck())
	  if(next.is_terminal())
	    return next.tail_index();
	  else
	    cd=0,base=next.base();
      }
      return -1;
    }
    int last_id (NodeIndex idx) const {
      NodeIndex base = nodes[idx].base();
      
      for(Code cd=KeyStream::MAX_CODE; cd > 0; cd--) {
	const Node next = nodes[base+cd];
	
	if(cd==next.chck())
	  if(next.is_terminal())
	    return next.tail_index();
	  else
	    cd=KeyStream::MAX_CODE+1,base=next.base();
      }
      return -1;
    }
    
  private:
    const mmap_t mm;
    header h;
    const Node* nodes;
    const unsigned* tind;
    const char* tail;
  };
  }
}
#endif
