// Copyright (C) 2009 - 2012  Mathias Froehlich
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifndef Frustum_hxx
#define Frustum_hxx

#include <cmath>
#include <osg/Matrix>
#include <osg/Vec2>
#include <osg/Vec3>
#include <simgear/math/SGMath.hxx>

namespace fgviewer  {

struct Frustum {
    Frustum(const double& aspectRatio = 1) :
        _left(-aspectRatio),
        _right(aspectRatio),
        _bottom(-1),
        _top(1),
        _near(2)
    { }
    Frustum(const double& left, const double& right, const double& bottom, const double& top, const double& zNear) :
        _left(left),
        _right(right),
        _bottom(bottom),
        _top(top),
        _near(zNear)
    { }
    Frustum(const Frustum& frustum) :
        _left(frustum._left),
        _right(frustum._right),
        _bottom(frustum._bottom),
        _top(frustum._top),
        _near(frustum._near)
    { }

    bool setMatrix(const osg::Matrix& matrix)
    {
        double zFar;
        return matrix.getFrustum(_left, _right, _bottom, _top, _near, zFar);
    }
    osg::Matrix getMatrix() const
    {
        return getMatrix(osg::Vec2(_near, 2*_near));
    }
    /// Finite projection matrix
    osg::Matrix getMatrix(const osg::Vec2& depthRange) const
    {
        double zNear = depthRange[0];
        double zFar = depthRange[1];
        /// left, right, bottom and top are rescaled by zNear/_near and the result is
        /// inserted into the final equations. This rescaling factor just cancels out mostly.
        double a00 = 2*_near/(_right - _left);
        double a11 = 2*_near/(_top - _bottom);
        double a20 = (_right + _left)/(_right - _left);
        double a21 = (_top + _bottom)/(_top - _bottom);
        double a22 = (zNear + zFar)/(zNear - zFar);
        double a23 = -1;
        double a32 = 2*zNear*zFar/(zNear - zFar);

        return osg::Matrix(a00,   0,   0,   0,
                             0, a11,   0,   0,
                           a20, a21, a22, a23,
                             0,   0, a32,   0);
    }
    /// Infinite projection matrix with a given zNear plane
    osg::Matrix getMatrix(const double& zNear, const double& eps = 0) const
    {
        /// left, right, bottom and top are rescaled by zNear/_near and the result is
        /// inserted into the final equations. This rescaling factor just cancels out mostly.
        double a00 = 2*_near/(_right - _left);
        double a11 = 2*_near/(_top - _bottom);
        double a20 = (_right + _left)/(_right - _left);
        double a21 = (_top + _bottom)/(_top - _bottom);
        double a22 = eps - 1;
        double a23 = -1;
        double a32 = zNear*(eps - 2);

        return osg::Matrix(a00,   0,   0,   0,
                             0, a11,   0,   0,
                           a20, a21, a22, a23,
                             0,   0, a32,   0);
    }

    /// Return the aspect ratio of the frustum
    double getAspectRatio() const
    { return (_right - _left)/(_top - _bottom); }
    void setAspectRatio(double aspectRatio)
    {
        double aspectScale = aspectRatio/getAspectRatio();
        _left *= aspectScale;
        _right *= aspectScale;
    }

    void setFieldOfViewRad(double fieldOfViewRad)
    {
        double fieldOfView = tan(fieldOfViewRad);
        double aspectRatio = getAspectRatio();
        _left = -fieldOfView*aspectRatio*0.5*_near;
        _right = fieldOfView*aspectRatio*0.5*_near;
        _bottom = -fieldOfView*0.5*_near;
        _top = fieldOfView*0.5*_near;
    }
    void setFieldOfViewDeg(double fieldOfViewDeg)
    { setFieldOfViewRad(SGMiscd::deg2rad(fieldOfViewDeg)); }


    /// Translate this frustum by the given eye point offset
    Frustum translate(const osg::Vec3& eyeOffset) const
    {
        double left = _left - eyeOffset[0];
        double right = _right - eyeOffset[0];
        double bottom = _bottom - eyeOffset[1];
        double top = _top - eyeOffset[1];
        double zNear = _near + eyeOffset[2];
        return Frustum(left, right, bottom, top, zNear);
    }
    /// Scale this frustum around the scale center.
    /// Gives something similar like zooming into the view.
    Frustum scale(double scaleFactor, const osg::Vec3& scaleCenter) const
    {
        Frustum frustum;
        frustum._left = scaleFactor*(_left - scaleCenter[0]) + scaleCenter[0];
        frustum._right = scaleFactor*(_right - scaleCenter[0]) + scaleCenter[0];
        frustum._bottom = scaleFactor*(_bottom - scaleCenter[1]) + scaleCenter[1];
        frustum._top = scaleFactor*(_top - scaleCenter[1]) + scaleCenter[1];
        frustum._near = scaleFactor*(_near + scaleCenter[2]) - scaleCenter[2];
        return frustum;
    }

    // Parameters for the reference view frustum.
    double _left;
    double _right;
    double _bottom;
    double _top;
    // Is not the real zNear plane. Just used to reference the other frustum parameters.
    double _near;
};

} // namespace fgviewer

#endif
