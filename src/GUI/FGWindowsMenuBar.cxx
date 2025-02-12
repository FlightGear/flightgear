/*
 * SPDX-FileName: FGWindowsMenuBar.cxx
 * SPDX-FileComment: XML-configured menu bar.
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

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

#include "new_gui.hxx"

#include <iostream>

LONG_PTR g_prevWindowProc;

static HWND getMainViewerHWND()
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

static bool nameIsSeparator(const std::string& n)
{
    return simgear::strutils::starts_with(n, "----");
}

static LRESULT CALLBACK menubarWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{   
	switch (Msg) {
    case WM_COMMAND:
        if (HIWORD(wParam) == 0) {
            auto gui = globals->get_subsystem<NewGUI>();
            auto menubar = (FGWindowsMenuBar*)gui->getMenuBar();
            int commandId = (int)LOWORD(wParam);
            if (commandId < menubar->getItemBindings().size()) {
                fireBindingList(menubar->getItemBindings()[commandId]);
            }
        }
        break;
    }

    return CallWindowProc((WNDPROC)g_prevWindowProc, hWnd, Msg, wParam, lParam);
}

class FGWindowsMenuBar::WindowsMenuBarPrivate
{
public:
  WindowsMenuBarPrivate();
  ~WindowsMenuBarPrivate();

  HWND mainWindow;
  HMENU menuBar;
  bool visible;
  bool hideIfOverlapsWindow;

  typedef std::vector<SGBindingList> MenuItemBindings;
  MenuItemBindings itemBindings;

};

FGWindowsMenuBar::WindowsMenuBarPrivate::WindowsMenuBarPrivate() :
    visible(true), hideIfOverlapsWindow(false)
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

FGWindowsMenuBar::FGWindowsMenuBar() :
    _p(new WindowsMenuBarPrivate)
{
}

void FGWindowsMenuBar::init()
{
    int menuIndex = 0;
    SGPropertyNode_ptr props = fgGetNode("/sim/menubar/default",true);

    _p->menuBar = CreateMenu();
    LONG_PTR tempWindowProc = GetWindowLongPtr(_p->mainWindow, GWLP_WNDPROC);
    if (tempWindowProc != (LONG_PTR)menubarWindowProc) {
        g_prevWindowProc = tempWindowProc;
        SetWindowLongPtr(_p->mainWindow, GWLP_WNDPROC, (LONG_PTR)menubarWindowProc);
    }

    for (auto menu : props->getChildren("menu")) {
        // synchronise menu with properties
        std::string l = getLocalizedLabel(menu);
        std::string label = simgear::strutils::simplify(l);
        HMENU menuItems = CreatePopupMenu();

        if (!menu->hasValue("enabled")) {
            menu->setBoolValue("enabled", true);
        }

        bool enabled = menu->getBoolValue("enabled");

		UINT flags = MF_POPUP;
        const auto wlabel = simgear::strutils::convertUtf8ToWString(label);
        AppendMenuW(_p->menuBar, flags, (UINT_PTR)menuItems, wlabel.c_str());

        // submenu
        int subMenuIndex = 0;
        SGPropertyNode* menuNode = menu;
        for (auto menuItem : menuNode->getChildren("item")) {

            if (!menuItem->hasValue("enabled")) {
                menuItem->setBoolValue("enabled", true);
            }

            std::string l2 = getLocalizedLabel(menuItem);
            std::string label2 = simgear::strutils::simplify(l2);
            std::string shortcut = menuItem->getStringValue("key");

            SGBindingList bl = readBindingList(menuItem->getChildren("binding"), globals->get_props());
			UINT commandId = _p->itemBindings.size();
			_p->itemBindings.push_back(bl);

            if (nameIsSeparator(label2)) {
                AppendMenu(menuItems, MF_SEPARATOR, NULL, NULL);
            } else {
                if (!shortcut.empty()) {
                    label2 += "\t"+shortcut;
                }
                BOOL enabled = menuItem->getBoolValue("enabled");

				UINT flags = MF_STRING;
                const auto wl2 = simgear::strutils::convertUtf8ToWString(label2);
                AppendMenuW(menuItems, flags, commandId, wl2.c_str());
            }
            subMenuIndex++;
        }
        menuIndex++;
    }

	show();
}

void FGWindowsMenuBar::show()
{
    SetMenu(_p->mainWindow, _p->menuBar);
	_p->visible = true;
}

void FGWindowsMenuBar::hide()
{
    SetMenu(_p->mainWindow, NULL);
	_p->visible = false;
}

bool FGWindowsMenuBar::isVisible() const
{
    return _p->visible;
}

void FGWindowsMenuBar::setHideIfOverlapsWindow(bool hideOverlapping)
{
    _p->hideIfOverlapsWindow = hideOverlapping;
    if (_p->menuBar) {
        const bool actualVis = _p->visible && (!_p->hideIfOverlapsWindow);
        if (actualVis) {
            show();
        } else {
            hide();
        }
    }
}

bool FGWindowsMenuBar::getHideIfOverlapsWindow() const
{
    return _p->hideIfOverlapsWindow;
}

std::vector<SGBindingList> FGWindowsMenuBar::getItemBindings() const
{
    return _p->itemBindings;
}
