// CocoaFileDialog.mm - Cocoa implementation of file-dialog interface

// Copyright (C) 2013 James Turner <zakalawe@mac.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//


#include "CocoaFileDialog.hxx"

#include <AppKit/NSSavePanel.h>
#include <AppKit/NSOpenPanel.h>

#include <osgViewer/Viewer>
#include <osgViewer/api/Cocoa/GraphicsWindowCocoa>

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/strutils.hxx>

#include <GUI/CocoaHelpers_private.h>
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Viewer/renderer.hxx>

// 10.6 compiler won't accept block-scoped locals in Objective-C++,
// so making these globals.
static NSString* completion_path = nil;
static SGPath completion_sgpath;

class CocoaFileDialog::CocoaFileDialogPrivate
{
public:
    CocoaFileDialogPrivate() :
        panel(nil)
    {

    }

    ~CocoaFileDialogPrivate()
    {
        [panel release];
    }

    NSSavePanel* panel;
};

CocoaFileDialog::CocoaFileDialog(FGFileDialog::Usage use) :
    FGFileDialog(use)
{
    d.reset(new CocoaFileDialogPrivate);
    if (use == USE_SAVE_FILE) {
        d->panel = [NSSavePanel savePanel];
    } else {
        NSOpenPanel* openPanel = [NSOpenPanel openPanel];
        d->panel = openPanel;

        if (use == USE_CHOOSE_DIR) {
            [openPanel setCanChooseDirectories:YES];
        }
    } // of USE_OPEN_FILE or USE_CHOOSE_DIR -> building NSOpenPanel

    [d->panel retain];
}

CocoaFileDialog::~CocoaFileDialog()
{

}

void CocoaFileDialog::exec()
{
// find the native Cocoa NSWindow handle so we can parent the dialog and show
// it window-modal.
    NSWindow* cocoaWindow = nil;
    std::vector<osgViewer::GraphicsWindow*> windows;
    globals->get_renderer()->getViewerBase()->getWindows(windows);

    for (auto gw : windows) {
        // OSG doesn't use RTTI, so no dynamic cast. Let's check the class type
        // using OSG's own system, before we blindly static_cast<> and break
        // everything.
        if (strcmp(gw->className(), "GraphicsWindowCocoa")) {
            continue;
        }

        osgViewer::GraphicsWindowCocoa* gwCocoa = static_cast<osgViewer::GraphicsWindowCocoa*>(gw);
        cocoaWindow = (NSWindow*) gwCocoa->getWindow();
        break;
    }

// setup the panel fields now we have collected all the data
    if (_usage == USE_SAVE_FILE) {
        [d->panel setNameFieldStringValue:stdStringToCocoa(_placeholder)];
    }

    if (_filterPatterns.empty()) {
        [d->panel setAllowedFileTypes:nil];
    } else {
        NSMutableArray* extensions = [NSMutableArray arrayWithCapacity:0];
        for (const auto& ext : _filterPatterns) {
            if (!simgear::strutils::starts_with(ext, "*.")) {
                SG_LOG(SG_GENERAL, SG_INFO, "can't use pattern on Cococa:" << ext);
                continue;
            }
            [extensions addObject:stdStringToCocoa(ext.substr(2))];
        }

        [d->panel setAllowedFileTypes:extensions];
    }

    [d->panel setTitle:stdStringToCocoa(_title)];
    if (_showHidden) {
        [d->panel setShowsHiddenFiles:YES];
    }

    [d->panel setDirectoryURL: pathToNSURL(_initialPath)];

    [d->panel beginSheetModalForWindow:cocoaWindow completionHandler:^(NSInteger result)
    {
        if (result == NSFileHandlingPanelOKButton) {
            completion_path = [[d->panel URL] path];
            //NSLog(@"the URL is: %@", d->panel URL]);
            completion_sgpath = SGPath::fromUtf8([completion_path UTF8String]);
            _callback->onFileDialogDone(this, completion_sgpath);
        }
    }];
}

void CocoaFileDialog::close()
{
    [d->panel close];
}
