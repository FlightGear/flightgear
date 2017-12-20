#ifndef _VECTOR_HPP
#define _VECTOR_HPP

#include <stdio.h>
#include <cassert>

namespace yasim {
//
// Excruciatingly simple vector-of-pointers class.  Easy & useful.
// No support for addition of elements anywhere but at the end of the
// list, nor for removal of elements.  Does not delete (or interpret
// in any way) its contents.
//
class Vector {
public:
    Vector();
    ~Vector();
    int   add(void* p);
    void* get(int i) const;
    void  set(int i, void* p);
    int   size() const;
    bool  empty() const;
private:
    void realloc();

    int _nelem {0};
    int _sz {0};
    void** _array {nullptr};
};

inline Vector::Vector()
{
    _nelem = 0;
    _sz = 0;
    _array = 0;
}

inline Vector::~Vector()
{
    delete[] _array;
}

inline int Vector::add(void* p)
{
    if(_nelem == _sz)
        realloc();
    _array[_sz] = p;
    return _sz++;
}

inline void* Vector::get(int i) const
{
    assert((i >= 0) && (i < _sz));
    return _array[i];

}

inline void Vector::set(int i, void* p)
{
    assert((i >= 0) && (i < _sz));
    _array[i] = p;
}

inline int Vector::size() const
{
    return _sz;
}

inline bool Vector::empty() const
{
    return _sz == 0;
}

inline void Vector::realloc()
{
    _nelem = 2*_nelem + 1;
    void** array = new void*[_nelem];
    for(int i=0; i<_sz; i++)
        array[i] = _array[i];
    delete[] _array;
    _array = array;
}
}; //namespace yasim
#endif // _VECTOR_HPP
