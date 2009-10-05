#ifndef READ_LINE_H
#define READ_LINE_H

#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

class ReadLine {
public:
  ReadLine(const char* filepath) : buf(NULL),index(0) {
    int f = open(filepath, O_RDONLY);
    if(f==-1)
      return;
    
    struct stat statbuf;
    fstat(f, &statbuf);

    buf = new char[statbuf.st_size+1];
    ::read(f, buf, statbuf.st_size);
    close(f);

    buf[statbuf.st_size]='\0';

    init(buf,buf+statbuf.st_size-1);
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

  unsigned size() const { return lines.size(); }

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
