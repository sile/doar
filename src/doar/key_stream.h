#ifndef DOAR_KEY_STREAM_H
#define DOAR_KEY_STREAM_H

#include "types.h"
#include <vector>

namespace Doar {
  class KeyStream {
  public:
    // NOTE: key is NULL terminated string.  And it can't contain '0xFF' character.
    KeyStream(const char* key) : cur(key) {}
    KeyStream(){}
    
    Code read() { return static_cast<unsigned char>(*cur++); }
    const char* rest() const { return cur; }
    bool eos() const { return cur[-1]=='\0'; }
    
  private:
    const char* cur;
  };

  class KeyStreamList {
  public:
    // construct from file
    KeyStreamList(const char* filepath) 
      : buf(NULL),valid(false) {
      FILE* f;
      if((f=fopen(filepath,"rb"))==NULL)
	return;
    
      fseek(f,0,SEEK_END);
      long file_size = ftell(f); // NOTE: File size limit is 2^(sizeof(long)-1) byte
      fseek(f,0,SEEK_SET);
      
      if(file_size != -1){
	buf = new char[file_size+1];
	fread(buf, sizeof(char), file_size, f);
	buf[file_size]='\0';
	init(buf,buf+file_size-1);
	valid=true;
      }
    
      fclose(f);
    }
    // construct from string array
    KeyStreamList(const char** strs, uint32 str_count) 
      : words(strs,strs+str_count), buf(NULL), valid(true) {}
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
    std::vector<KeyStream> words;
    char* buf;
    bool valid;
  };
}

#endif
