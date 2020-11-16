#include "FGWindowsMenuBar.hxx"

#include <windows.h>
#include <cstring>

#include <osgViewer/Viewer>
#include <osgViewer/GraphicsWindow>
#include <osgViewer/api/Win32/GraphicsWindowWin32>

#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/structure/SGBinding.hxx>
#include <simgear/misc/strutils.hxx>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Viewer/renderer.hxx>

#include <iostream>

using namespace simgear;

namespace {

HWND getMainViewerHWND()
{
	osgViewer::Viewer::Windows windows;
	if (!globals->get_renderer() || !globals->get_renderer()->getViewerBase()) {
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

bool labelIsSeparator(const std::string& s)
{
    std::string t = "---";
    if (s.compare(0, t.length(), t) == 0)
        return true;
    else
        return false;
}

LRESULT CALLBACK menubarWindowProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	SG_LOG(SG_GENERAL, SG_INFO, "called window proc");


	return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}

} // of anonymous namespace

//

class FGWindowsMenuBar::WindowsMenuBarPrivate
{
public:
  WindowsMenuBarPrivate();
  ~WindowsMenuBarPrivate();

  void fireBindingsForItem(UINT commandId)
  {
    fireBindingList(itemBindings[commandId]);
  }

  HWND mainWindow;
  HMENU menuBar;
  bool visible;
  WNDPROC baseMenuProc;

  typedef std::vector<SGBindingList> MenuItemBindings;
  MenuItemBindings itemBindings;

};

FGWindowsMenuBar::FGWindowsMenuBar() :
  p(new WindowsMenuBarPrivate)
{

}

FGWindowsMenuBar::~FGWindowsMenuBar()
{

}

FGWindowsMenuBar::WindowsMenuBarPrivate::WindowsMenuBarPrivate() :
	visible(true)
{
	mainWindow = getMainViewerHWND();
	menuBar = 0;
}

FGWindowsMenuBar::WindowsMenuBarPrivate::~WindowsMenuBarPrivate()
{
	if (menuBar) {
		SetMenu(mainWindow, NULL);
		DestroyMenu(menuBar);
	}
}

void FGWindowsMenuBar::init()
{
    int menuIndex = 0;
    SGPropertyNode_ptr props = fgGetNode("/sim/menubar/default",true);

    p->menuBar = CreateMenu();
//	p->baseMenuProc = (WNDPROC) ::SetWindowLongPtr((HWND) p->mainWindow, GWL_WNDPROC, (LONG_PTR) menubarWindowProc);

    for (auto n : props->getChildren("menu")) {
        // synchronise menu with properties
        std::string l = getLocalizedLabel(n);
        std::string label = strutils::simplify(l).c_str();
        HMENU menuItems = CreatePopupMenu();

        if (!n->hasValue("enabled")) {
            n->setBoolValue("enabled", true);
        }

        bool enabled = n->getBoolValue("enabled");

		UINT flags = MF_POPUP;
		AppendMenu(p->menuBar, flags, (UINT) menuItems, label.c_str());

        // submenu
        int subMenuIndex = 0;
        SGPropertyNode* menuNode = n;
        for (auto n2 : menuNode->getChildren("item")) {

            if (!n2->hasValue("enabled")) {
                n2->setBoolValue("enabled", true);
            }

            std::string l2 = getLocalizedLabel(n2);
            std::string label2 = strutils::simplify(l2).c_str();
            std::string shortcut = n2->getStringValue("key");

            SGBindingList bl = readBindingList(n->getChildren("binding"), globals->get_props());
			UINT commandId = p->itemBindings.size();
			p->itemBindings.push_back(bl);

            if (labelIsSeparator(label2)) {
                AppendMenu(menuItems, MF_SEPARATOR, NULL, NULL);
            } else {
                if (!shortcut.empty()) {
                    label2 += "\t"+shortcut;
                }
                BOOL enabled = n2->getBoolValue("enabled");

				UINT flags = MF_STRING;
				AppendMenu(menuItems, flags, commandId, label2.c_str());
            }
            subMenuIndex++;
        }
        menuIndex++;
    }

	show();
}

bool FGWindowsMenuBar::isVisible() const
{
    return p->visible;
}

void FGWindowsMenuBar::show()
{
    SetMenu(p->mainWindow, p->menuBar);
	p->visible = true;
}

void FGWindowsMenuBar::hide()
{
    SetMenu(p->mainWindow, NULL);
	p->visible = false;
}

#if 0
LRESULT CALLBACK WndProcedure(HWND hwnd, UINT Msg,
			   WPARAM wParam, LPARAM lParam)
{
    switch(Msg)
    {
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case MY_MENU:
			MessageBox(hwnd, "Menu Item Selected = Large", "Message", MB_OK);
			break;
		}
		return 0;

    case WM_DESTROY:
        PostQuitMessage(WM_QUIT);
        break;

    default:
        return DefWindowProc(hwnd, Msg, wParam, lParam);
    }

    return 0;
}
#endif
