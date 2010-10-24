// new_gui.hxx - XML-configured GUI subsystem.

#ifndef __NEW_GUI_HXX
#define __NEW_GUI_HXX 1

#include <plib/pu.h>

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/misc/sg_path.hxx>

#include <functional>
#include <vector>
#include <map>

#include <Main/fg_props.hxx>

class SGBinding;

class FGMenuBar;
class FGDialog;
class FGColor;
class FGFontCache;


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
     * Reinitialize the GUI subsystem. Reloads all XML dialogs.
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

private:
    struct ltstr
    {
        bool operator()(const char* s1, const char* s2) const {
            return strcmp(s1, s2) < 0;
        }
    };

    puFont *_font;
    map<const char*,FGColor*, ltstr> _colors;
    typedef map<const char*,FGColor*, ltstr>::iterator _itt_t;
    typedef map<const char*,FGColor*, ltstr>::const_iterator _citt_t;

    void clear_colors();

    // Read all the configuration files in a directory.
    void readDir (const SGPath& path);

    FGMenuBar * _menubar;
    FGDialog * _active_dialog;
    std::map<std::string,FGDialog *> _active_dialogs;
    std::map<std::string,SGPropertyNode_ptr> _dialog_props;

};


class FGColor {
public:
    FGColor() { clear(); }
    FGColor(float r, float g, float b, float a = 1.0f) { set(r, g, b, a); }
    FGColor(const SGPropertyNode *prop) { set(prop); }
    FGColor(FGColor *c) { 
        if (c) set(c->_red, c->_green, c->_blue, c->_alpha);
    }

    inline void clear() { _red = _green = _blue = _alpha = -1.0f; }
    // merges in non-negative components from property with children <red> etc.
    bool merge(const SGPropertyNode *prop);
    bool merge(const FGColor *color);

    bool set(const SGPropertyNode *prop) { clear(); return merge(prop); };
    bool set(const FGColor *color) { clear(); return merge(color); }
    bool set(float r, float g, float b, float a = 1.0f) {
        _red = r, _green = g, _blue = b, _alpha = a;
        return true;
    }
    bool isValid() const {
        return _red >= 0.0 && _green >= 0.0 && _blue >= 0.0;
    }
    void print() const;

    inline void setRed(float red) { _red = red; }
    inline void setGreen(float green) { _green = green; }
    inline void setBlue(float blue) { _blue = blue; }
    inline void setAlpha(float alpha) { _alpha = alpha; }

    inline float red() const { return clamp(_red); }
    inline float green() const { return clamp(_green); }
    inline float blue() const { return clamp(_blue); }
    inline float alpha() const { return _alpha < 0.0 ? 1.0 : clamp(_alpha); }

protected:
    float _red, _green, _blue, _alpha;

private:
    float clamp(float f) const { return f < 0.0 ? 0.0 : f > 1.0 ? 1.0 : f; }
};



/**
 * A class to keep all fonts available for future use.
 * This also assures a font isn't resident more than once.
 */
class FGFontCache {
private:
    // The parameters of a request to the cache.
    struct FntParams
    {
        const std::string name;
        const float size;
        const float slant;
        FntParams() : size(0.0f), slant(0.0f) {}
        FntParams(const FntParams& rhs)
            : name(rhs.name), size(rhs.size), slant(rhs.slant)
        {
        }
        FntParams(const std::string& name_, float size_, float slant_)
            : name(name_), size(size_), slant(slant_)
        {
        }
    };
    struct FntParamsLess
        : public std::binary_function<const FntParams, const FntParams, bool>
    {
        bool operator() (const FntParams& f1, const FntParams& f2) const;
    };
    struct fnt {
        fnt(puFont *pu = 0) : pufont(pu), texfont(0) {}
        ~fnt() { if (texfont) { delete pufont; delete texfont; } }
        // Font used by plib GUI code
        puFont *pufont;
        // TXF font
        fntTexFont *texfont;
    };
    // Path to the font directory
    SGPath _path;

    typedef map<const string, fntTexFont*> TexFontMap;
    typedef map<const FntParams, fnt*, FntParamsLess> PuFontMap;
    TexFontMap _texFonts;
    PuFontMap _puFonts;

    bool _initialized;
    struct fnt *getfnt(const char *name, float size, float slant);
    void init();

public:
    FGFontCache();
    ~FGFontCache();

    puFont *get(const char *name, float size=15.0, float slant=0.0);
    puFont *get(SGPropertyNode *node);

    fntTexFont *getTexFont(const char *name, float size=15.0, float slant=0.0);

    SGPath getfntpath(const char *name);
    /**
     * Preload all the fonts in the FlightGear font directory. It is
     * important to load the font textures early, with the proper
     * graphics context current, so that no plib (or our own) code
     * tries to load a font from disk when there's no current graphics
     * context.
     */
    bool initializeFonts();
};


#endif // __NEW_GUI_HXX

