

#include "PUIFileDialog.hxx"

#include <boost/foreach.hpp>

#include <simgear/debug/logstream.hxx>
#include <simgear/props/props_io.hxx>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <GUI/new_gui.hxx>

class PUIFileDialog::PathListener : public SGPropertyChangeListener
{
public:
    PathListener(PUIFileDialog* dlg) :
        _dialog(dlg)
    { ; }
    
    virtual void valueChanged(SGPropertyNode* node)
    {
        _dialog->pathChanged(SGPath(node->getStringValue()));
    }
    
private:
    PUIFileDialog* _dialog;
};

PUIFileDialog::PUIFileDialog(Usage use) :
    FGFileDialog(use),
    _listener(NULL)
{
    SG_LOG(SG_GENERAL, SG_INFO, "created PUIFileDialog");
}

PUIFileDialog::~PUIFileDialog()
{
    if (_listener) {
        SGPropertyNode_ptr path = _dialogRoot->getNode("path");
        path->removeChangeListener(_listener);
        delete _listener;
    }
}

void PUIFileDialog::exec()
{
    NewGUI* gui = static_cast<NewGUI*>(globals->get_subsystem("gui"));
    std::string name("native-file-0");
    _dialogRoot = fgGetNode("/sim/gui/dialogs/" + name, true);
    
    SGPropertyNode_ptr dlg = _dialogRoot->getChild("dialog", 0, true);
    SGPath dlgXML = globals->resolve_resource_path("gui/dialogs/file-select.xml");
    readProperties(dlgXML.str(), dlg);
    
    dlg->setStringValue("name", name);
    gui->newDialog(dlg);
    
    _dialogRoot->setStringValue("title", _title);
    _dialogRoot->setStringValue("button", _buttonText);
    _dialogRoot->setStringValue("directory", _initialPath.str());
    _dialogRoot->setStringValue("selection", _placeholder);
    
// convert patterns vector into pattern nodes
    _dialogRoot->removeChildren("pattern");
    int index=0;
    BOOST_FOREACH(std::string pat, _filterPatterns) {
        _dialogRoot->getNode("pattern", index++, true)->setStringValue(pat);
    }
    
    _dialogRoot->setBoolValue("show-files", _usage != USE_CHOOSE_DIR);
    _dialogRoot->setBoolValue("dotfiles", _showHidden);
    
    if (!_listener) {
        _listener = new PathListener(this);
    }
    SGPropertyNode_ptr path = _dialogRoot->getNode("path", 0, true);
    path->addChangeListener(_listener);
    
    gui->showDialog(name);
}

void PUIFileDialog::close()
{
    NewGUI* gui = static_cast<NewGUI*>(globals->get_subsystem("gui"));
    std::string name("native-file-0");
    gui->closeDialog(name);
}

void PUIFileDialog::pathChanged(const SGPath& aPath)
{
    _callback->onFileDialogDone(this, aPath);
}
