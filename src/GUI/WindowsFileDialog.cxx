

#include "WindowsFileDialog.hxx"

#include <windows.h>
#include <Shlobj.h>

#include <osgViewer/Viewer>
#include <osgViewer/api/Win32/GraphicsWindowWin32>

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/strutils.hxx>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Viewer/renderer.hxx>

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

static int CALLBACK BrowseFolderCallback(
                  HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    if (uMsg == BFFM_INITIALIZED) {
		// set the initial directory now
		WindowsFileDialog* dlg = reinterpret_cast<WindowsFileDialog*>(lpData);
		const auto w = dlg->getDirectory().wstr();
        ::SendMessageW(hwnd, BFFM_SETSELECTIONW, true, (LPARAM) w.c_str());
    }
    return 0;
}

} // of anonymous namespace

WindowsFileDialog::WindowsFileDialog(FGFileDialog::Usage use) :
    FGFileDialog(use)
{

}

WindowsFileDialog::~WindowsFileDialog()
{

}

void WindowsFileDialog::exec()
{
    char Filestring[MAX_PATH] = "\0";
    OPENFILENAME opf={0};
    opf.lStructSize = sizeof(OPENFILENAME);
    opf.lpstrFile = Filestring;
    opf.lpstrTitle = const_cast<char *>(_title.c_str());
    opf.nMaxFile = MAX_PATH;

    std::string extensions;
    size_t extensionsLen;
    if (!_filterPatterns.empty()) {
        for (const auto& ext : _filterPatterns) {
            if (!simgear::strutils::starts_with(ext, "*.")) {
                SG_LOG(SG_GENERAL, SG_ALERT, "WindowsFileDialog: can't use pattern on Windows:" << ext);
                continue;
            }
            extensions += "("+ext+")\0"+ext+"\0";
            extensionsLen += ext.size()*2+4;
        }
        opf.lpstrFilter = (LPCSTR) malloc(extensionsLen);
        memcpy((void*)opf.lpstrFilter, (void*)extensions.data(), extensionsLen);
    }

	std::string s = _initialPath.local8BitStr();
    opf.lpstrInitialDir =  const_cast<char *>(s.c_str());

    if (_showHidden) {
        opf.Flags = OFN_PATHMUSTEXIST;
    }

    if (_usage == USE_SAVE_FILE) {
        if (GetSaveFileNameA(&opf)) {
            std::string stringPath(opf.lpstrFile);
            _callback->onFileDialogDone(this, stringPath);
        }
    } else if (_usage == USE_CHOOSE_DIR) {
        chooseDir();
    } else {
        if (GetOpenFileNameA(&opf)) {
            std::string stringPath(opf.lpstrFile);
            _callback->onFileDialogDone(this, stringPath);
        }
    }
}

void WindowsFileDialog::close()
{

}

void WindowsFileDialog::chooseDir()
{
	// MSDN says this needs to be called first
	OleInitialize(NULL);

	char pathBuf[MAX_PATH] = "\0";

	BROWSEINFO binfo;
	memset(&binfo, 0, sizeof(BROWSEINFO));
	binfo.hwndOwner = getMainViewerHWND();
	binfo.ulFlags = BIF_USENEWUI | BIF_RETURNONLYFSDIRS | BIF_EDITBOX;

	binfo.pidlRoot = NULL; // can browse anywhere
	binfo.lpszTitle = const_cast<char *>(_title.c_str());
	binfo.lpfn = BrowseFolderCallback;
	binfo.lParam = reinterpret_cast<LPARAM>(this);

	PIDLIST_ABSOLUTE results = SHBrowseForFolder(&binfo);
	if (results == NULL) {
		// user cancelled
		return;
	}

	SHGetPathFromIDList(results, pathBuf);
	CoTaskMemFree(results);

	_callback->onFileDialogDone(this, SGPath(pathBuf));
}
