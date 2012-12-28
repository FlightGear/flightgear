// CocoaFileDialog.hxx - file dialog implemented using Cocoa

#ifndef FG_COCOA_FILE_DIALOG_HXX
#define FG_COCOA_FILE_DIALOG_HXX 1

#include <GUI/FileDialog.hxx>

class CocoaFileDialog : public FGFileDialog
{
public:
    CocoaFileDialog(const std::string& aTitle, FGFileDialog::Usage use);
    
    virtual ~CocoaFileDialog();
    
    virtual void exec();
    
private:
    class CocoaFileDialogPrivate;
    std::auto_ptr<CocoaFileDialogPrivate> d;
};

#endif // FG_COCOA_FILE_DIALOG_HXX
