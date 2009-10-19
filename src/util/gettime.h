#ifndef GETTIME_H
#define GETTIME_H

#if defined(WIN32) || defined(WIN64)

#include <ctime>
inline double gettime() {
  return static_cast<double>(clock())/static_cast<double>(CLOCKS_PER_SEC);
}

#else

#include <sys/time.h>
inline double gettime(){
  timeval tv;
  gettimeofday(&tv,NULL);
  return static_cast<double>(tv.tv_sec)+static_cast<double>(tv.tv_usec)/1000000.0;
}
#endif

#endif
