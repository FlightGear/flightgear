/* Unpack.h */

#ifndef Unpack_h
#define Unpack_h

#ifdef DOCUMENTATION
Objective:  We want to assign:
  Unpack<T>(xx, yy) = foo;

where the RHS is a list containing two elements of type T.  We want
the first element of the list to be assigned to xx, and the second
element of the list to be assigned to yy.  We want to do this in one
statement, so that nothing needs to be recomputed (or stored by the
creature on the RHS), to make it manifestly thread-safe.

We do not want to require any predeclared relationship between xx and
yy; we just put them together on the fly.

This sort of list-to-elements assignment is routine and natural
in perl.  There are several ways of doing it in C++.
This way is reasonably convenient.

#endif /* DOCUMENTATION */

using namespace std;
#include <iostream>
#include <list>


template<class T> class Pair{
public:
  T x;
  T y;

// no default constructor:
  Pair();

// Constructor using two objects of type T:
  inline Pair(const T& xx, const T& yy){
    x = xx;
    y = yy;
  }

};

template <class T> class Unpack {
  T* xptr;
  T* yptr;
public:

// no default constructor:
  Unpack();

// No copy constructor, at least until I figure out
// what it "should" do.
  Unpack(const Unpack& );

// Constructor using two objects of type T:
  inline Unpack(T& xx, T& yy){
    xptr = &xx;
    yptr = &yy;
  }

  typedef const list<T> CLT;

// Assignment operator, assigning from a list:
  inline CLT& operator= (CLT& rhs) {
    typename CLT::const_iterator ii(rhs.begin());
    if (ii != rhs.end()) {
      *xptr = *ii++;
    }
    if (ii != rhs.end()) {
      *yptr = *ii++;
    }
    return rhs;
  }


// Assignment operator, assigning from a Pair:
  inline const Pair<T>& operator= (const Pair<T>& rhs) {
    *xptr = rhs.x;
    *yptr = rhs.y;
    return rhs;
  }
};

#endif
