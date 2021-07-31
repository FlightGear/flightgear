#ifndef FG_GUI_MESSAGE_BOX_HXX
#define FG_GUI_MESSAGE_BOX_HXX

#include <string>
#include <cstdlib>

namespace flightgear
{

// set a global value indicating we're in headless mode.
void setHeadlessMode(bool headless);
bool isHeadlessMode();

// special exception class used to signal an exit. Must not inherit
// std::exception or similar, since we want to handle it specially
class FatalErrorException
{
};

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
    const std::string& moreText = std::string(),
    bool reportToSentry = true);

[[noreturn]] void fatalMessageBoxThenExit(
    const std::string& caption,
    const std::string& msg,
    const std::string& moreText = std::string(),
    int exitStatus = EXIT_FAILURE,
    bool reportToSentry = true);


} // of namespace flightgear

#endif // of FG_GUI_MESSAGE_BOX_HXX
