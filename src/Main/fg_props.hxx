// fg_props.hxx - Declarations and inline methods for property handling.
// Written by David Megginson, started 2000.
//
// This file is in the Public Domain, and comes with no warranty.

#ifndef __FG_PROPS_HXX
#define __FG_PROPS_HXX 1

#include <iosfwd>

#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/props/tiedpropertylist.hxx>

#include <Main/globals.hxx>

////////////////////////////////////////////////////////////////////////
// Property management.
////////////////////////////////////////////////////////////////////////

class FGProperties : public SGSubsystem
{
public:
    FGProperties ();
    virtual ~FGProperties ();

    void init ();
    void bind ();
    void unbind ();
    void update (double dt);

private:
    simgear::TiedPropertyList _tiedProperties;
};


/**
 * Save a flight to disk.
 *
 * This function saves all of the archivable properties to a stream
 * so that the current flight can be restored later.
 *
 * @param output The output stream to write the XML save file to.
 * @param write_all If true, write all properties rather than
 *        just the ones flagged as archivable.
 * @return true if the flight was saved successfully.
 */
extern bool fgSaveFlight (std::ostream &output, bool write_all = false);


/**
 * Load a flight from disk.
 *
 * This function loads an XML save file from a stream to restore
 * a flight.
 *
 * @param input The input stream to read the XML from.
 * @return true if the flight was restored successfully.
 */
extern bool fgLoadFlight (std::istream &input);


/**
 * Load properties from a file.
 *
 * @param file The relative or absolute filename.
 * @param props The property node to load the properties into.
 * @param in_fg_root If true, look for the file relative to
 *        $FG_ROOT; otherwise, look for the file relative to the
 *        current working directory.
 * @return true if the properties loaded successfully, false
 *         otherwise.
 */
extern bool fgLoadProps (const char * path, SGPropertyNode * props,
                         bool in_fg_root = true, int default_mode = 0);

void setLoggingClasses (const char * c);
void setLoggingPriority (const char * p);


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
extern SGPropertyNode * fgGetNode (const char * path, bool create = false);

/**
 * Get a property node.
 *
 * @param path The path of the node, relative to root.
 * @param create true to create the node if it doesn't exist.
 * @return The node, or 0 if none exists and none was created.
 */
inline SGPropertyNode * fgGetNode (const std::string & path, bool create = false)
{
    return fgGetNode(path.c_str(), create );
}


/**
 * Get a property node with separate index.
 *
 * This method separates the index from the path string, to make it
 * easier to iterate through multiple components without using sprintf
 * to add indices.  For example, fgGetNode("foo[1]/bar", 3) will
 * return the same result as fgGetNode("foo[1]/bar[3]").
 *
 * @param path The path of the node, relative to root.
 * @param index The index for the last member of the path (overrides
 * any given in the string).
 * @param create true to create the node if it doesn't exist.
 * @return The node, or 0 if none exists and none was created.
 */
extern SGPropertyNode * fgGetNode (const char * path,
				   int index, bool create = false);

/**
 * Get a property node with separate index.
 *
 * This method separates the index from the path string, to make it
 * easier to iterate through multiple components without using sprintf
 * to add indices.  For example, fgGetNode("foo[1]/bar", 3) will
 * return the same result as fgGetNode("foo[1]/bar[3]").
 *
 * @param path The path of the node, relative to root.
 * @param index The index for the last member of the path (overrides
 * any given in the string).
 * @param create true to create the node if it doesn't exist.
 * @return The node, or 0 if none exists and none was created.
 */
inline SGPropertyNode * fgGetNode (const std::string & path,
				   int index, bool create = false)
{
    return fgGetNode(path.c_str(), index, create );
}


/**
 * Test whether a given node exists.
 *
 * @param path The path of the node, relative to root.
 * @return true if the node exists, false otherwise.
 */
extern bool fgHasNode (const char * path);

/**
 * Test whether a given node exists.
 *
 * @param path The path of the node, relative to root.
 * @return true if the node exists, false otherwise.
 */
inline bool fgHasNode (const std::string & path)
{
    return fgHasNode( path.c_str() );
}


/**
 * Add a listener to a node.
 *
 * @param listener The listener to add to the node.
 * @param path The path of the node, relative to root.
 * @param index The index for the last member of the path (overrides
 * any given in the string).
 */
extern void fgAddChangeListener (SGPropertyChangeListener * listener,
				 const char * path);

