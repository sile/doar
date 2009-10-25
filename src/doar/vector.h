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

    // *this[from] is copied onto *this[to],
    // and initialized by default T value.
    void shift(size_type from, size_type to) {
      // NOTE: Brief next expression 'at(to) = at(from)' is danger.
      //       Because reference of at(from) maybe become invalid if size() <= to.
      //       (It's behavior is depends on your compiler and compiler options.) 
      //       So it is required to save value of at(from) in temporary variable
      //       and then assign it on at(to).
      T tmp = at(from); 
      at(to) = tmp;
      operator[](from) = T();
    }
    
    // NOTE: Definition of std::vector.data().
    //       This method is not part of C++ standard yet. (but part of C++0x)
    //       Some C++ compiler have been implemeting this method. (see types.h)
#ifndef VECTOR_HAS_MEMBER_NAMED_DATA
    const T* data() const {
      return orig::empty() ? NULL : &orig::front();
    }
#endif
  };
}

#endif
