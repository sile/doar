#ifndef MMAP_T_H
#define MMAP_T_H

#if defined(WIN32) || defined(WIN64)
// Windows
#include <windows.h>

struct mmap_t {
  // NOTE: Variable 'flags' is ignored under Windows.
  mmap_t(const char* path, bool write_mode=false, int flags=0) : ptr(NULL) {
    DWORD fileAccess = write_mode ? GENERIC_READ|GENERIC_WRITE : GENERIC_READ;
    DWORD flProtect  = write_mode ? PAGE_READWRITE : PAGE_READONLY;
    DWORD mapAccess  = write_mode ? FILE_MAP_WRITE : FILE_MAP_READ;
    
    HANDLE hFile = CreateFile(path, fileAccess, 0, 0, OPEN_EXISTING, 0, 0);
    if(hFile == INVALID_HANDLE_VALUE)
      return;
    size = GetFileSize(hFile, NULL);
    
    HANDLE hMap = CreateFileMapping(hFile, 0, flProtect, 0, 0, NULL);
    if(hMap != NULL)
      ptr = MapViewOfFile(hMap, mapAccess, 0, 0, 0);
    
    CloseHandle(hMap);
    CloseHandle(hFile);
  }

  ~mmap_t() { UnmapViewOfFile(ptr); }

  operator bool () const { return ptr!=NULL; }

  size_t size;
  void *ptr;
};

#else
// NOTE: Assume Unix compatible (POSIX conformance?) environment.

#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

struct mmap_t {
  mmap_t(const char* path, bool write_mode=false, int flags=MAP_PRIVATE){
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

  ~mmap_t() { munmap(ptr, size); }
  
  operator bool () const { return ptr!=reinterpret_cast<void*>(-1); }
  
  size_t size;
  void *ptr;
};

#endif // #if defined(WIN32) || defined(WIN64)
#endif // #ifndef MMAP_T_H
