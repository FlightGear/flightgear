

#include "CocoaFileDialog.hxx"

// bring it all in!
#include <Cocoa/Cocoa.h>

#include <boost/foreach.hpp>

#include <osgViewer/Viewer>
#include <osgViewer/api/Cocoa/GraphicsWindowCocoa>

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/strutils.hxx>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Viewer/renderer.hxx>

static NSString* stdStringToCocoa(const std::string& s)
{
    return [NSString stringWithUTF8String:s.c_str()];
}

static NSURL* pathToNSURL(const SGPath& aPath)
{
    return [NSURL fileURLWithPath:stdStringToCocoa(aPath.str())];
}

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
    globals->get_renderer()->getViewer()->getWindows(windows);
    
    BOOST_FOREACH(osgViewer::GraphicsWindow* gw, windows) {
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
        BOOST_FOREACH(std::string ext, _filterPatterns) {
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
        NSString* path = nil;
        SGPath sgpath;
        
        if (result == NSFileHandlingPanelOKButton) {
            path = [[d->panel URL] path];
            //NSLog(@"the URL is: %@", d->panel URL]);
            sgpath = ([path UTF8String]);
            _callback->onFileDialogDone(this, sgpath);
        }
    }];
}

void CocoaFileDialog::close()
{
    [d->panel close];
}

