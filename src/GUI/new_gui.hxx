// new_gui.hxx - XML-configured GUI subsystem.

#ifndef __NEW_GUI_HXX
#define __NEW_GUI_HXX 1

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/misc/sg_path.hxx>

#include <vector>
#include <map>
#include <memory> // for unique_ptr on some systems
#include <cstring> // for strcmp in lstr() (in this header, alas)

class FGMenuBar;
class FGDialog;
class FGColor;
class FGFontCache;
class puFont;

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

    // Subsystem API.
    void bind() override;
    void init() override;
    void reinit() override;
    void shutdown() override;
    void unbind() override;
    void update(double delta_time_sec) override;

    // Subsystem identification.
    static const char* staticSubsystemClassId() { return "gui"; }

    /**
     * Redraw the GUI picking up new GUI colors.
     */
    virtual void redraw ();

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
    virtual bool showDialog (const std::string &name);

    /**
     * Toggle display of a dialog box.
     *
     * @param name The name of the dialog box.
     * @return true if the dialog is being displayed, false otherwise.
     */
    virtual bool toggleDialog (const std::string &name);


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
    virtual bool closeDialog (const std::string &name);

    /**
     * Get dialog property tree's root node.
     * @param name The name of the dialog box.
     * @return node pointer if the dialog was found, zero otherwise.
     */
    virtual SGPropertyNode_ptr getDialogProperties (const std::string &name);

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


    /**
     * Get the named dialog if active.
     *
     * @return The named dialog, or 0 if it isn't active.
     */
    virtual FGDialog * getDialog (const std::string &name);


    virtual FGColor *getColor (const char * name) const {
        _citt_t it = _colors.find(name);
        return (it != _colors.end()) ? it->second : NULL;
    }
    virtual FGColor *getColor (const std::string &name) const {
        _citt_t it = _colors.find(name.c_str());
        return (it != _colors.end()) ? it->second : NULL;
    }

    virtual puFont *getDefaultFont() { return _font; }

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

    virtual void setStyle ();
    virtual void setupFont (SGPropertyNode *);

    /**
     * Used by reinit() and redraw() to close all dialogs and to apply
     * current GUI colors. If "reload" is false, reopens all dialogs.
     * Otherwise reloads all XML dialog files from disk and reopens all
     * but Nasal * generated dialogs, omitting dynamic widgets. (This
     * is only useful for GUI development.)
     */
    virtual void reset (bool reload);

    bool getMenuBarOverlapHide() const;
    void setMenuBarOverlapHide(bool hide);

private:
    void createMenuBarImplementation();

    struct ltstr
    {
        bool operator()(const char* s1, const char* s2) const {
            return strcmp(s1, s2) < 0;
        }
    };

    puFont *_font;
    typedef std::map<const char*,FGColor*, ltstr> ColourDict;
    ColourDict _colors;
    typedef ColourDict::iterator _itt_t;
    typedef ColourDict::const_iterator _citt_t;

    void clear_colors();

    // Read all the configuration files in a directory.
    void readDir (const SGPath& path);

    std::unique_ptr<FGMenuBar> _menubar;
    FGDialog * _active_dialog;
    typedef std::map<std::string,FGDialog *> DialogDict;
    DialogDict _active_dialogs;

    typedef std::map<std::string, SGPath> NamePathDict;
    // mapping from dialog names to the corresponding XML property list
    // which defines them
    NamePathDict _dialog_names;

    // cache of loaded dialog proeprties
    typedef std::map<std::string,SGPropertyNode_ptr> NameDialogDict;
    NameDialogDict _dialog_props;
};

#endif // __NEW_GUI_HXX

