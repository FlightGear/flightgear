// PUICompatDialog.hxx - XML dialog object without using PUI
// Copyright (C) 2022 James Turner
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <simgear/math/SGMath.hxx>
#include <simgear/nasal/cppbind/NasalHash.hxx>
#include <simgear/nasal/cppbind/NasalObject.hxx>
#include <simgear/props/propsfwd.hxx>
#include <simgear/structure/SGBinding.hxx>
#include <simgear/structure/SGReferenced.hxx>

class PUICompatObject;
class FGPUICompatDialog;

using PUICompatObjectRef = SGSharedPtr<PUICompatObject>;
using PUICompatObjectVec = std::vector<PUICompatObjectRef>;

using PUICompatDialogRef = SGSharedPtr<FGPUICompatDialog>;

class PUICompatObject : public nasal::Object, public SGPropertyChangeListener
{
public:
    static PUICompatObjectRef createForType(const std::string& type, SGPropertyNode_ptr config);

    static void setupGhost(nasal::Hash& guiModule);

    virtual ~PUICompatObject();

    virtual void init();

    virtual void update();

    virtual void apply();

    naRef config() const;
    
    /// return the wrapped props,Node corresponding to our property
    naRef property() const;
    
    /// return the actual Nasal value of our property: this avoids the need to
    /// create a the property ghost and props.Node wrapper in common cases
    naRef propertyValue(naContext ctx) const;

    PUICompatObjectRef parent() const;

    PUICompatDialogRef dialog() const;

    PUICompatObjectVec children() const;

    naRef show(naRef viewParent);

    double getX() const;
    double getY() const;
    double width() const;
    double height() const;

    SGRectd geometry() const;

    // bool heightForWidth properties

    void setGeometry(const SGRectd& g);

    bool visible() const;
    bool enabled() const;
    const std::string& type() const;

    void setVisible(bool v);
    void setEnabled(bool e);

protected:
    PUICompatObject(naRef impl, const std::string& type);

    virtual void activateBindings();

    virtual void updateGeometry(const SGRectd& newGeom);

    void valueChanged(SGPropertyNode* node) override;

    // emporary solution to decide which SGPropertyNode children of an
    // object, are children
    static bool isNodeAChildObject(const std::string& nm);

private:
    enum class LiveValueMode {
        OnApply,  ///< not live, only update on explicit apply()
        Listener, ///< live, via SGPropertyListener::valueChanged
        Polled    ///< live, used for tied, non-listener-safe properties
    };

    friend class FGPUICompatDialog;

    friend naRef f_makeCompatObjectPeer(const nasal::CallContext& ctx);
    naRef nasalGetConfigValue(const nasal::CallContext ctx) const;

    void setDialog(PUICompatDialogRef dialog);

    void recursiveUpdate(const std::string& objectName = {});
    void recursiveApply(const std::string& objectName = {});
    void recursiveOnDelete();

    void doActivate();
    
    nasal::Hash gridLocation(const nasal::CallContext& ctx) const;

    SGWeakPtr<PUICompatObject> _parent;
    SGWeakPtr<FGPUICompatDialog> _dialog;

    PUICompatObjectVec _children; // owning references to children

    SGPropertyNode_ptr _config;

    std::string _type;
    std::string _label;
    std::string _name;
    std::string _oldPolledValue;

    SGPropertyNode_ptr _value;
    SGRectd _geometry;

    LiveValueMode _live = LiveValueMode::OnApply;
    bool _valueChanged = false;
    bool _visible = true;
    bool _enabled = true;
    bool _labelChanged = false;

    SGConditionRef _visibleCondition;
    SGConditionRef _enableCondition;

    SGBindingList _bindings;
};
