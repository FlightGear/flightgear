// FileDialog.hxx - abstract inteface for a file open/save dialog

#ifndef FG_GUI_FILE_DIALOG_HXX
#define FG_GUI_FILE_DIALOG_HXX 1

#include <memory> // for std::unique_ptr

#include <simgear/misc/strutils.hxx> // for string_list
#include <simgear/misc/sg_path.hxx>

#include <simgear/nasal/cppbind/NasalCallContext.hxx>

// forward decls
class SGPropertyNode;

class FGFileDialog
{
public:
    typedef enum {
        USE_OPEN_FILE = 0,
        USE_SAVE_FILE,
        USE_CHOOSE_DIR
    } Usage;
    
    std::string getTitle() const
    { return _title; }
    
    void setTitle(const std::string& aTitle);

    std::string getButton() const
    { return _buttonText; }
    
    void setButton(const std::string& aText);

    SGPath getDirectory() const
    { return _initialPath; }
    
    void setDirectory(const SGPath& aPath);
    
    string_list filterPatterns() const
    { return _filterPatterns; }
    
    void setFilterPatterns(const string_list& patterns);
    
    /// for saving
    std::string getPlaceholder() const
    { return _placeholder; }
    
    void setPlaceholderName(const std::string& aName);

    bool showHidden() const
    { return _showHidden; }
    void setShowHidden(bool show);
    
    /**
     * Destructor.
     */
    virtual ~FGFileDialog ();

    virtual void exec() = 0;
    virtual void close() = 0;
    
    class Callback
    {
      public:
        virtual ~Callback() { }
        virtual void onFileDialogDone(FGFileDialog* ins, const SGPath& result) = 0;
    };

    virtual void setCallback(Callback* aCB);

    void setCallbackFromNasal(const nasal::CallContext& ctx);
protected:
    FGFileDialog(Usage use);
    
    const Usage _usage;
    std::string _title, _buttonText;
    SGPath _initialPath;
    string_list _filterPatterns;
    std::string _placeholder;
    bool _showHidden;
    std::unique_ptr<Callback> _callback;
};

#endif // FG_GUI_FILE_DIALOG_HXX
