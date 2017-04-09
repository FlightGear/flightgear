#ifndef FG_GUI_MESSAGE_BOX_HXX
#define FG_GUI_MESSAGE_BOX_HXX

#include <string>
#include <cstdlib>

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

MessageBoxResult fatalMessageBoxWithoutExit(
    const std::string& caption,
    const std::string& msg,
    const std::string& moreText = std::string());

[[noreturn]] void fatalMessageBoxThenExit(
    const std::string& caption,
    const std::string& msg,
    const std::string& moreText = std::string(),
    int exitStatus = EXIT_FAILURE);


} // of namespace flightgear

#endif // of FG_GUI_MESSAGE_BOX_HXX
