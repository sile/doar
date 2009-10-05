#ifndef GETTIME_H
#define GETTIME_H

#include <sys/time.h>

inline double gettime(){
  timeval tv;
  gettimeofday(&tv,NULL);
  return static_cast<double>(tv.tv_sec)+static_cast<double>(tv.tv_usec)/1000000.0;
}
#endif
