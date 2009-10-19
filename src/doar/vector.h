#ifndef DOAR_VECTOR_H

#include <vector>

namespace Doar {
  template<typename T, unsigned PADDING=0>
  class Vector : public std::vector<T> {
    typedef std::vector<T> orig;

  public:
    Vector() : orig() {}
    Vector(typename orig::size_type n) : orig(n) {}

    T& at(typename orig::size_type idx) {
      for(; idx+PADDING >= orig::size(); orig::resize(orig::size()*2));
      return orig::operator[](idx);
    }

#if defined(WIN32) || defined(WIN64)
    // XXX: for dev
    T* data() const {
      if(orig::empty())
	return NULL;
      return &orig::operator[0];
    }
#endif
  };
}

#endif
