/**************************************************************************
 * fg_callback.hxx -- Wrapper classes to treat function and method pointers
 * as objects.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id$
 **************************************************************************/

#ifndef _FG_CALLBACK_HXX
#define _FG_CALLBACK_HXX

// -dw- need size_t for params() function
#ifdef __MWERKS__
typedef unsigned long	size_t;
#endif


//-----------------------------------------------------------------------------
//
// Abstract base class for all FlightGear callbacks.
//
class fgCallback
{
public:
    virtual ~fgCallback() {}

    virtual fgCallback* clone() const = 0;
    virtual void* call( void** ) = 0;

    size_t params() const { return n_params; }

protected:
    fgCallback( size_t params )
	: n_params(params) {}

protected:
    // The number of parameters to pass to the callback routine.
    size_t n_params;

private:
};

//-----------------------------------------------------------------------------
//
// Callback for invoking a file scope function.
//
class fgFunctionCallback : public fgCallback
{
public:
    // Pointer to function taking no arguments and returning void.
    typedef void (*Proc0v)();

    // A callback instance to invoke the function 'p'
    fgFunctionCallback( Proc0v p );

    // Create a clone on the heap.
    virtual fgCallback* clone() const;

private:
    void* call( void** in );
    inline void* call0v( void** );

private:
    // Not defined.
    fgFunctionCallback();

private:

    typedef void* (fgFunctionCallback::*DoPtr)( void** );
    DoPtr doPtr;
    Proc0v proc0v;
};

inline
fgFunctionCallback::fgFunctionCallback( Proc0v p )
    : fgCallback(0),
      doPtr(&fgFunctionCallback::call0v),
      proc0v(p)
{
    // empty
}

inline fgCallback*
fgFunctionCallback::clone() const
{
    return new fgFunctionCallback( *this );
}

inline void*
fgFunctionCallback::call( void** in )
{
    return (this->*doPtr)( in );
}

inline void*
fgFunctionCallback::call0v( void** )
{
    (*proc0v)();
    return (void*) NULL;
}

//-----------------------------------------------------------------------------
//
// Callback for invoking an object method.
//
template< class T >
class fgMethodCallback : public fgCallback
{
public:
    // Pointer to method taking no arguments and returning void.
    typedef void (T::*Method0v)();

    // A callback instance to invoke method 'm' of object 'o' 
    fgMethodCallback( T* o, Method0v m )
	: fgCallback(0),
	  object(o),
	  method0v(m),
	  doPtr(&fgMethodCallback<T>::call0v) {}

    // Create a clone on the heap.
    fgCallback* clone() const;

private:
    //
    void* call( void** in );

    //
    void* call0v( void** );

private:
    // Not defined.
    fgMethodCallback();

private:
    T* object;
    Method0v method0v;

    typedef void * (fgMethodCallback::*DoPtr)( void ** );
    DoPtr doPtr;
};

template< class T > inline fgCallback*
fgMethodCallback<T>::clone() const
{
    return new fgMethodCallback( *this );
}

template< class T > inline void*
fgMethodCallback<T>::call( void** in )
{
    return (this->*doPtr)( in );
}


template< class T > inline void*
fgMethodCallback<T>::call0v( void** )
{
    (object->*method0v)();
    return (void*) NULL;
}

#endif // _FG_CALLBACK_HXX