/**
 * Add a listener to a node.
 *
 * @param listener The listener to add to the node.
 * @param path The path of the node, relative to root.
 * @param index The index for the last member of the path (overrides
 * any given in the string).
 */
inline void fgAddChangeListener (SGPropertyChangeListener * listener,
				 const std::string & path)
{
    fgAddChangeListener( listener, path.c_str() );
}


/**
 * Add a listener to a node.
 *
 * @param listener The listener to add to the node.
 * @param path The path of the node, relative to root.
 * @param index The index for the last member of the path (overrides
 * any given in the string).
 */
extern void fgAddChangeListener (SGPropertyChangeListener * listener,
				 const char * path, int index);

/**
 * Add a listener to a node.
 *
 * @param listener The listener to add to the node.
 * @param path The path of the node, relative to root.
 * @param index The index for the last member of the path (overrides
 * any given in the string).
 */
inline void fgAddChangeListener (SGPropertyChangeListener * listener,
				 const std::string & path, int index)
{
    fgAddChangeListener( listener, path.c_str(), index );
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
extern bool fgGetBool (const char * name, bool defaultValue = false);

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
inline bool fgGetBool (const std::string & name, bool defaultValue = false)
{
    return fgGetBool( name.c_str(), defaultValue );
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
extern int fgGetInt (const char * name, int defaultValue = 0);

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
inline int fgGetInt (const std::string & name, int defaultValue = 0)
{
    return fgGetInt( name.c_str(), defaultValue );
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
extern int fgGetLong (const char * name, long defaultValue = 0L);

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
inline int fgGetLong (const std::string & name, long defaultValue = 0L)
{
    return fgGetLong( name.c_str(), defaultValue );
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
extern float fgGetFloat (const char * name, float defaultValue = 0.0);

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
inline float fgGetFloat (const std::string & name, float defaultValue = 0.0)
{
    return fgGetFloat( name.c_str(), defaultValue );
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
extern double fgGetDouble (const char * name, double defaultValue = 0.0);

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
inline double fgGetDouble (const std::string & name, double defaultValue = 0.0)
{
    return fgGetDouble( name.c_str(), defaultValue );
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
extern const char * fgGetString (const char * name,
				 const char * defaultValue = "");

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
inline const char * fgGetString (const std::string & name,
                                 const std::string & defaultValue = std::string(""))
{
    return fgGetString( name.c_str(), defaultValue.c_str() );
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
extern bool fgSetBool (const char * name, bool val);

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
inline bool fgSetBool (const std::string & name, bool val)
{
    return fgSetBool( name.c_str(), val );
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
extern bool fgSetInt (const char * name, int val);

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
inline bool fgSetInt (const std::string & name, int val)
{
    return fgSetInt( name.c_str(), val );
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
extern bool fgSetLong (const char * name, long val);

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
inline bool fgSetLong (const std::string & name, long val)
{
    return fgSetLong( name.c_str(), val );
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
extern bool fgSetFloat (const char * name, float val);

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
inline bool fgSetFloat (const std::string & name, float val)
{
    return fgSetFloat( name.c_str(), val );
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
extern bool fgSetDouble (const char * name, double val);

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
inline bool fgSetDouble (const std::string & name, double val)
{
    return fgSetDouble( name.c_str(), val );
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
extern bool fgSetString (const char * name, const char * val);

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
inline bool fgSetString (const std::string & name, const std::string & val)
{
    return fgSetString( name.c_str(), val.c_str() );
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
extern void fgSetArchivable (const char * name, bool state = true);


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
extern void fgSetReadable (const char * name, bool state = true);


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
extern void fgSetWritable (const char * name, bool state = true);



////////////////////////////////////////////////////////////////////////
// Convenience functions for tying properties, with logging.
////////////////////////////////////////////////////////////////////////


/**
 * Untie a property from an external data source.
 *
 * Classes should use this function to release control of any
 * properties they are managing.
 */
extern void fgUntie (const char * name);


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
fgTie (const char * name, V (*getter)(), void (*setter)(V) = 0,
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
fgTie (const char * name, int index, V (*getter)(int),
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
fgTie (const char * name, T * obj, V (T::*getter)() const,
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
fgTie (const char * name, T * obj, int index,
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


class FGMakeUpperCase : public SGPropertyChangeListener {
public:
    void valueChanged(SGPropertyNode *node) {
        if (node->getType() != simgear::props::STRING)
            return;

        char *s = const_cast<char *>(node->getStringValue());
        for (; *s; s++)
            *s = toupper(*s);
    }
};


#endif // __FG_PROPS_HXX

