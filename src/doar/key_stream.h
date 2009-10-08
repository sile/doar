#ifndef DOAR_KEY_STREAM_H
#define DOAR_KEY_STREAM_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <vector>

namespace Doar {
  // keyは'\0'終端文字列で、値が0xFFの文字を含むことはできない
  class KeyStream {
  public:
    KeyStream(){}
    KeyStream(const char* key) : cur(key) {}
    
    // TODO: Codeはなくす。遷移する時(get_next_index)だけ、+1するようにする
    // chckの初期値は0xFF
    Code read() { return static_cast<unsigned char>(*cur++); }
    const char* rest() const { return cur; }
    bool eos() const { return cur[-1]=='\0'; }

  private:
    const char* cur;
  };

  class KeyStreamList {
  public:
    KeyStreamList(const char* filepath) 
      : buf(NULL),valid(false) {
      int f = open(filepath, O_RDONLY);
      if(f==-1)
	return;
      valid=true;
      
      struct stat statbuf;
      fstat(f, &statbuf);
      
      buf = new char[statbuf.st_size+1];
      ::read(f, buf, statbuf.st_size);
      close(f);
      
      buf[statbuf.st_size]='\0';
      
      init(buf,buf+statbuf.st_size-1);
    }
    KeyStreamList(const char** strs, unsigned str_count) 
      : buf(NULL),valid(true) {
      words.resize(str_count);
      for(unsigned i=0; i < str_count; i++)
	words[i] = KeyStream(strs[i]);
    }
    ~KeyStreamList() { delete [] buf; }

    std::size_t size() const { return words.size(); }
    KeyStream& operator[](std::size_t idx) { return words[idx]; }

    operator bool() const { return valid; }
    
  private:
    void init(char* cur, const char* end) {
      words.push_back(cur);
      for(cur=strchr(cur,'\n'); cur; cur=strchr(cur,'\n')) {
	*cur = '\0';
	cur++;
	if(cur < end)
	  words.push_back(KeyStream(cur));
      }
    }
    
  private:
    char* buf;
    std::vector<KeyStream> words;
    
    bool valid;
  };
}

#endif
