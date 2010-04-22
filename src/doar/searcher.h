#ifndef DOAR_SEARCHER_H
#define DOAR_SEARCHER_H

#include "types.h"
#include "key_stream.h"
#include "node.h"
#include "mmap_t.h"
#include "loader.h"

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
	return INVALID;
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

    // CustomKeyStream require following method
    //  - read(): read one code from stream. If end of stream reached, must return TERMINAL_CODE.
    template<typename CustomKeyStream, typename Callback>
    void each_common_prefix(CustomKeyStream& in, const Callback& fn) const {
      Node node = root_node();

      for(Code cd=in.read();; cd=in.read()){
	const NodeIndex terminal_idx = node.next_index(TERMINAL_CODE);
	if(chck[terminal_idx].trans_by(TERMINAL_CODE)) {
	  fn(in, base[terminal_idx].id());
	  if(cd==TERMINAL_CODE)
	    return;
	}
	
	const NodeIndex idx = node.next_index(cd);
	node = base[idx];
	if(chck[idx].trans_by(cd))
	  if(!node.is_leaf())                         continue;
	  else if(key_including(in, node)) fn(in, node.id());
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

  private:
    bool key_exists(const KeyStream in, const Base n) const {
      return strcmp(in.rest(), tail+tind[n.id()])==0;
    }

    bool key_including(const KeyStream in, const Base n, uint32& key_offset) const {
      const char* ptr=tail+tind[n.id()];
      std::size_t len = strlen(ptr);
      key_offset += static_cast<uint32>(len) + 1;
      return strncmp(in.rest(), ptr, len)==0;
    }

    template<typename CustomKeyStream>
    bool key_including(CustomKeyStream& in, const Base n) const {
      const char* p=tail+tind[n.id()];
      for(; *p != '\0'; ++p)
	if(char2code(*p) != in.read())
	  return false;
      return true;
    }
  protected:
    const Node INVALID;
    
  private:
    const Base*      const base; // BASE array
    const Chck*      const chck; // CHECK array
    const TailIndex* const tind; // TAIL index array
    const char*      const tail; // TAIL array
  };

  class Searcher {
  public:
    Searcher(const char* filepath) 
      : doar(filepath), 
	srch(doar.base, doar.chck, doar.tind, doar.tail),
	status(doar.status) {}

    operator    bool() const { return doar; }
    std::size_t size() const { return doar.size(); }

    Node search(const char* key) const { return srch.search(key); }

    template<typename Callback>
    void each_common_prefix(const char* key, const Callback& fn) const 
    { srch.each_common_prefix(key,fn); }

    template<typename Callback>
    void each_common_prefix(const char* key, Node root_node, const Callback& fn) const 
    { srch.each_common_prefix(key,root_node,fn); }

    template<typename CustomKeyStream, typename Callback>
    void each_common_prefix(CustomKeyStream& in, const Callback& fn) const 
    { srch.each_common_prefix(in, fn); }

    template<typename Callback>
    void each_child(Node parent, const Callback& fn) const 
    { return srch.each_child(parent,fn); }

    Node root_node() const { return srch.root_node(); }

  private:
    const Loader doar;
    const SearcherBase srch;

  public:
    const int status;
  };

  class OnMemorySearcher {
  public:
    OnMemorySearcher(const Base* base, const Chck* chck, unsigned node_size,
		     const TailIndex* tind, unsigned tind_size, 
		     const char* tail, unsigned tail_size) 
      : data(base,chck,node_size,tind,tind_size,tail,tail_size), 
	srch(data.base,data.chck,data.tind,data.tail) {}
    
    std::size_t size() const { return data.tind_size; }
    
    Node search(const char* key) const { return srch.search(key); }

    template<typename Callback>
    void each_common_prefix(const char* key, const Callback& fn) const 
    { srch.each_common_prefix(key,fn); }

    template<typename Callback>
    void each_common_prefix(const char* key, Node root_node, const Callback& fn) const 
    { srch.each_common_prefix(key,root_node,fn); }

    template<typename CustomKeyStream, typename Callback>
    void each_common_prefix(CustomKeyStream& in, const Callback& fn) const 
    { srch.each_common_prefix(in, fn); }

    template<typename Callback>
    void each_child(Node parent, const Callback& fn) const 
    { return srch.each_child(parent,fn); }

    Node root_node() const { return srch.root_node(); }   

  private:
    struct Data {
      Data(const Base* base, const Chck* chck, unsigned node_size,
	   const TailIndex* tind, unsigned tind_size, 
	   const char* tail, unsigned tail_size) 
	: node_size(node_size), tind_size(tind_size), tail_size(tail_size) {
	this->base = new Base[node_size];
	this->chck = new Chck[node_size];
	this->tind = new TailIndex[tind_size];
	this->tail = new char[tail_size];

	memcpy(this->base, base, node_size*sizeof(Base));
	memcpy(this->chck, chck, node_size*sizeof(Chck));
	memcpy(this->tind, tind, tind_size*sizeof(TailIndex));
	memcpy(this->tail, tail, tail_size*sizeof(char));
      }
      ~Data() {
	delete [] base;
	delete [] chck;
	delete [] tind;
	delete [] tail;
      }

      unsigned node_size; // base and chck size
      unsigned tind_size; // tind size
      unsigned tail_size; // tail size

      Base*      base;
      Chck*      chck;
      TailIndex* tind;
      char*      tail;
    };
  private:
    const Data data;
    const SearcherBase srch;
  };
}
#endif
