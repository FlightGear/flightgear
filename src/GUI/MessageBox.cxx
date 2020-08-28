#ifdef HAVE_CONFIG_H
    #include "config.h"
#endif

#include <simgear/simgear_config.h>

#include <cstdlib>

#include "MessageBox.hxx"

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Viewer/renderer.hxx>
#include <GUI/new_gui.hxx>
#include <Main/sentryIntegration.hxx>

#include <osgViewer/Viewer>

#include <simgear/structure/commands.hxx>

#ifdef SG_WINDOWS
    #include <windows.h>

#include <osgViewer/GraphicsWindow>
#include <osgViewer/api/Win32/GraphicsWindowWin32>
#endif

#if defined(SG_MAC)

// externs from CocoaMessageBox.mm
flightgear::MessageBoxResult
cocoaFatalMessage(const std::string& msg, const std::string& text);

flightgear::MessageBoxResult
cocoaMessageBox(const std::string& msg, const std::string& text);

#endif

#ifdef HAVE_QT
    #include "QtMessageBox.hxx"
#endif

using namespace simgear::strutils;

namespace {

static bool static_isHeadless = false;

bool isCanvasImplementationRegistered()
{
	if (!globals) {
		return false;
	}

    SGCommandMgr* cmd = globals->get_commands();
    return (cmd->getCommand("canvas-message-box") != NULL);
}

#if defined(SG_WINDOWS)

HWND getMainViewerHWND()
{
	osgViewer::Viewer::Windows windows;
	if (!globals || !globals->get_renderer() || !globals->get_renderer()->getViewerBase()) {
		return 0;
	}

    globals->get_renderer()->getViewerBase()->getWindows(windows);
    osgViewer::Viewer::Windows::const_iterator it = windows.begin();
    for(; it != windows.end(); ++it) {
        if (strcmp((*it)->className(), "GraphicsWindowWin32")) {
            continue;
        }

        osgViewer::GraphicsWindowWin32* platformWin =
            static_cast<osgViewer::GraphicsWindowWin32*>(*it);
        return platformWin->getHWND();
    }

    return 0;
}

flightgear::MessageBoxResult
win32MessageBox(const std::string& caption,
                    const std::string& msg,
                    const std::string& moreText)
{
    // during early startup (aircraft / fg-data validation) there is no
    // osgViewer so no HWND.
    HWND ownerWindow = getMainViewerHWND();
    std::string fullMsg(msg);
    if (!moreText.empty()) {
        fullMsg += "\n\n" + moreText;
    }

    UINT mbType = MB_OK;
	std::wstring wMsg(convertUtf8ToWString(fullMsg)),
		wCap(convertUtf8ToWString(caption));

	::MessageBoxExW(ownerWindow, wMsg.c_str(), wCap.c_str(),
                    mbType, 0 /* system lang */);

	return flightgear::MSG_BOX_OK;
}

#endif

} // anonymous namespace

namespace flightgear
{

void setHeadlessMode(bool headless)
{
    static_isHeadless = headless;
}

bool isHeadlessMode()
{
    return static_isHeadless;
}

MessageBoxResult modalMessageBox(const std::string& caption,
    const std::string& msg,
    const std::string& moreText)
{
    // Headless mode.
    if (static_isHeadless) {
        SG_LOG(SG_HEADLESS, SG_ALERT, "ModalMessageBox Caption: \"" << caption << "\"");
        SG_LOG(SG_HEADLESS, SG_ALERT, "ModalMessageBox Message: \"" << msg << "\"");
        if (!moreText.empty())
            SG_LOG(SG_HEADLESS, SG_ALERT, "ModalMessageBox More text: \"" << moreText << "\"");
        return MSG_BOX_OK;
    }

    // prefer canvas
    if (isCanvasImplementationRegistered()) {
        SGPropertyNode_ptr args(new SGPropertyNode);
        args->setStringValue("caption", caption);
        args->setStringValue("message", msg);
        args->setStringValue("more", moreText);
        globals->get_commands()->execute("canvas-message-box", args, nullptr);

        // how to make it modal?

        return MSG_BOX_OK;
    }

#if defined(SG_WINDOWS)
    return win32MessageBox(caption, msg, moreText);
#elif defined(SG_MAC)
    return cocoaMessageBox(msg, moreText);
#elif defined(HAVE_QT)
    return QtMessageBox(caption, msg, moreText, false);
#else
    std::string s = caption + ": "+ msg;
    if (!moreText.empty()) {
        s += "\n( " +  moreText + ")";
    }

    NewGUI* gui = globals->get_subsystem<NewGUI>();
    if (!gui || (fgGetBool("/sim/rendering/initialized", false) == false)) {
        SG_LOG(SG_GENERAL, SG_POPUP, s);
    } else {
        SGPropertyNode_ptr dlg = gui->getDialogProperties("popup");
        dlg->setStringValue("text/label", s  );
        gui->showDialog("popup");
    }
    return MSG_BOX_OK;
#endif
}

MessageBoxResult fatalMessageBoxWithoutExit(const std::string& caption,
    const std::string& msg,
    const std::string& moreText)
{
    flightgear::sentryReportFatalError(msg, moreText);

    // Headless mode.
    if (static_isHeadless) {
        SG_LOG(SG_HEADLESS, SG_ALERT, "Fatal Error: \"" << caption << "\"");
        SG_LOG(SG_HEADLESS, SG_ALERT, "Error Message: \"" << msg << "\"");
        if (!moreText.empty())
            SG_LOG(SG_HEADLESS, SG_ALERT, "\tMore text: \"" << moreText << "\"");
        return MSG_BOX_OK;
    }

#if defined(SG_WINDOWS)
    return win32MessageBox(caption, msg, moreText);
#elif defined(SG_MAC)
    return cocoaFatalMessage(msg, moreText);
#elif defined(HAVE_QT)
    return QtMessageBox(caption, msg, moreText, true);
#else
    std::string s = "FATAL: "+ msg;
    if (!moreText.empty()) {
        s += "\n( " +  moreText + ")";
    }
    if (fgGetBool("/sim/rendering/initialized", false) == false) {
        std::cerr << s << std::endl;
    } else {
        NewGUI* _gui = (NewGUI *)globals->get_subsystem("gui");
        SGPropertyNode_ptr dlg = _gui->getDialogProperties("popup");
        dlg->setStringValue("text/label", s  );
        _gui->showDialog("popup");
    }
    return MSG_BOX_OK;
#endif
}

[[noreturn]] void fatalMessageBoxThenExit(
    const std::string& caption,
    const std::string& msg,
    const std::string& moreText,
    int exitStatus)
{
    fatalMessageBoxWithoutExit(caption, msg, moreText);
    // we can't use exit() here or QGuiApplication crashes
    // let's instead throw a sepcial exception which we catch
    // in boostrap.
    throw FatalErrorException{};
}

} // of namespace flightgear
