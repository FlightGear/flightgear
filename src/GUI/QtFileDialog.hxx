#ifndef FG_QT_FILE_DIALOG_HXX
#define FG_QT_FILE_DIALOG_HXX 1

#include <GUI/FileDialog.hxx>

class QtFileDialog : public FGFileDialog
{
public:
    QtFileDialog(FGFileDialog::Usage use);

    virtual ~QtFileDialog();
    
    virtual void exec();
    virtual void close();
};
#endif
