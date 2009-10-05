#ifndef MMAP_T_H
#define MMAP_T_H

#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

struct mmap_t{
  mmap_t(const char* path, bool write_mode=false, int flags=MAP_FILE|MAP_PRIVATE){
    int OPEN_MODE=O_RDONLY;
    int PROT = PROT_READ;
    if(write_mode) {
      OPEN_MODE=O_RDWR;
      PROT |= PROT_WRITE;
    }
    
    int f = open(path, OPEN_MODE);
    struct stat statbuf;
    fstat(f, &statbuf);
    ptr = mmap(0, statbuf.st_size, PROT, flags, f, 0);
    size=statbuf.st_size;
    close(f);  
  }

  ~mmap_t() {
    munmap(ptr, size);
  }

  operator bool () const 
  { return reinterpret_cast<int>(ptr)!=-1; }

  size_t size;
  void *ptr;
};

#endif
