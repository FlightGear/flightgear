#pragma once

// Support for extra view windows. Requires that that composite-viewer is
// enabled at runtime (e.g. with --composite-viewer=1.
//

#include <osgViewer/View>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>


// Pushes current view to internal circular list of two items used by
// SviewCreate() with 'last_pair' or 'last_pair_double'. */
//
void SviewPush();

// Updates camera position/orientation/zoom of all sviews - should be called
// each frame.
//
void SviewUpdate(double dt);

// Deletes all internal views.
//
void SviewClear();


// Full definition of this is not public.
//
struct SviewView;

// Returns the osgViewer::View associated with a SviewView.
//
osgViewer::View* SviewGetOsgView(std::shared_ptr<SviewView> view);


// Creates a new OSG View and a new Compositor and links with the global scene
// graph and adds to the global CompositeViewer.
//
// If <gc> is null we also create a new window with a new graphics context and
// link to the newly created vew; and also add the new view to our internal
// list of views that are updated by SviewUpdate().
//
// Otherwise <gc> is not null: we make the new view use <gc> but do not add it
// to the internal list of views so, for example, it will not be updated by
// SviewUpdate().
// 
// type:
//     "last_pair"
//         Look from first pushed view's eye to second pushed view's eye.
//     "last_pair_double"
//         Keep pushed view 1's aircraft in foreground and pushed view 2's
//         aircraft in background.
//     "current"
//         Clones the current view.
// gc:
//     If null we create a new window which is managed internally and the
//     returned SviewView can be ignored.
//
// texture:
//     Must be null if <gc> is null. Otherwise must be non-null.
//
// The returned std::shared_ptr<SviewView> can be passed to SviewGetOsgView()
// to get the osgViewer::View if required.
// 
std::shared_ptr<SviewView> SviewCreate(
        const std::string& type,
        //osg::ref_ptr<osg::GraphicsContext> gc=nullptr,
        osg::ref_ptr<osg::Texture2D> texture=nullptr
        );


// Should be called early on so that SviewCreate() can create new
// simgear::compositor::Compositor instances with the same parameters as were
// used for the main window.
//
void SViewSetCompositorParams(
        osg::ref_ptr<simgear::SGReaderWriterOptions> options,
        const std::string& compositor_path
        );

// Returns true if <gc> is a known sview window.
//
bool SviewGC(const osg::GraphicsContext* gc);
