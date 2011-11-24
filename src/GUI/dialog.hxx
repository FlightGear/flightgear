// dialog.hxx - XML-configured dialog box.

#ifndef __DIALOG_HXX
#define __DIALOG_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

// forward decls
class SGPropertyNode;

/**
 * An XML-configured dialog box.
 *
 * The GUI manager stores only the property tree for the dialog
 * boxes.  This class creates a PUI dialog box on demand from
 * the properties in that tree.  The manager recreates the dialog
 * every time it needs to show it.
 */
class FGDialog
{
public:



    /**
     * Destructor.
     */
    virtual ~FGDialog ();


    /**
     * Update the values of all GUI objects with a specific name,
     * or all if name is 0 (default).
     *
     * This method copies values from the FlightGear property tree to
     * the GUI object(s).
     *
     * @param objectName The name of the GUI object(s) to update.
     *        Use the empty name for all unnamed objects.
     */
    virtual void updateValues (const char * objectName = 0) = 0;


    /**
     * Apply the values of all GUI objects with a specific name,
     * or all if name is 0 (default)
     *
     * This method copies values from the GUI object(s) to the
     * FlightGear property tree.
     *
     * @param objectName The name of the GUI object(s) to update.
     *        Use the empty name for all unnamed objects.
     */
    virtual void applyValues (const char * objectName = 0) = 0;


    /**
     * Update state.  Called on active dialogs before rendering.
     */
    virtual void update () = 0;

protected:
    /**
     * Construct a new GUI widget configured by a property tree.
     *
     * The configuration properties are not part of the main
     * FlightGear property tree; the GUI manager reads them
     * from individual configuration files.
     *
     * @param props A property tree describing the dialog.
     */
    FGDialog (SGPropertyNode * props);

private:

};

#endif // __DIALOG_HXX
