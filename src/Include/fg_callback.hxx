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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * $Id$
 **************************************************************************/

#ifndef _FG_CALLBACK_HXX
#define _FG_CALLBACK_HXX

/**
 * Abstract base class for all FlightGear callbacks.
 */
class fgCallback
{
public:

    /**
     * 
     */
    virtual ~fgCallback() {}

    /**
     * 
     */
    virtual fgCallback* clone() const = 0;

    /**
     * Execute the callback function.
     */
    virtual void operator()() = 0;

protected:
    /**
     * 
     */
    fgCallback() {}

private:
    // Not implemented.
    void operator=( const fgCallback& );
};

/**
 * Callback for invoking a file scope function.
 */
template< typename Fun >
class fgFunctionCallback : public fgCallback
{
public:
    /**
     * 
     */
    fgFunctionCallback( const Fun& fun )
	: fgCallback(), f_(fun) {}

    fgCallback* clone() const
    {
	return new fgFunctionCallback( *this );
    }

    void operator()() { f_(); }

private:
    // Not defined.
    fgFunctionCallback();

private:
    Fun f_;
};

/**
 * Callback for invoking a member function.
 */
template< class ObjPtr, typename MemFn >
class fgMethodCallback : public fgCallback
{
public:

    /**
     * 
     */
    fgMethodCallback( const ObjPtr& pObj, MemFn pMemFn )
	: fgCallback(),
	  pObj_(pObj),
	  pMemFn_(pMemFn)
    {
    }

    /**
     * 
     */
    fgCallback* clone() const
    {
	return new fgMethodCallback( *this );
    }

    /**
     * 
     */
    void operator()()
    {
	((*pObj_).*pMemFn_)();
    }

private:
    // Not defined.
    fgMethodCallback();

private:
    ObjPtr pObj_;
    MemFn pMemFn_;
};

/**
 * Helper template functions.
 */

template< typename Fun >
fgCallback*
make_callback( const Fun& fun )
{
    return new fgFunctionCallback<Fun>(fun);
}

template< class ObjPtr, typename MemFn >
fgCallback*
make_callback( const ObjPtr& pObj, MemFn pMemFn )
{
    return new fgMethodCallback<ObjPtr,MemFn>(pObj, pMemFn );
}

#endif // _FG_CALLBACK_HXX

