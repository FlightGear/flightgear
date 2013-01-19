// CocoaFileDialog.hxx - file dialog implemented using Cocoa

#ifndef FG_COCOA_FILE_DIALOG_HXX
#define FG_COCOA_FILE_DIALOG_HXX 1

#include <GUI/FileDialog.hxx>

class CocoaFileDialog : public FGFileDialog
{
public:
    CocoaFileDialog(FGFileDialog::Usage use);
    
    virtual ~CocoaFileDialog();
    
    virtual void exec();
    virtual void close();
private:
    class CocoaFileDialogPrivate;
    std::auto_ptr<CocoaFileDialogPrivate> d;
};

#endif // FG_COCOA_FILE_DIALOG_HXX
