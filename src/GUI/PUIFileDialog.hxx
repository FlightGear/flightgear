// PUIFileDialog.hxx - file dialog implemented using PUI

#ifndef FG_PUI_FILE_DIALOG_HXX
#define FG_PUI_FILE_DIALOG_HXX 1

#include <simgear/props/props.hxx>
#include <GUI/FileDialog.hxx>

class PUIFileDialog : public FGFileDialog
{
public:
    PUIFileDialog(FGFileDialog::Usage use);

    virtual ~PUIFileDialog();
    
    virtual void exec();
    virtual void close();
private:
    class PathListener;
    friend class PathListener;
    
    // called by the listener
    void pathChanged(const SGPath& aPath);
    
    
    SGPropertyNode_ptr _dialogRoot;
    PathListener* _listener;
};

#endif // FG_PUI_FILE_DIALOG_HXX
