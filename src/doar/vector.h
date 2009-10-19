#ifndef DOAR_VECTOR_H

#include <vector>

namespace Doar {
  template<typename T, unsigned PADDING=0>
  class Vector : public std::vector<T> {
    typedef std::vector<T> orig;

  public:
    typedef typename orig::size_type size_type;

  public:
    Vector() : orig() {}
    Vector(size_type n) : orig(n) {}

    T& at(size_type idx) {
      for(; idx+PADDING >= orig::size(); orig::resize(orig::size()*2));
      return orig::operator[](idx);
    }

    void shift(size_type from, size_type to) {
      T tmp = at(from); // NOTE: avoid reference to old data_ ...
      at(to) = tmp;
      operator[](from) = T();
    }

#if defined(WIN32) || defined(WIN64) || defined(VECTOR_HAS_NO_MEMBER_DATA)
    // XXX: for dev
    const T* data() const {
      return orig::empty() ? NULL : &orig::operator[](0);
    }
#endif
  };
}

#endif
