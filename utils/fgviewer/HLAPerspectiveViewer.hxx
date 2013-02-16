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

#ifndef HLAPerspectiveViewer_hxx
#define HLAPerspectiveViewer_hxx

#include "HLAEyeTracker.hxx"
#include "HLAObjectReferenceData.hxx"
#include "HLAView.hxx"
#include "HLAViewer.hxx"

namespace fgviewer {

class HLAPerspectiveViewerClass;

class HLAPerspectiveViewer : public HLAViewer {
public:
    HLAPerspectiveViewer(HLAPerspectiveViewerClass* objectClass, const SGWeakPtr<simgear::HLAFederate>& federate);
    virtual ~HLAPerspectiveViewer();

    virtual void createAttributeDataElements();

    const HLAView* getView() const;
    HLAView* getView();
    void setView(HLAView* view);

    const SGVec3d& getPosition() const
    { return _position.getValue(); }
    void setPosition(const SGVec3d& position)
    { _position.setValue(position); }

    const SGQuatd& getOrientation() const
    { return _orientation.getValue(); }
    void setOrientation(const SGQuatd& orientation)
    { _orientation.setValue(orientation); }

    double getZoomFactor() const
    { return _zoomFactor.getValue(); }
    void setZoomFactor(double zoomFactor)
    { _zoomFactor.setValue(zoomFactor); }

    const HLAEyeTracker* getEyeTracker() const;
    HLAEyeTracker* getEyeTracker();
    void setEyeTracker(HLAEyeTracker* eyeTracker);

    SGLocationd getLocation(const SGTimeStamp& simTime) const;

    SGVec3d getLeftEyeOffset() const;
    SGVec3d getRightEyeOffset() const;

private:
    simgear::HLAObjectReferenceData<HLAView> _view;
    simgear::HLAVec3dData _position;
    simgear::HLAQuat3dData _orientation;
    simgear::HLADoubleData _zoomFactor;
    simgear::HLAObjectReferenceData<HLAEyeTracker> _eyeTracker;
};

} // namespace fgviewer

#endif
