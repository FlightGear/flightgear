#ifndef __FG_PROPS_HXX
#define __FG_PROPS_HXX 1

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/props.hxx>
#include "globals.hxx"


////////////////////////////////////////////////////////////////////////
// Property management.
////////////////////////////////////////////////////////////////////////

extern void fgInitProps ();	// fixme: how are they untied?
extern void fgUpdateProps ();
extern bool fgSaveFlight (ostream &output);
extern bool fgLoadFlight (istream &input);



////////////////////////////////////////////////////////////////////////
// Convenience functions for getting property values.
////////////////////////////////////////////////////////////////////////


/**
 * Get a property node.
 *
 * @param path The path of the node, relative to root.
 * @param create true to create the node if it doesn't exist.
 */
inline SGPropertyNode * 
fgGetNode (const string &path, bool create = false)
{
  return globals->get_props()->getNode(path, create);
}


/**
 * Get an SGValue pointer that can be queried repeatedly.
 *
 * If the property value is going to be accessed within the loop,
 * it is best to use this method for maximum efficiency.
 */
inline SGValue * 
fgGetValue (const string &name, bool create = false)
{
  return globals->get_props()->getValue(name, create);
}

inline bool fgHasValue (const string &name)
{
  return globals->get_props()->hasValue(name);
}

inline bool fgGetBool (const string &name, bool defaultValue = false)
{
  return globals->get_props()->getBoolValue(name, defaultValue);
}

inline int fgGetInt (const string &name, int defaultValue = 0)
{
  return globals->get_props()->getIntValue(name, defaultValue);
}

inline int fgGetLong (const string &name, long defaultValue = 0L)
{
  return globals->get_props()->getLongValue(name, defaultValue);
}

inline float fgGetFloat (const string &name, float defaultValue = 0.0)
{
  return globals->get_props()->getFloatValue(name, defaultValue);
}

inline double fgGetDouble (const string &name, double defaultValue = 0.0)
{
  return globals->get_props()->getDoubleValue(name, defaultValue);
}

inline string fgGetString (const string &name, string defaultValue = "")
{
  return globals->get_props()->getStringValue(name, defaultValue);
}

inline bool fgSetBool (const string &name, bool val)
{
  return globals->get_props()->setBoolValue(name, val);
}

inline bool fgSetInt (const string &name, int val)
{
  return globals->get_props()->setIntValue(name, val);
}

inline bool fgSetLong (const string &name, long val)
{
  return globals->get_props()->setLongValue(name, val);
}

inline bool fgSetFloat (const string &name, float val)
{
  return globals->get_props()->setFloatValue(name, val);
}

inline bool fgSetDouble (const string &name, double val)
{
  return globals->get_props()->setDoubleValue(name, val);
}

inline bool fgSetString (const string &name, const string &val)
{
  return globals->get_props()->setStringValue(name, val);
}



////////////////////////////////////////////////////////////////////////
// Convenience functions for tying properties, with logging.
////////////////////////////////////////////////////////////////////////

inline void
fgUntie (const string &name)
{
  if (!globals->get_props()->untie(name))
    SG_LOG(SG_GENERAL, SG_WARN, "Failed to untie property " << name);
}


				// Templates cause ambiguity here
inline void
fgTie (const string &name, bool *pointer, bool useDefault = true)
{
  if (!globals->get_props()->tie(name, SGRawValuePointer<bool>(pointer),
				 useDefault))
    SG_LOG(SG_GENERAL, SG_WARN,
	   "Failed to tie property " << name << " to a pointer");
}

inline void
fgTie (const string &name, int *pointer, bool useDefault = true)
{
  if (!globals->get_props()->tie(name, SGRawValuePointer<int>(pointer),
				 useDefault))
    SG_LOG(SG_GENERAL, SG_WARN,
	   "Failed to tie property " << name << " to a pointer");
}

inline void
fgTie (const string &name, long *pointer, bool useDefault = true)
{
  if (!globals->get_props()->tie(name, SGRawValuePointer<long>(pointer),
				 useDefault))
    SG_LOG(SG_GENERAL, SG_WARN,
	   "Failed to tie property " << name << " to a pointer");
}

inline void
fgTie (const string &name, float *pointer, bool useDefault = true)
{
  if (!globals->get_props()->tie(name, SGRawValuePointer<float>(pointer),
				 useDefault))
    SG_LOG(SG_GENERAL, SG_WARN,
	   "Failed to tie property " << name << " to a pointer");
}

inline void
fgTie (const string &name, double *pointer, bool useDefault = true)
{
  if (!globals->get_props()->tie(name, SGRawValuePointer<double>(pointer),
				 useDefault))
    SG_LOG(SG_GENERAL, SG_WARN,
	   "Failed to tie property " << name << " to a pointer");
}

inline void
fgTie (const string &name, string *pointer, bool useDefault = true)
{
  if (!globals->get_props()->tie(name, SGRawValuePointer<string>(pointer),
				 useDefault))
    SG_LOG(SG_GENERAL, SG_WARN,
	   "Failed to tie property " << name << " to a pointer");
}

template <class V>
inline void
fgTie (const string &name, V (*getter)(), void (*setter)(V) = 0,
       bool useDefault = true)
{
  if (!globals->get_props()->tie(name, SGRawValueFunctions<V>(getter, setter),
				 useDefault))
    SG_LOG(SG_GENERAL, SG_WARN,
	   "Failed to tie property " << name << " to functions");
}

template <class V>
inline void
fgTie (const string &name, int index, V (*getter)(int),
       void (*setter)(int, V) = 0, bool useDefault = true)
{
  if (!globals->get_props()->tie(name,
				 SGRawValueFunctionsIndexed<V>(index,
							       getter,
							       setter),
				 useDefault))
    SG_LOG(SG_GENERAL, SG_WARN,
	   "Failed to tie property " << name << " to indexed functions");
}

template <class T, class V>
inline void
fgTie (const string &name, T * obj, V (T::*getter)() const,
       void (T::*setter)(V) = 0, bool useDefault = true)
{
  if (!globals->get_props()->tie(name,
				 SGRawValueMethods<T,V>(*obj, getter, setter),
				 useDefault))
    SG_LOG(SG_GENERAL, SG_WARN,
	   "Failed to tie property " << name << " to object methods");
}

template <class T, class V>
inline void 
fgTie (const string &name, T * obj, int index,
       V (T::*getter)(int) const, void (T::*setter)(int, V) = 0,
       bool useDefault = true)
{
  if (!globals->get_props()->tie(name,
				 SGRawValueMethodsIndexed<T,V>(*obj,
							       index,
							       getter,
							       setter),
				 useDefault))
    SG_LOG(SG_GENERAL, SG_WARN,
	   "Failed to tie property " << name << " to indexed object methods");
}


#endif // __FG_PROPS_HXX

