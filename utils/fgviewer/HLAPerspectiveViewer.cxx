// Copyright (C) 2009 - 2012  Mathias Froehlich - Mathias.Froehlich@web.de
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "HLAPerspectiveViewer.hxx"

#include "HLAPerspectiveViewerClass.hxx"

namespace fgviewer {

HLAPerspectiveViewer::HLAPerspectiveViewer(HLAPerspectiveViewerClass* objectClass, const SGWeakPtr<simgear::HLAFederate>& federate) :
    HLAViewer(objectClass),
    _view(federate),
    _zoomFactor(1),
    _eyeTracker(federate)
{
}

HLAPerspectiveViewer::~HLAPerspectiveViewer()
{
}

void
HLAPerspectiveViewer::createAttributeDataElements()
{
    HLAViewer::createAttributeDataElements();

    assert(dynamic_cast<HLAPerspectiveViewerClass*>(getObjectClass().get()));
    HLAPerspectiveViewerClass& objectClass = static_cast<HLAPerspectiveViewerClass&>(*getObjectClass());

    setAttributeDataElement(objectClass.getViewNameIndex(), _view.getDataElement());
    setAttributeDataElement(objectClass.getPositionIndex(), _position.getDataElement());
    setAttributeDataElement(objectClass.getOrientationIndex(), _orientation.getDataElement());
    setAttributeDataElement(objectClass.getZoomFactorIndex(), _zoomFactor.getDataElement());
    setAttributeDataElement(objectClass.getEyeTrackerNameIndex(), _eyeTracker.getDataElement());
}

const HLAView*
HLAPerspectiveViewer::getView() const
{
    return _view.getObject();
}

HLAView*
HLAPerspectiveViewer::getView()
{
    return _view.getObject();
}

void
HLAPerspectiveViewer::setView(HLAView* view)
{
    _view.setObject(view);
}

const HLAEyeTracker*
HLAPerspectiveViewer::getEyeTracker() const
{
    return _eyeTracker.getObject();
}

HLAEyeTracker*
HLAPerspectiveViewer::getEyeTracker()
{
    return _eyeTracker.getObject();
}

void
HLAPerspectiveViewer::setEyeTracker(HLAEyeTracker* eyeTracker)
{
    _eyeTracker.setObject(eyeTracker);
}

SGLocationd
HLAPerspectiveViewer::getLocation(const SGTimeStamp& simTime) const
{
    const HLAView* view = getView();
    if (!view)
        return SGLocationd(getPosition(), getOrientation());
    return view->getLocation(simTime).getAbsoluteLocation(SGLocationd(getPosition(), getOrientation()));
}

SGVec3d
HLAPerspectiveViewer::getLeftEyeOffset() const
{
    const HLAEyeTracker* eyeTracker = getEyeTracker();
    if (!eyeTracker)
        return SGVec3d(0, 0, 0);
    return eyeTracker->getLeftEyeOffset();
}

SGVec3d
HLAPerspectiveViewer::getRightEyeOffset() const
{
    const HLAEyeTracker* eyeTracker = getEyeTracker();
    if (!eyeTracker)
        return SGVec3d(0, 0, 0);
    return eyeTracker->getRightEyeOffset();
}

} // namespace fgviewer
