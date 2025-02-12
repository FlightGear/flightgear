// dialog.cxx: implementation of an XML-configurable dialog box.

#include "config.h"

#include "dialog.hxx"

#include <simgear/props/props.hxx>

namespace {

using WS = FGDialog::WindowStyle;
using WF = FGDialog::WindowFlags;

FGDialog::WindowStyle styleFromProps(const std::string& s)
{
    if (s == "modal-dialag") {
        return WS::ModalDialog;
    }

    if (s == "message-box") {
        return WS::MessageBox;
    }

    return WS::Window;
}

int defaultFlagsForStyle(FGDialog::WindowStyle ws)
{
    switch (ws) {
    case WS::ModalDialog:
        return WF::ButtonBox;

    case WS::MessageBox:
        return WF::ButtonBox;

    default:
        return WF::Resizable | WF::Closeable;
    }
}

} // namespace

FGDialog::FGDialog(SGPropertyNode* props) : _windowStyle(styleFromProps(props->getStringValue("window-style")))
{
    _flags = defaultFlagsForStyle(_windowStyle);
    updateFlagFromProperty(WF::Closeable, props, "closeable");
    updateFlagFromProperty(WF::Resizable, props, "resizeable");
    updateFlagFromProperty(WF::ButtonBox, props, "has-buttons");
}

void FGDialog::updateFlagFromProperty(WindowFlags f, SGPropertyNode* props, const std::string& name)
{
    auto c = props->getChild(name);
    if (!c) {
        return;
    }

    const auto invF = ~f;
    _flags &= invF; // clear to zero
    if (c->getBoolValue()) {
        _flags |= f;
    }
}


FGDialog::~FGDialog() = default;

FGDialog::WindowStyle FGDialog::windowStyle() const
{
    return _windowStyle;
}

bool FGDialog::isFlagSet(WindowFlags f) const
{
    return _flags & f;
}
