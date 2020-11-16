#pragma once

/*
Support for extra view windows. Requires that composite-viewer is enabled at
startup with --composite-viewer=1.
*/

#include <simgear/scene/util/SGReaderWriterOptions.hxx>

#include <osgViewer/View>


/* Should be called before the first call to SviewCreate() so that
SviewCreate() can create new simgear::compositor::Compositor instances with the
same parameters as were used for the main window.

options
compositor_path
    Suitable for passing to simgear::compositor::Compositor().
*/
void SViewSetCompositorParams(
        osg::ref_ptr<simgear::SGReaderWriterOptions> options,
        const std::string& compositor_path
        );

/* Pushes current main window view to internal circular list of two items used
by SviewCreate() with 'last_pair' or 'last_pair_double'. */
void SviewPush();

/* Updates camera position/orientation/zoom of all sviews - should be called
each frame. Will also handle closing of Sview windows. */
void SviewUpdate(double dt);

/* Deletes all internal views; e.g. used when restarting. */
void SviewClear();

/* A view, typically an extra view window. The definition of this is not
public. */
struct SviewView;

/*
This is the main interface to the Sview system. We create a new SviewView in a
new top-level window. It will be updated as required by SviewUpdate().

As of 2020-11-18, the new window will be half width and height of the main
window, and will have top-left corner at (100, 100). It can be dragged, resized
and closed by the user.

type:
    This controls what sort of view we create:
    
    "current"
        Clones the current view.
    "last_pair"
        Look from first pushed view's eye to second pushed view's eye. Returns
        nullptr if SviewPush hasn't been called at least twice.
    "last_pair_double"
        Keep first pushed view's aircraft in foreground and second pushed
        view's aircraft in background. Returns nullptr if SviewPush hasn't been
        called at least twice.

Returns:
    Shared ptr to SviewView instance. As of 2020-11-18 there is little that
    the caller can do with this. We handle closing of the SviewView's window
    internally.

As of 2020-11-17, extra views have various limitations including:

    No event handling, so no panning, zooming etc.
    
    Tower View AGL is like Tower View so no zooming to keep ground visible.
    
    Cockpit view has a incorrect calculation giving slightly incorrect
    translation when rolling.
    
    No damping in chase views.
    
    Hard-coded chase distances.
*/ 
std::shared_ptr<SviewView> SviewCreate(const std::string& type);
