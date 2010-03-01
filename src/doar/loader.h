#ifndef DOAR_LOADER_H
#define DOAR_LOADER_H

#include "types.h"
#include "node.h"
#include "mmap_t.h"

namespace Doar {
  class Loader {
  public:
    Loader(const char* filepath) : mm(filepath), status(init(filepath)) {}

    operator    bool() const { return status==Status::OK; }
    std::size_t size() const { return h.tind_size; }

    const Header& header() const { return h; }
    
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
    
  protected:
    const mmap_t mm;
    Header h;

  public:
    const int status;

    const Base*      base; // BASE array
    const Chck*      chck; // CHECK array
    const TailIndex* tind; // TAIL index array
    const char*      tail; // TAIL array
  };
}

#endif
