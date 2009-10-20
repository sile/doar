#ifndef READ_LINE_H
#define READ_LINE_H

#include <vector>
#include <cstdio>

class ReadLine {
public:
  ReadLine(const char* filepath) : buf(NULL),index(0) {
    FILE* f;
    if((f=fopen(filepath,"rb"))==NULL)
      return;
    
    fseek(f,0,SEEK_END);
    long file_size = ftell(f); // NOTE# file size limit: 2^(sizeof(long)-1)
    fseek(f,0,SEEK_SET);
    
    if(file_size != -1){
      buf = new char[file_size+1];
      fread(buf, sizeof(char), file_size, f);
      buf[file_size]='\0';
      init(buf,buf+file_size-1);
    }
    
    fclose(f);
  }
  ~ReadLine() {
    delete [] buf;
  }

  operator bool() const { 
    return buf!=NULL;
  }

  void reset() {
    index = 0;
  }

  const char* read() {
    if(index == lines.size())
      return NULL;

    return lines[index++];
  }

  std::size_t size() const { return lines.size(); }

private:
  void init(char* cur, const char* end) {
    lines.push_back(cur);
    for(cur=strchr(cur,'\n'); cur; cur=strchr(cur,'\n')) {
      *cur = '\0';
      cur++;
      if(cur < end)
	lines.push_back(cur);
    }
  }

private:
  char* buf;
  std::size_t index;
  std::vector<const char*> lines;
};

#endif
