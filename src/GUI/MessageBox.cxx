#ifdef HAVE_CONFIG_H
    #include "config.h"
#endif

#include <simgear/simgear_config.h>

#include "MessageBox.hxx"

#include <Main/globals.hxx>
#include <Viewer/renderer.hxx>

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

using namespace simgear::strutils;

namespace {

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
	if (!globals || !globals->get_renderer() || !globals->get_renderer()->getViewer()) {
		return 0;
	}

    globals->get_renderer()->getViewer()->getWindows(windows);
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
	WCharVec wMsg(convertUtf8ToWString(fullMsg)),
		wCap(convertUtf8ToWString(caption));
	wMsg.push_back(0);
	wCap.push_back(0);

	::MessageBoxExW(ownerWindow, wMsg.data(), wCap.data(),
                    mbType, 0 /* system lang */);

	return flightgear::MSG_BOX_OK;
}
    
#endif
    
} // anonymous namespace

namespace flightgear
{
    
MessageBoxResult modalMessageBox(const std::string& caption,
    const std::string& msg,
    const std::string& moreText)
{
    // prefer canvas
    if (isCanvasImplementationRegistered()) {
        SGPropertyNode_ptr args(new SGPropertyNode);
        args->setStringValue("caption", caption);
        args->setStringValue("message", msg);
        args->setStringValue("more", moreText);
        globals->get_commands()->execute("canvas-message-box", args);
   
        // how to make it modal?
        
        return MSG_BOX_OK;
    }

#if defined(SG_WINDOWS)
    return win32MessageBox(caption, msg, moreText);
#elif defined(SG_MAC)
    return cocoaMessageBox(msg, moreText);
#else
    SG_LOG(SG_GENERAL, SG_ALERT, caption << ":" << msg);
    if (!moreText.empty()) {
        SG_LOG(SG_GENERAL, SG_ALERT, "(" << moreText << ")");
    }
    return MSG_BOX_OK;
#endif
}

MessageBoxResult fatalMessageBox(const std::string& caption,
    const std::string& msg,
    const std::string& moreText)
{
#if defined(SG_WINDOWS)
    return win32MessageBox(caption, msg, moreText);
#elif defined(SG_MAC)
    return cocoaFatalMessage(msg, moreText);
#else
    std::cerr << "FATAL:" << msg << "\n";
    if (!moreText.empty()) {
        std::cerr << "(" << moreText << ")";
    }
    std::cerr << std::endl;
    return MSG_BOX_OK;
#endif
}

} // of namespace flightgear
