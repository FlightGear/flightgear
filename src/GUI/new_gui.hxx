// new_gui.hxx - XML-configured GUI subsystem.

#ifndef __NEW_GUI_HXX
#define __NEW_GUI_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <plib/pu.h>

#include <simgear/compiler.h>	// for SG_USING_STD
#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>

#include <vector>
SG_USING_STD(vector);

#include <map>
SG_USING_STD(map);

#include <Main/fg_props.hxx>

class FGMenuBar;
class FGDialog;
class FGBinding;


/**
 * XML-configured GUI subsystem.
 *
 * This subsystem manages the graphical user interface for FlightGear.
 * It creates a menubar from the XML configuration file in
 * $FG_ROOT/gui/menubar.xml, then stores the configuration properties
 * for XML-configured dialog boxes found in $FG_ROOT/gui/dialogs/.  It
 * can show or hide the menubar, and can display any dialog by name.
 */
class NewGUI : public SGSubsystem
{
public:

    /**
     * Constructor.
     */
    NewGUI ();

    /**
     * Destructor.
     */
    virtual ~NewGUI ();

    /**
     * Initialize the GUI subsystem.
     */
    virtual void init ();

    /**
     * Reinitialize the GUI subsystem.
     */
    virtual void reinit ();

    /**
     * Bind properties for the GUI subsystem.
     *
     * Currently, this method binds the properties for showing and
     * hiding the menu.
     */
    virtual void bind ();

    /**
     * Unbind properties for the GUI subsystem.
     */
    virtual void unbind ();

    /**
     * Update the GUI subsystem.
     *
     * Currently, this method is a no-op, because nothing the GUI
     * subsystem does is time-dependent.
     */
    virtual void update (double delta_time_sec);

    /**
     * Creates a new dialog box, using the same property format as the
     * gui/dialogs configuration files.  Does not display the
     * resulting dialog.  If a pre-existing dialog of the same name
     * exists, it will be deleted.  The node argument will be stored
     * in the GUI subsystem using SGPropertNode_ptr reference counting.
     * It should not be deleted by user code.
     *
     * @param node A property node containing the dialog definition
     */
    virtual void newDialog (SGPropertyNode* node);

    /**
     * Display a dialog box.
     *
     * At initialization time, the subsystem reads all of the XML
     * configuration files from the directory $FG_ROOT/gui/dialogs/.
     * The configuration for each dialog specifies a name, and this
     * method invokes the dialog with the name specified (if it
     * exists).
     *
     * @param name The name of the dialog box.
     * @return true if the dialog exists, false otherwise.
     */
    virtual bool showDialog (const string &name);


    /**
     * Close the currenty active dialog.  This function is intended to
     * be called from code (pui callbacks, for instance) that registers
     * its dialog object as active via setActiveDialog().  Other
     * user-level code should use the closeDialog(name) API.
     *
     * @return true if a dialog was active, false otherwise
     */
    virtual bool closeActiveDialog ();

    /**
     * Close a named dialog, if it is open.
     *
     * @param name The name of the dialog box.
     * @return true if the dialog was active, false otherwise.
     */
    virtual bool closeDialog (const string &name);

    /**
     * Return a pointer to the current menubar.
     */
    virtual FGMenuBar * getMenuBar ();


    /**
     * Ignore this method.
     *
     * This method is for internal use only, but it has to be public
     * so that a non-class callback can see it.
     */
    virtual void setActiveDialog (FGDialog * dialog);

    /**
     * Get the dialog currently active, if any.
     *
     * @return The active dialog, or 0 if none is active.
     */
    virtual FGDialog * getActiveDialog ();

protected:

    /**
     * Test if the menubar is visible.
     *
     * This method exists only for binding.
     */
    virtual bool getMenuBarVisible () const;

    /**
     * Show or hide the menubar.
     *
     * This method exists only for binding.
     */
    virtual void setMenuBarVisible (bool visible);


private:

    // Free all allocated memory.
    void clear ();

    // Read all the configuration files in a directory.
    void readDir (const char * path);

    FGMenuBar * _menubar;
    FGDialog * _active_dialog;
    map<string,FGDialog *> _active_dialogs;
    map<string,SGPropertyNode_ptr> _dialog_props;

};


#endif // __NEW_GUI_HXX

