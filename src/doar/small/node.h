#ifndef DOAR_SMALLNODE_H
#define DOAR_SMALLNODE_H

#include <stdexcept>
#include <cassert>

namespace Doar{
  namespace Small {
  struct Node {
    // 0b10000000 00000000 00000000 00000000  : TAIL配列へのindexかどうかのフラグ
    // 0b01111111 11111111 11111111 00000000  : BASEの値
    // 0b00000000 00000000 00000000 11111111  : CHECKの値

    Node() : data(0) {}
    Node(unsigned d) : data(d) {}

    Code chck() const  { return data&0xFF; }    
    NodeIndex base() const  { return data>>8; }
    NodeIndex next_index(Code cd) const  { return base()+cd; }
    TailIndex tail_index() const  { return (data&0x7FFFFFFF)>>8; }

    bool is_terminal() const { return data&0x80000000; }
    bool is_vacant() const { return chck()==0; }
    
    void set_base(NodeIndex idx) { check_base(idx); data = chck() | (idx<<8); }
    void set_chck(Code cd) { check_chck(cd); data = (data&0xFFFFFF00) | (cd&0xFF); }
    void set_tail_index(TailIndex idx) { check_tail(idx); data = chck() | (idx<<8) | 0x80000000; }

    unsigned data;
    
  private:
    void check_base(NodeIndex idx) const {  
      if((idx+KeyStream::MAX_CODE)&0xFF800000)
	throw std::overflow_error("Base array overflowed");
    }
    void check_tail(TailIndex idx) const { assert(!(idx&0xFF800000)); }
    void check_chck(Code cd) const { assert(!(cd&0xFFFFFF00)); }
  };
  }
}
#endif
