#ifndef __FG_PROPS_HXX
#define __FG_PROPS_HXX 1

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/props.hxx>
#include "globals.hxx"


////////////////////////////////////////////////////////////////////////
// Loading and saving properties.
////////////////////////////////////////////////////////////////////////

extern bool fgSaveFlight (ostream &output);
extern bool fgLoadFlight (istream &input);



////////////////////////////////////////////////////////////////////////
// Convenience functions for tying properties, with logging.
////////////////////////////////////////////////////////////////////////

inline void
fgUntie (const string &name)
{
  if (!globals->get_props()->untie(name))
    FG_LOG(FG_GENERAL, FG_WARN, "Failed to untie property " << name);
}


				// Templates cause ambiguity here
inline void
fgTie (const string &name, bool *pointer)
{
  if (!globals->get_props()->tie(name, SGRawValuePointer<bool>(pointer)))
    FG_LOG(FG_GENERAL, FG_WARN,
	   "Failed to tie property " << name << " to a pointer");
}

inline void
fgTie (const string &name, int *pointer)
{
  if (!globals->get_props()->tie(name, SGRawValuePointer<int>(pointer)))
    FG_LOG(FG_GENERAL, FG_WARN,
	   "Failed to tie property " << name << " to a pointer");
}

inline void
fgTie (const string &name, float *pointer)
{
  if (!globals->get_props()->tie(name, SGRawValuePointer<float>(pointer)))
    FG_LOG(FG_GENERAL, FG_WARN,
	   "Failed to tie property " << name << " to a pointer");
}

inline void
fgTie (const string &name, double *pointer)
{
  if (!globals->get_props()->tie(name, SGRawValuePointer<double>(pointer)))
    FG_LOG(FG_GENERAL, FG_WARN,
	   "Failed to tie property " << name << " to a pointer");
}

inline void
fgTie (const string &name, string *pointer)
{
  if (!globals->get_props()->tie(name, SGRawValuePointer<string>(pointer)))
    FG_LOG(FG_GENERAL, FG_WARN,
	   "Failed to tie property " << name << " to a pointer");
}

template <class V>
inline void
fgTie (const string &name, V (*getter)(), void (*setter)(V) = 0)
{
  if (!globals->get_props()->tie(name, SGRawValueFunctions<V>(getter, setter)))
    FG_LOG(FG_GENERAL, FG_WARN,
	   "Failed to tie property " << name << " to functions");
}

template <class V>
inline void
fgTie (const string &name, int index, V (*getter)(int),
       void (*setter)(int, V) = 0)
{
  if (!globals->get_props()->tie(name,
				 SGRawValueFunctionsIndexed<V>(index,
							       getter,
							       setter)))
    FG_LOG(FG_GENERAL, FG_WARN,
	   "Failed to tie property " << name << " to indexed functions");
}

template <class T, class V>
inline void
fgTie (const string &name, T * obj, V (T::*getter)() const,
       void (T::*setter)(V) = 0)
{
  if (!globals->get_props()->tie(name,
				 SGRawValueMethods<T,V>(*obj, getter, setter)))
    FG_LOG(FG_GENERAL, FG_WARN,
	   "Failed to tie property " << name << " to object methods");
}

template <class T, class V>
inline void 
fgTie (const string &name, T * obj, int index,
       V (T::*getter)(int) const, void (T::*setter)(int, V) = 0)
{
  if (!globals->get_props()->tie(name,
				 SGRawValueMethodsIndexed<T,V>(*obj,
							       index,
							       getter,
							       setter)))
    FG_LOG(FG_GENERAL, FG_WARN,
	   "Failed to tie property " << name << " to indexed object methods");
}


#endif // __FG_PROPS_HXX

