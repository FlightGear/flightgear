// fg_props.hxx - Declarations and inline methods for property handling.
// Written by David Megginson, started 2000.
//
// This file is in the Public Domain, and comes with no warranty.

#ifndef __FG_PROPS_HXX
#define __FG_PROPS_HXX 1

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/props.hxx>
#include "globals.hxx"


////////////////////////////////////////////////////////////////////////
// Property management.
////////////////////////////////////////////////////////////////////////


/**
 * Initialize the default FlightGear properties.
 *
 * These are mostly properties that haven't been claimed by a
 * specific module yet.  This function should be invoked once,
 * while the program is starting (and after the global property
 * tree has been created).
 */
extern void fgInitProps ();	// fixme: how are they untied?


/**
 * Update the default FlightGear properties.
 *
 * This function should be invoked once in each loop to update all
 * of the default properties.
 */
extern void fgUpdateProps ();


/**
 * Save a flight to disk.
 *
 * This function saves all of the archivable properties to a stream
 * so that the current flight can be restored later.
 *
 * @param output The output stream to write the XML save file to.
 * @return true if the flight was saved successfully.
 */
extern bool fgSaveFlight (ostream &output);


/**
 * Load a flight from disk.
 *
 * This function loads an XML save file from a stream to restore
 * a flight.
 *
 * @param input The input stream to read the XML from.
 * @return true if the flight was restored successfully.
 */
extern bool fgLoadFlight (istream &input);



////////////////////////////////////////////////////////////////////////
// Convenience functions for getting property values.
////////////////////////////////////////////////////////////////////////


/**
 * Get a property node.
 *
 * @param path The path of the node, relative to root.
 * @param create true to create the node if it doesn't exist.
 * @return The node, or 0 if none exists and none was created.
 */
inline SGPropertyNode * 
fgGetNode (const string &path, bool create = false)
{
  return globals->get_props()->getNode(path, create);
}


/**
 * Test whether a given node exists.
 *
 * @param path The path of the node, relative to root.
 * @return true if the node exists, false otherwise.
 */
inline bool
fgHasNode (const string &path)
{
  return (fgGetNode(path, false) != 0);
}


/**
 * Get a bool value for a property.
 *
 * This method is convenient but inefficient.  It should be used
 * infrequently (i.e. for initializing, loading, saving, etc.),
 * not in the main loop.  If you need to get a value frequently,
 * it is better to look up the node itself using fgGetNode and then
 * use the node's getBoolValue() method, to avoid the lookup overhead.
 *
 * @param name The property name.
 * @param defaultValue The default value to return if the property
 *        does not exist.
 * @return The property's value as a bool, or the default value provided.
 */
inline bool fgGetBool (const string &name, bool defaultValue = false)
{
  return globals->get_props()->getBoolValue(name, defaultValue);
}


/**
 * Get an int value for a property.
 *
 * This method is convenient but inefficient.  It should be used
 * infrequently (i.e. for initializing, loading, saving, etc.),
 * not in the main loop.  If you need to get a value frequently,
 * it is better to look up the node itself using fgGetNode and then
 * use the node's getIntValue() method, to avoid the lookup overhead.
 *
 * @param name The property name.
 * @param defaultValue The default value to return if the property
 *        does not exist.
 * @return The property's value as an int, or the default value provided.
 */
inline int fgGetInt (const string &name, int defaultValue = 0)
{
  return globals->get_props()->getIntValue(name, defaultValue);
}


/**
 * Get a long value for a property.
 *
 * This method is convenient but inefficient.  It should be used
 * infrequently (i.e. for initializing, loading, saving, etc.),
 * not in the main loop.  If you need to get a value frequently,
 * it is better to look up the node itself using fgGetNode and then
 * use the node's getLongValue() method, to avoid the lookup overhead.
 *
 * @param name The property name.
 * @param defaultValue The default value to return if the property
 *        does not exist.
 * @return The property's value as a long, or the default value provided.
 */
inline int fgGetLong (const string &name, long defaultValue = 0L)
{
  return globals->get_props()->getLongValue(name, defaultValue);
}


/**
 * Get a float value for a property.
 *
 * This method is convenient but inefficient.  It should be used
 * infrequently (i.e. for initializing, loading, saving, etc.),
 * not in the main loop.  If you need to get a value frequently,
 * it is better to look up the node itself using fgGetNode and then
 * use the node's getFloatValue() method, to avoid the lookup overhead.
 *
 * @param name The property name.
 * @param defaultValue The default value to return if the property
 *        does not exist.
 * @return The property's value as a float, or the default value provided.
 */
