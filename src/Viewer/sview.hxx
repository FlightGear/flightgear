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
void SviewCreate(const std::string type);

/* Should be called early on so that SviewCreate() can create new compositors.
*/
void SViewSetCompositorParams(
        osg::ref_ptr<simgear::SGReaderWriterOptions> options,
        const std::string& compositor_path
        );
