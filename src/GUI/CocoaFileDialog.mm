

#include "CocoaFileDialog.hxx"

// bring it all in!
#include <Cocoa/Cocoa.h>

#include <boost/foreach.hpp>

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/strutils.hxx>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>

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

CocoaFileDialog::CocoaFileDialog(const std::string& aTitle, FGFileDialog::Usage use) :
    FGFileDialog(aTitle, use)
{
    d.reset(new CocoaFileDialogPrivate);
    if (use == USE_SAVE_FILE) {
        d->panel = [NSSavePanel savePanel];
    } else {
        d->panel = [NSOpenPanel openPanel];
    }
    
    if (use == USE_CHOOSE_DIR) {
        [d->panel setCanChooseDirectories:YES];
    }
}

CocoaFileDialog::~CocoaFileDialog()
{
    
}

void CocoaFileDialog::exec()
{
    if (_usage == USE_SAVE_FILE) {
        [d->panel setNameFieldStringValue:stdStringToCocoa(_placeholder)];
    }
    
    NSMutableArray* extensions = [NSMutableArray arrayWithCapacity:0];
    BOOST_FOREACH(std::string ext, _filterPatterns) {
        if (!simgear::strutils::starts_with(ext, "*.")) {
            SG_LOG(SG_GENERAL, SG_INFO, "can't use pattern on Cococa:" << ext);
            continue;
        }
        [extensions addObject:stdStringToCocoa(ext.substr(2))];
    }

    [d->panel setAllowedFileTypes:extensions];
    [d->panel setTitle:stdStringToCocoa(_title)];
    if (_showHidden) {
        [d->panel setShowsHiddenFiles:YES];
    }
    
    [d->panel setDirectoryURL: pathToNSURL(_initialPath)];
    
    [d->panel beginWithCompletionHandler:^(NSInteger result)
    {
        if (result == NSFileHandlingPanelOKButton) {
            NSURL*  theDoc = [d->panel URL];
            NSLog(@"the URL is: %@", theDoc);
            // Open  the document.
        }
        
    }];
}
