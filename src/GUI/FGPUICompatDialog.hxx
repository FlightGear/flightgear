// SPDX-FileName: FGPUICompatDialog.hxx
// SPDX-FileComment: XML dialog class without using PUI
// SPDX-FileCopyrightText: Copyright (C) 2022 James Turner
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>

#include "dialog.hxx"


#include <simgear/misc/sg_path.hxx>
#include <simgear/nasal/cppbind/NasalHash.hxx>
#include <simgear/nasal/cppbind/NasalObject.hxx>
#include <simgear/props/condition.hxx>
#include <simgear/props/props.hxx>


class NewGUI;
class FGColor;

class PUICompatObject;
using PUICompatObjectRef = SGSharedPtr<PUICompatObject>;

/**
 * An XML-configured dialog box.
 *
 * The GUI manager stores only the property tree for the dialog
 * boxes.  This class creates a PUI dialog box on demand from
 * the properties in that tree.  The manager recreates the dialog
 * every time it needs to show it.
 */
class FGPUICompatDialog : public FGDialog
{
public:
    static void setupGhost(nasal::Hash& compatModule);

    /**
     * Construct a new GUI widget configured by a property tree.
     *
     * The configuration properties are not part of the main
     * FlightGear property tree; the GUI manager reads them
     * from individual configuration files.
     *
     * @param props A property tree describing the dialog.
     */
    FGPUICompatDialog(SGPropertyNode* props);


    /**
     * Destructor.
     */
    virtual ~FGPUICompatDialog();


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
    virtual void updateValues(const std::string& objectName = "");


    /**
     * Apply the values of all GUI objects with a specific name,
     * or all if an empty name is given (default).
     *
     * This method copies values from the GUI object(s) to the
     * FlightGear property tree.
     *
     * @param objectName The name of the GUI object(s) to update.
     *        Use the empty name for all objects.
     */
    virtual void applyValues(const std::string& objectName = "");

    bool init();

    /**
     * Update state.  Called on active dialogs before rendering.
     */
    void update() override;

    /**
     * Recompute the dialog's layout
     */
    void relayout();


    void setNeedsLayout()
    {
        _needsRelayout = true;
    }

    virtual const char* getName();
    virtual void bringToFront();

    std::string nameString() const;
    std::string nasalModule() const;

    SGRectd geometry() const;

    double getX() const;
    double getY() const;
    double width() const;
    double height() const;

    void close() override;

    std::string title() const;
    void setTitle(const std::string& s);

    const std::string& windowType() const
    {
        return _windowType;
    }

    /**
     * @brief return the UI XML syntax version used by this dialog. 
     * 
     * 0 = no version specified explicitly, 1 = compatible with PUI dialogs in older
     * versions of FlightGear. Higher numbers indicate features than only work with the
     * updated XML UI.
     */
    uint32_t uiVersion() const
    {
        return _uiVersion;
    }

    /**
     * @brief find the dialog widget with the specified name, or nullptr.
     * 
     */
    PUICompatObjectRef widgetByName(const std::string& name) const;

private:
    friend naRef f_makeDialogPeer(const nasal::CallContext& ctx);
    friend naRef f_dialogRootObject(FGPUICompatDialog& dialog, naContext c);

    // Show the dialog.
    void display(SGPropertyNode* props);

    void requestClose();

    // return key code number for keystring
    int getKeyCode(const char* keystring);

    // The source xml tree, so that we can pass data back, such as the
    // last position.
    SGPropertyNode_ptr _props;

    bool _needsRelayout;

    // Nasal module.
    std::string _module;
    SGPropertyNode_ptr _nasal_close;

    class DialogPeer;

    SGSharedPtr<DialogPeer> _peer;

    std::string _windowType; ///< eg a dialog, an overlay, a modal dialog
    std::string _name;
    PUICompatObjectRef _root;
    std::string _title;
    uint32_t _uiVersion = 0;
};
