#pragma once

#include <osgViewer/View>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>


/* Pushes current view to internal list used by SviewCreate() with 'last_pair'
or 'last_pair_double'. */
void SviewPush();

/* Update all sviews. */
void SviewUpdate(double dt);

void SviewClear();

/* Opens a window showing a new view. */
void SviewCreate(const std::string& type);

struct SviewView;

/* Creates a new OSG View and a new Compositor and links with the global scene
graph and adds to the global CompositeViewer.

If <gc> is null we create a new window with a new graphics context and and link
to the newly created vew. We also add the new view to our internal list of
views that are updated by SviewUpdate().

Otherwise <gc> is not null: we make the new view use <gc> but do not add it
to the internal list of views so, for example, it will not be updated by
SviewUpdate(). */
std::shared_ptr<SviewView> SviewCreate(
        const std::string& type,
        osg::ref_ptr<osg::GraphicsContext> gc
        );

/* Should be called early on so that SviewCreate() can create new compositors.
*/
void SViewSetCompositorParams(
        osg::ref_ptr<simgear::SGReaderWriterOptions> options,
        const std::string& compositor_path
        );

/* Returns true if <gc> is a known sview window.
*/
bool SviewGC(const osg::GraphicsContext* gc);
