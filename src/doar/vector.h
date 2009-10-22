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

    // Copy *this[from] value to *this[to],
    // and initialize *this[from] by default T value.
    void shift(size_type from, size_type to) {
      // TODO: ......
      // NOTE: Value of at(from) must be saved as temporary value (not reference!),
      //       because if size() < 'to' then call resize(), reference to at(from) became invalid.
      //       So expression 'at(to) = at(from)' is danger that may be succeeded or may be failed denpend on compiler and compile options.
      T tmp = at(from); 
      at(to) = tmp;
      operator[](from) = T();
    }
    
    // NOTE: Definition of std::vector.data().
    //       It is not part of C++ standard yet. (but part of C++0x)
    //       G++ of a recent version has been implementing this method.
#ifndef _GLIBCXX_RESOLVE_LIB_DEFECTS 
    const T* data() const {
      return orig::empty() ? NULL : &orig::front();
    }
#endif
  };
}

#endif
