// WindowsFileDialog.hxx - file dialog implemented using Windows

#ifndef FG_WINDOWS_FILE_DIALOG_HXX
#define FG_WINDOWS_FILE_DIALOG_HXX 1

#include <GUI/FileDialog.hxx>

class WindowsFileDialog : public FGFileDialog
{
public:
    WindowsFileDialog(FGFileDialog::Usage use);
    
    virtual ~WindowsFileDialog();
    
    virtual void exec();
    virtual void close();
private:
	void chooseDir();

};

#endif // FG_WINDOWS_FILE_DIALOG_HXX
