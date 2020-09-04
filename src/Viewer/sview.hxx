#pragma once

#include <osgViewer/View>


/* */
void SviewPush();

/* Takes control the specified OSG view's camera, and makes it a clone of the
currently active main view. */
void SviewAddClone(osgViewer::View* view);

void SviewAddLastPair(osgViewer::View* view);

void SviewAddDouble(osgViewer::View* view);

/* Update all views added by SviewAdd(). */
void SviewUpdate(double dt);

void SviewClear();
