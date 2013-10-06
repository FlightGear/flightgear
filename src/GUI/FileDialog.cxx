// FileDialog -- generic FileDialog interface and Nasal wrapper
//
// Written by James Turner, started 2012.
//
// Copyright (C) 2012 James Turner  <zakalawe@mac.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif


#include "FileDialog.hxx"

#include <boost/shared_ptr.hpp>

#include <simgear/nasal/cppbind/from_nasal.hxx>
#include <simgear/nasal/cppbind/to_nasal.hxx>
#include <simgear/nasal/cppbind/NasalHash.hxx>

#include <Main/globals.hxx>
#include <Scripting/NasalSys.hxx>
#include "PUIFileDialog.hxx"

#ifdef SG_MAC
    #include "CocoaFileDialog.hxx"
#endif

FGFileDialog::FGFileDialog(Usage use) :
    _usage(use),
    _showHidden(false)
{
    
}

FGFileDialog::~FGFileDialog()
{
    // ensure this is concrete so callback gets cleaned up.
}

void FGFileDialog::setTitle(const std::string& aText)
{
    _title = aText;
}

void FGFileDialog::setButton(const std::string& aText)
{
    _buttonText = aText;
}

void FGFileDialog::setDirectory(const SGPath& aPath)
{
    _initialPath = aPath;
}

void FGFileDialog::setFilterPatterns(const string_list& patterns)
{
    _filterPatterns = patterns;
}

void FGFileDialog::setPlaceholderName(const std::string& aName)
{
    _placeholder = aName;
}

void FGFileDialog::setCallback(Callback* aCB)
{
    _callback.reset(aCB);
}

void FGFileDialog::setShowHidden(bool show)
{
    _showHidden = show;
}

class NasalCallback : public FGFileDialog::Callback
{
public:
    NasalCallback(naRef f, naRef obj) :
        func(f),
        object(obj)
    {
        FGNasalSys* sys = static_cast<FGNasalSys*>(globals->get_subsystem("nasal"));
        _gcKeys[0] = sys->gcSave(f);
        _gcKeys[1] = sys->gcSave(obj);
    }
    
    virtual void onFileDialogDone(FGFileDialog* instance, const SGPath& aPath)
    {
        FGNasalSys* sys = static_cast<FGNasalSys*>(globals->get_subsystem("nasal"));
        naContext ctx = sys->context();
        
        naRef args[1];
        args[0] = nasal::to_nasal(ctx, aPath);
        
        sys->callMethod(func, object, 1, args, naNil() /* locals */);
    }
    
    ~NasalCallback()
    {
        FGNasalSys* sys = static_cast<FGNasalSys*>(globals->get_subsystem("nasal"));
        sys->gcRelease(_gcKeys[0]);
        sys->gcRelease(_gcKeys[1]);
    }
private:
    naRef func;
    naRef object;
    int _gcKeys[2];
};

void FGFileDialog::setCallbackFromNasal(const nasal::CallContext& ctx)
{
    // wrap up the naFunc in our callback type
    naRef func = ctx.requireArg<naRef>(0);
    naRef object = ctx.getArg<naRef>(1, naNil());
    
    setCallback(new NasalCallback(func, object));
}

typedef boost::shared_ptr<FGFileDialog> FileDialogPtr;
typedef nasal::Ghost<FileDialogPtr> NasalFileDialog;

/**
 * Create new FGFileDialog and get ghost for it.
 */
static naRef f_createFileDialog(naContext c, naRef me, int argc, naRef* args)
{
    nasal::CallContext ctx(c, argc, args);
    FGFileDialog::Usage usage = (FGFileDialog::Usage) ctx.requireArg<int>(0);
  
#ifdef SG_MAC
    FileDialogPtr fd(new CocoaFileDialog(usage));
#else
    FileDialogPtr fd(new PUIFileDialog(usage));
#endif
    
    return NasalFileDialog::create(c, fd);
}

void postinitNasalGUI(naRef globals, naContext c)
{
    if (!NasalFileDialog::isInit()) {
        NasalFileDialog::init("gui._FileDialog")
        .member("title", &FGFileDialog::getTitle,  &FGFileDialog::setTitle)
        .member("button", &FGFileDialog::getButton,  &FGFileDialog::setButton)
        .member("directory", &FGFileDialog::getDirectory, &FGFileDialog::setDirectory)
        .member("show_hidden", &FGFileDialog::showHidden, &FGFileDialog::setShowHidden)
        .member("placeholder", &FGFileDialog::getPlaceholder, &FGFileDialog::setPlaceholderName)
        .member("pattern", &FGFileDialog::filterPatterns, &FGFileDialog::setFilterPatterns)
        .method("open", &FGFileDialog::exec)
        .method("close", &FGFileDialog::close)
        .method("setCallback", &FGFileDialog::setCallbackFromNasal);
    }
    
    nasal::Hash guiModule = nasal::Hash(globals, c).get<nasal::Hash>("gui");
    
    guiModule.set("FILE_DIALOG_OPEN_FILE", (int) FGFileDialog::USE_OPEN_FILE);
    guiModule.set("FILE_DIALOG_SAVE_FILE", (int) FGFileDialog::USE_SAVE_FILE);
    guiModule.set("FILE_DIALOG_CHOOSE_DIR", (int) FGFileDialog::USE_CHOOSE_DIR);
    guiModule.set("_createFileDialog", f_createFileDialog);
}