inline float fgGetFloat (const string &name, float defaultValue = 0.0)
{
  return globals->get_props()->getFloatValue(name, defaultValue);
}


/**
 * Get a double value for a property.
 *
 * This method is convenient but inefficient.  It should be used
 * infrequently (i.e. for initializing, loading, saving, etc.),
 * not in the main loop.  If you need to get a value frequently,
 * it is better to look up the node itself using fgGetNode and then
 * use the node's getDoubleValue() method, to avoid the lookup overhead.
 *
 * @param name The property name.
 * @param defaultValue The default value to return if the property
 *        does not exist.
 * @return The property's value as a double, or the default value provided.
 */
inline double fgGetDouble (const string &name, double defaultValue = 0.0)
{
  return globals->get_props()->getDoubleValue(name, defaultValue);
}


/**
 * Get a string value for a property.
 *
 * This method is convenient but inefficient.  It should be used
 * infrequently (i.e. for initializing, loading, saving, etc.),
 * not in the main loop.  If you need to get a value frequently,
 * it is better to look up the node itself using fgGetNode and then
 * use the node's getStringValue() method, to avoid the lookup overhead.
 *
 * @param name The property name.
 * @param defaultValue The default value to return if the property
 *        does not exist.
 * @return The property's value as a string, or the default value provided.
 */
inline string fgGetString (const string &name, string defaultValue = "")
{
  return globals->get_props()->getStringValue(name, defaultValue);
}


/**
 * Set a bool value for a property.
 *
 * Assign a bool value to a property.  If the property does not
 * yet exist, it will be created and its type will be set to
 * BOOL; if it has a type of UNKNOWN, the type will also be set to
 * BOOL; otherwise, the bool value will be converted to the property's
 * type.
 *
 * @param name The property name.
 * @param val The new value for the property.
 * @return true if the assignment succeeded, false otherwise.
 */
inline bool fgSetBool (const string &name, bool val)
{
  return globals->get_props()->setBoolValue(name, val);
}


/**
 * Set an int value for a property.
 *
 * Assign an int value to a property.  If the property does not
 * yet exist, it will be created and its type will be set to
 * INT; if it has a type of UNKNOWN, the type will also be set to
 * INT; otherwise, the bool value will be converted to the property's
 * type.
 *
 * @param name The property name.
 * @param val The new value for the property.
 * @return true if the assignment succeeded, false otherwise.
 */
inline bool fgSetInt (const string &name, int val)
{
  return globals->get_props()->setIntValue(name, val);
}


/**
 * Set a long value for a property.
 *
 * Assign a long value to a property.  If the property does not
 * yet exist, it will be created and its type will be set to
 * LONG; if it has a type of UNKNOWN, the type will also be set to
 * LONG; otherwise, the bool value will be converted to the property's
 * type.
 *
 * @param name The property name.
 * @param val The new value for the property.
 * @return true if the assignment succeeded, false otherwise.
 */
inline bool fgSetLong (const string &name, long val)
{
  return globals->get_props()->setLongValue(name, val);
}


/**
 * Set a float value for a property.
 *
 * Assign a float value to a property.  If the property does not
 * yet exist, it will be created and its type will be set to
 * FLOAT; if it has a type of UNKNOWN, the type will also be set to
 * FLOAT; otherwise, the bool value will be converted to the property's
 * type.
 *
 * @param name The property name.
 * @param val The new value for the property.
 * @return true if the assignment succeeded, false otherwise.
 */
inline bool fgSetFloat (const string &name, float val)
{
  return globals->get_props()->setFloatValue(name, val);
}


/**
 * Set a double value for a property.
 *
 * Assign a double value to a property.  If the property does not
 * yet exist, it will be created and its type will be set to
 * DOUBLE; if it has a type of UNKNOWN, the type will also be set to
 * DOUBLE; otherwise, the double value will be converted to the property's
 * type.
 *
 * @param name The property name.
 * @param val The new value for the property.
 * @return true if the assignment succeeded, false otherwise.
 */
inline bool fgSetDouble (const string &name, double val)
{
  return globals->get_props()->setDoubleValue(name, val);
}


/**
 * Set a string value for a property.
 *
 * Assign a string value to a property.  If the property does not
 * yet exist, it will be created and its type will be set to
 * STRING; if it has a type of UNKNOWN, the type will also be set to
 * STRING; otherwise, the string value will be converted to the property's
 * type.
 *
 * @param name The property name.
 * @param val The new value for the property.
 * @return true if the assignment succeeded, false otherwise.
 */
