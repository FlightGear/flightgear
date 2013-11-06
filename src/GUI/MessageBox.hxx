#ifndef FG_GUI_MESSAGE_BOX_HXX
#define FG_GUI_MESSAGE_BOX_HXX

#include <string>

namespace flightgear
{

enum MessageBoxResult 
{
    MSG_BOX_OK,
    MSG_BOX_YES,
    MSG_BOX_NO
};

MessageBoxResult modalMessageBox(const std::string& caption,
    const std::string& msg,
    const std::string& moreText = std::string());

MessageBoxResult fatalMessageBox(const std::string& caption,
    const std::string& msg,
    const std::string& moreText = std::string());
                
} // of namespace flightgear

#endif // of FG_GUI_MESSAGE_BOX_HXX
