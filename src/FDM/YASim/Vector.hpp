#ifndef _VECTOR_HPP
#define _VECTOR_HPP

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
    void* get(int i);
    void  set(int i, void* p);
    int   size();
private:
    void realloc();

    int _nelem;
    int _sz;
    void** _array;
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

inline void* Vector::get(int i)
{
    return _array[i];
}

inline void Vector::set(int i, void* p)
{
    _array[i] = p;
}

inline int Vector::size()
{
    return _sz;
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

#endif // _VECTOR_HPP