inline bool fgSetString (const string &name, const string &val)
{
  return globals->get_props()->setStringValue(name, val);
}



////////////////////////////////////////////////////////////////////////
// Convenience functions for setting property attributes.
////////////////////////////////////////////////////////////////////////


/**
 * Set the state of the archive attribute for a property.
 *
 * If the archive attribute is true, the property will be written
 * when a flight is saved; if it is false, the property will be
 * skipped.
 *
 * A warning message will be printed if the property does not exist.
 *
 * @param name The property name.
 * @param state The state of the archive attribute (defaults to true).
 */
inline void
fgSetArchivable (const string &name, bool state = true)
{
  SGPropertyNode * node = globals->get_props()->getNode(name);
  if (node == 0)
    SG_LOG(SG_GENERAL, SG_ALERT,
	   "Attempt to set archive flag for non-existant property "
	   << name);
  else
    node->setAttribute(SGPropertyNode::ARCHIVE, state);
}


/**
 * Set the state of the read attribute for a property.
 *
 * If the read attribute is true, the property value will be readable;
 * if it is false, the property value will always be the default value
 * for its type.
 *
 * A warning message will be printed if the property does not exist.
 *
 * @param name The property name.
 * @param state The state of the read attribute (defaults to true).
 */
inline void
fgSetReadable (const string &name, bool state = true)
{
  SGPropertyNode * node = globals->get_props()->getNode(name);
  if (node == 0)
    SG_LOG(SG_GENERAL, SG_ALERT,
	   "Attempt to set read flag for non-existant property "
	   << name);
  else
    node->setAttribute(SGPropertyNode::READ, state);
}


/**
 * Set the state of the write attribute for a property.
 *
 * If the write attribute is true, the property value may be modified
 * (depending on how it is tied); if the write attribute is false, the
 * property value may not be modified.
 *
 * A warning message will be printed if the property does not exist.
 *
 * @param name The property name.
 * @param state The state of the write attribute (defaults to true).
 */
inline void
fgSetWritable (const string &name, bool state = true)
{
  SGPropertyNode * node = globals->get_props()->getNode(name);
  if (node == 0)
    SG_LOG(SG_GENERAL, SG_ALERT,
	   "Attempt to set write flag for non-existant property "
	   << name);
  else
    node->setAttribute(SGPropertyNode::WRITE, state);
}



////////////////////////////////////////////////////////////////////////
// Convenience functions for tying properties, with logging.
////////////////////////////////////////////////////////////////////////


/**
 * Untie a property from an external data source.
 *
 * Classes should use this function to release control of any
 * properties they are managing.
 */
inline void
fgUntie (const string &name)
{
  if (!globals->get_props()->untie(name))
    SG_LOG(SG_GENERAL, SG_WARN, "Failed to untie property " << name);
}


				// Templates cause ambiguity here

/**
 * Tie a property to an external bool variable.
 *
 * The property's value will automatically mirror the variable's
 * value, and vice-versa, until the property is untied.
 *
 * @param name The property name to tie (full path).
 * @param pointer A pointer to the variable.
 * @param useDefault true if any existing property value should be
 *        copied to the variable; false if the variable should not
 *        be modified; defaults to true.
 */
inline void
fgTie (const string &name, bool *pointer, bool useDefault = true)
{
  if (!globals->get_props()->tie(name, SGRawValuePointer<bool>(pointer),
				 useDefault))
    SG_LOG(SG_GENERAL, SG_WARN,
	   "Failed to tie property " << name << " to a pointer");
}


/**
 * Tie a property to an external int variable.
 *
 * The property's value will automatically mirror the variable's
 * value, and vice-versa, until the property is untied.
 *
 * @param name The property name to tie (full path).
 * @param pointer A pointer to the variable.
 * @param useDefault true if any existing property value should be
 *        copied to the variable; false if the variable should not
 *        be modified; defaults to true.
 */
inline void
fgTie (const string &name, int *pointer, bool useDefault = true)
{
  if (!globals->get_props()->tie(name, SGRawValuePointer<int>(pointer),
				 useDefault))
    SG_LOG(SG_GENERAL, SG_WARN,
	   "Failed to tie property " << name << " to a pointer");
}


/**
 * Tie a property to an external long variable.
 *
 * The property's value will automatically mirror the variable's
 * value, and vice-versa, until the property is untied.
 *
 * @param name The property name to tie (full path).
 * @param pointer A pointer to the variable.
 * @param useDefault true if any existing property value should be
 *        copied to the variable; false if the variable should not
 *        be modified; defaults to true.
 */
