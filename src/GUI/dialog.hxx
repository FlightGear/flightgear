// dialog.hxx - XML-configured dialog box.
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>
#include <string>

#include <simgear/structure/SGWeakReferenced.hxx>

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
class FGDialog : public SGWeakReferenced
{
public:



    /**
     * Destructor.
     */
    virtual ~FGDialog ();


    /**
     * Update the values of all GUI objects with a specific name,
     * or all if an empty name is given (default).
     *
     * This method copies values from the FlightGear property tree to
     * the GUI object(s).
     *
     * @param objectName The name of the GUI object(s) to update.
     *        Use the empty name for all objects.
     */
    virtual void updateValues(const std::string& objectName = "") = 0;


    /**
     * Apply the values of all GUI objects with a specific name,
     * or all if an empty name is given (default)
     *
     * This method copies values from the GUI object(s) to the
     * FlightGear property tree.
     *
     * @param objectName The name of the GUI object(s) to update.
     *        Use the empty name for all objects.
     */
    virtual void applyValues(const std::string& objectName = "") = 0;


    /**
     * Update state.  Called on active dialogs before rendering.
     */
    virtual void update () = 0;

    virtual const char *getName() { return ""; }
    virtual void bringToFront() {}

    /**
     * @brief Close the dialog. This should actually close the GUI
     * assets associated, if you want an 'are you sure?' interaction, it
     * needs to be handled in advance of this interaction.
     */
    virtual void close() = 0;

    enum WindowStyle {
        Window,      // regular window with titlebar
        ModalDialog, // dialog (probably without a title-bar)
        MessageBox   // for alerts, yes/no choices, progress bars, etc
    };

    enum WindowFlags {
        Resizable = 1 << 0,
        Closeable = 1 << 1,
        ButtonBox = 1 << 2
    };

    WindowStyle windowStyle() const;
    bool isFlagSet(WindowFlags f) const;

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
    void updateFlagFromProperty(WindowFlags f, SGPropertyNode* props, const std::string& name);

    const WindowStyle _windowStyle;
    uint32_t _flags = 0;
};
