#ifndef DOAR_SEARCHER_H
#define DOAR_SEARCHER_H

#include "types.h"
#include "key_stream.h"
#include "node.h"
#include "mmap_t.h"

namespace Doar {
  class SearcherBase {
  public:
    SearcherBase(const Base* base, const Chck* chck, const TailIndex* tind, const char* tail) 
      : base(base),chck(chck),tind(tind),tail(tail) {}

    // NOTE: key is NULL terminated string.  And it can't contain '0xFF' character.
    Node search(const char* key) const {
      Node node=root_node();
      KeyStream in(key); 
      for(Code cd=in.read();; cd=in.read()) {
	const NodeIndex idx = node.next_index(cd);
	node = base[idx];
	
	if(chck[idx].trans_by(cd))
	  if(!node.is_leaf())                      continue;
	  else if(in.eos() || key_exists(in,node)) return node;
	return Node::INVALID;
      }
    }
    
    Node search(const char* key, Node& root_node) const {
      if(root_node.is_leaf())
	return Node::INVALID;
      
      Base node = root_node;
      KeyStream in(key); 
      for(Code cd=in.read();; cd=in.read(), root_node=node) {
	const NodeIndex idx = node.next_index(cd);
	node = base[idx];
	
	if(chck[idx].trans_by(cd))
	  if(!node.is_leaf())           continue;
	  else if(in.eos())             return node;
	  else if(key_exists(in, node)) return root_node=node;
	return Node::INVALID;
      } 
    }

    template<typename Callback>
    void each_common_prefix(const char* key, const Callback& fn) const {
      Node node = root_node();
      uint32 offset=0;
      KeyStream in(key);
      
      for(Code cd=in.read();; cd=in.read(), offset++) {
	const NodeIndex terminal_idx = node.next_index(TERMINAL_CODE);
	if(chck[terminal_idx].trans_by(TERMINAL_CODE)) {
	  fn(key,offset,base[terminal_idx].id());
	  if(cd==TERMINAL_CODE)
	    return;
	}
	
	const NodeIndex idx = node.next_index(cd);
	node = base[idx];
	if(chck[idx].trans_by(cd))
	  if(!node.is_leaf())                    continue;
	    else if(key_including(in,node,offset)) fn(key,offset,node.id());
	return;
      }      
    }   

    template<typename Callback>
    void each_common_prefix(const char* key, Node root_node, const Callback& fn) const {
      if(root_node.is_leaf())
	return;

      Node node = root_node;
      uint32 offset=0;
      KeyStream in(key);
      
      for(Code cd=in.read();; cd=in.read(), offset++) {
	const NodeIndex terminal_idx = node.next_index(TERMINAL_CODE);
	if(chck[terminal_idx].trans_by(TERMINAL_CODE)){
	  fn(key,offset,base[terminal_idx].id(),node);
	  if(cd==TERMINAL_CODE)
	    return;
	  }
	
	const NodeIndex idx = node.next_index(cd);
	node = base[idx];
	if(chck[idx].trans_by(cd))
	  if(!node.is_leaf())                    continue;
	  else if(key_including(in,node,offset)) fn(key,offset,node.id(),node);
	return;
      }      
    }  

    template<typename Callback>
    void each_child(Node parent, const Callback& fn) const {
      if(parent.is_leaf())
	return;
      
      for(Code cd=0; cd < CODE_LIMIT; cd++)
	if(chck[parent.next_index(cd)].trans_by(cd)) {
	  Node node = base[parent.next_index(cd)];
	  fn(static_cast<char>(cd), node, node.is_leaf() ? tail+tind[node.id()] : NULL);
	}
    }

    Node root_node() const { return base[0]; }

  protected:
    SearcherBase() : base(NULL), chck(NULL), tind(NULL), tail(NULL) {}
    
    bool key_exists(const KeyStream in, const Base n) const {
      return strcmp(in.rest(), tail+tind[n.id()])==0;
    }

    bool key_including(const KeyStream in, const Base n, uint32& key_offset) const {
      const char* ptr=tail+tind[n.id()];
      std::size_t len = strlen(ptr);
      key_offset += static_cast<uint32>(len) + 1;
      return strncmp(in.rest(), ptr, len)==0;
    }
   
  protected:
    const Base*      base; // BASE array
    const Chck*      chck; // CHECK array
    const TailIndex* tind; // TAIL index array
    const char*      tail; // TAIL array
  };

  class Searcher : public SearcherBase {
  public:
    Searcher(const char* filepath) : mm(filepath), status(init(filepath)) {}


    operator    bool() const { return status==Status::OK; }
    std::size_t size() const { return h.tind_size; }
    
  private:
    int init(const char* filepath) {
      if(!mm)
	return Status::OPEN_FILE_FAILED;
      memcpy(&h,mm.ptr,sizeof(Header));

      // data validation
      {
	if(strncmp(h.magic_s, MAGIC_STRING, sizeof(h.magic_s))!=0)
	  return Status::INVALID_FILE_FORMAT;
	
	unsigned total_size = 
	  sizeof(Header) + 
	  sizeof(TailIndex)*h.tind_size +
	  sizeof(Base)*h.node_size + 
	  sizeof(Chck)*h.node_size +
	  sizeof(char)*h.tail_size;
	if(mm.size != total_size)
	  return Status::FILE_IS_CORRUPTED;
      }

      tind = reinterpret_cast<const TailIndex*>(static_cast<char*>(mm.ptr)+sizeof(Header));
      base = reinterpret_cast<const Base*>(tind+h.tind_size);
      chck = reinterpret_cast<const Chck*>(base+h.node_size);
      tail = reinterpret_cast<const char*>(chck + h.node_size);
      return Status::OK;
    }
    
  private:
    const mmap_t mm;
    Header h;

  public:
    const int status;
  };
}
#endif