inline void
fgTie (const string &name, long *pointer, bool useDefault = true)
{
  if (!globals->get_props()->tie(name, SGRawValuePointer<long>(pointer),
				 useDefault))
    SG_LOG(SG_GENERAL, SG_WARN,
	   "Failed to tie property " << name << " to a pointer");
}


/**
 * Tie a property to an external float variable.
 *
 * The property's value will automatically mirror the variable's
 * value, and vice-versa, until the property is untied.
 *
 * @param name The property name to tie (full path).
 * @param pointer A pointer to the variable.
 * @param useDefault true if any existing property value should be
 *        copied to the variable; false if the variable should not
 *        be modified; defaults to true.
 */
inline void
fgTie (const string &name, float *pointer, bool useDefault = true)
{
  if (!globals->get_props()->tie(name, SGRawValuePointer<float>(pointer),
				 useDefault))
    SG_LOG(SG_GENERAL, SG_WARN,
	   "Failed to tie property " << name << " to a pointer");
}


/**
 * Tie a property to an external double variable.
 *
 * The property's value will automatically mirror the variable's
 * value, and vice-versa, until the property is untied.
 *
 * @param name The property name to tie (full path).
 * @param pointer A pointer to the variable.
 * @param useDefault true if any existing property value should be
 *        copied to the variable; false if the variable should not
 *        be modified; defaults to true.
 */
inline void
fgTie (const string &name, double *pointer, bool useDefault = true)
{
  if (!globals->get_props()->tie(name, SGRawValuePointer<double>(pointer),
				 useDefault))
    SG_LOG(SG_GENERAL, SG_WARN,
	   "Failed to tie property " << name << " to a pointer");
}


/**
 * Tie a property to an external string variable.
 *
 * The property's value will automatically mirror the variable's
 * value, and vice-versa, until the property is untied.
 *
 * @param name The property name to tie (full path).
 * @param pointer A pointer to the variable.
 * @param useDefault true if any existing property value should be
 *        copied to the variable; false if the variable should not
 *        be modified; defaults to true.
 */
inline void
fgTie (const string &name, string *pointer, bool useDefault = true)
{
  if (!globals->get_props()->tie(name, SGRawValuePointer<string>(pointer),
				 useDefault))
    SG_LOG(SG_GENERAL, SG_WARN,
	   "Failed to tie property " << name << " to a pointer");
}


/**
 * Tie a property to a pair of simple functions.
 *
 * Every time the property value is queried, the getter (if any) will
 * be invoked; every time the property value is modified, the setter
 * (if any) will be invoked.  The getter can be 0 to make the property
 * unreadable, and the setter can be 0 to make the property
 * unmodifiable.
 *
 * @param name The property name to tie (full path).
 * @param getter The getter function, or 0 if the value is unreadable.
 * @param setter The setter function, or 0 if the value is unmodifiable.
 * @param useDefault true if the setter should be invoked with any existing 
 *        property value should be; false if the old value should be
 *        discarded; defaults to true.
 */
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


/**
 * Tie a property to a pair of indexed functions.
 *
 * Every time the property value is queried, the getter (if any) will
 * be invoked with the index provided; every time the property value
 * is modified, the setter (if any) will be invoked with the index
 * provided.  The getter can be 0 to make the property unreadable, and
 * the setter can be 0 to make the property unmodifiable.
 *
 * @param name The property name to tie (full path).
 * @param index The integer argument to pass to the getter and
 *        setter functions.
 * @param getter The getter function, or 0 if the value is unreadable.
 * @param setter The setter function, or 0 if the value is unmodifiable.
 * @param useDefault true if the setter should be invoked with any existing 
 *        property value should be; false if the old value should be
 *        discarded; defaults to true.
 */
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


/**
 * Tie a property to a pair of object methods.
 *
 * Every time the property value is queried, the getter (if any) will
 * be invoked; every time the property value is modified, the setter
 * (if any) will be invoked.  The getter can be 0 to make the property
 * unreadable, and the setter can be 0 to make the property
 * unmodifiable.
 *
 * @param name The property name to tie (full path).
 * @param obj The object whose methods should be invoked.
 * @param getter The object's getter method, or 0 if the value is
 *        unreadable.
 * @param setter The object's setter method, or 0 if the value is
 *        unmodifiable.
 * @param useDefault true if the setter should be invoked with any existing 
 *        property value should be; false if the old value should be
 *        discarded; defaults to true.
 */
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


