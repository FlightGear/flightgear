// dialog.hxx - XML-configurable dialog box.

#ifndef __DIALOG_HXX
#define __DIALOG_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <plib/pu.h>

#include <simgear/compiler.h>	// for SG_USING_STD
#include <simgear/misc/props.hxx>

#include <vector>
SG_USING_STD(vector);

class FGDialog;
class FGBinding;


/**
 * PUI userdata describing a GUI object.
 */
struct GUIInfo
{
    GUIInfo (FGDialog * w);
    virtual ~GUIInfo ();
    
    FGDialog * widget;
    vector <FGBinding *> bindings;
};


/**
 * Top-level GUI widget.
 */
class FGDialog
{
public:


    /**
     * Construct a new GUI widget configured by a property tree.
     */
    FGDialog (SGPropertyNode_ptr props);


    /**
     * Destructor.
     */
    virtual ~FGDialog ();


    /**
     * Update the values of all GUI objects with a specific name.
     *
     * This method copies from the property to the GUI object.
     *
     * @param objectName The name of the GUI object(s) to update.
     *        Use the empty name for all unnamed objects.
     */
    virtual void updateValue (const char * objectName);


    /**
     * Apply the values of all GUI objects with a specific name.
     *
     * This method copies from the GUI object to the property.
     *
     * @param objectName The name of the GUI object(s) to update.
     *        Use the empty name for all unnamed objects.
     */
    virtual void applyValue (const char * objectName);


    /**
     * Update the values of all GUI objects.
     *
     * This method copies from the properties to the GUI objects.
     */
    virtual void updateValues ();


    /**
     * Apply the values of all GUI objects.
     *
     * This method copies from the GUI objects to the properties.
     */
    virtual void applyValues ();


private:
    FGDialog (const FGDialog &); // just for safety

    void display (SGPropertyNode_ptr props);
    puObject * makeObject (SGPropertyNode * props,
                           int parentWidth, int parentHeight);
    void setupObject (puObject * object, SGPropertyNode * props);
    void setupGroup (puGroup * group, SGPropertyNode * props,
                     int width, int height, bool makeFrame = false);

    puObject * _object;
    vector<GUIInfo *> _info;
    struct PropertyObject {
        PropertyObject (const char * name,
                        puObject * object,
                        SGPropertyNode_ptr node);
        string name;
        puObject * object;
        SGPropertyNode_ptr node;
    };
    vector<PropertyObject *> _propertyObjects;
};

#endif // __DIALOG_HXX
