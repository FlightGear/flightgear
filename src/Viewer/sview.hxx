#pragma once

#include <osgViewer/View>

/* Takes control the specified OSG view's camera, and makes it a clone of the
currently active main view. */
void SviewAddClone(osgViewer::View* view);

/* Update all views added by SviewAdd(). */
void SviewUpdate(double dt);

void SviewClear();