/**
 * Tie a property to a pair of indexed object methods.
 *
 * Every time the property value is queried, the getter (if any) will
 * be invoked with the index provided; every time the property value
 * is modified, the setter (if any) will be invoked with the index
 * provided.  The getter can be 0 to make the property unreadable, and
 * the setter can be 0 to make the property unmodifiable.
 *
 * @param name The property name to tie (full path).
 * @param obj The object whose methods should be invoked.
 * @param index The integer argument to pass to the getter and
 *        setter methods.
 * @param getter The getter method, or 0 if the value is unreadable.
 * @param setter The setter method, or 0 if the value is unmodifiable.
 * @param useDefault true if the setter should be invoked with any existing 
 *        property value should be; false if the old value should be
 *        discarded; defaults to true.
 */
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



////////////////////////////////////////////////////////////////////////
// Conditions.
////////////////////////////////////////////////////////////////////////


/**
 * An encoded condition.
 *
 * This class encodes a single condition of some sort, possibly
 * connected with properties.
 *
 * This class should migrate to somewhere more general.
 */
class FGCondition
{
public:
  FGCondition ();
  virtual ~FGCondition ();
  virtual bool test () const = 0;
};


/**
 * Condition for a single property.
 *
 * This condition is true only if the property returns a boolean
 * true value.
 */
class FGPropertyCondition : public FGCondition
{
public:
  FGPropertyCondition (const string &propname);
  virtual ~FGPropertyCondition ();
  virtual bool test () const { return _node->getBoolValue(); }
private:
  const SGPropertyNode * _node;
};


/**
 * Condition for a 'not' operator.
 *
 * This condition is true only if the child condition is false.
 */
class FGNotCondition : public FGCondition
{
public:
				// transfer pointer ownership
  FGNotCondition (FGCondition * condition);
  virtual ~FGNotCondition ();
  virtual bool test () const;
private:
  FGCondition * _condition;
};


/**
 * Condition for an 'and' group.
 *
 * This condition is true only if all of the conditions
 * in the group are true.
 */
class FGAndCondition : public FGCondition
{
public:
  FGAndCondition ();
  virtual ~FGAndCondition ();
  virtual bool test () const;
				// transfer pointer ownership
  virtual void addCondition (FGCondition * condition);
private:
  vector<FGCondition *> _conditions;
};


/**
 * Condition for an 'or' group.
 *
 * This condition is true if at least one of the conditions in the
 * group is true.
 */
class FGOrCondition : public FGCondition
{
public:
  FGOrCondition ();
  virtual ~FGOrCondition ();
  virtual bool test () const;
				// transfer pointer ownership
  virtual void addCondition (FGCondition * condition);
private:
  vector<FGCondition *> _conditions;
};


/**
 * Abstract base class for property comparison conditions.
 */
class FGComparisonCondition : public FGCondition
{
public:
  enum Type {
    LESS_THAN,
    GREATER_THAN,
    EQUALS
  };
  FGComparisonCondition (Type type, bool reverse = false);
  virtual ~FGComparisonCondition ();
  virtual bool test () const;
  virtual void setLeftProperty (const string &propname);
  virtual void setRightProperty (const string &propname);
				// will make a local copy
  virtual void setRightValue (const SGPropertyNode * value);
private:
  Type _type;
  bool _reverse;
  const SGPropertyNode * _left_property;
  const SGPropertyNode * _right_property;
  const SGPropertyNode * _right_value;
};


/**
 * Base class for a conditional components.
 *
 * This class manages the conditions and tests; the component should
 * invoke the test() method whenever it needs to decide whether to
 * active itself, draw itself, and so on.
 */
class FGConditional
{
public:
  FGConditional ();
  virtual ~FGConditional ();
				// transfer pointer ownership
  virtual void setCondition (FGCondition * condition);
  virtual const FGCondition * getCondition () const { return _condition; }
  virtual bool test () const;
private:
  FGCondition * _condition;
};


/**
 * Global function to make a condition out of properties.
 *
 * The top-level is always an implicit 'and' group, whatever the
 * node's name (it should usually be "condition").
 *
 * @param node The top-level condition node (usually named "condition").
 * @return A pointer to a newly-allocated condition; it is the
 *         responsibility of the caller to delete the condition when
 *         it is no longer needed.
 */
FGCondition * fgReadCondition (const SGPropertyNode * node);


#endif // __FG_PROPS_HXX

