/*
  WARNING - Do not remove this header.

  This code is a templated version of the 'magic-software' spherical
  interpolation code by Dave Eberly. The original (un-hacked) code can be
  obtained from here: http://www.magic-software.com/gr_appr.htm
  This code is derived from linintp2.h/cpp and sphrintp.h/cpp.

  Dave Eberly says that the conditions for use are:

  * You may distribute the original source code to others at no charge.

  * You may modify the original source code and distribute it to others at
    no charge. The modified code must be documented to indicate that it is
    not part of the original package.

  * You may use this code for non-commercial purposes. You may also
    incorporate this code into commercial packages. However, you may not
    sell any of your source code which contains my original and/or modified
    source code. In such a case, you need to factor out my code and freely
    distribute it.

  * The original code comes with absolutely no warranty and no guarantee is
    made that the code is bug-free.

  This does not seem incompatible with GPL - so this modified version
  is hereby placed under GPL along with the rest of FlightGear.

                              Christian Mayer
*/

#ifndef SPHRINTP_H
#define SPHRINTP_H

#include "linintp2.h"
#include <plib/sg.h>

template<class T>
class SphereInterpolate
{
public:
    SphereInterpolate (int n, const double* x, const double* y,
	const double* z, const T* f);
    SphereInterpolate (int n, const sgVec2* p, const T* f);
   
    ~SphereInterpolate ();
    
    void GetSphericalCoords (const double x, const double y, const double z,
	double& thetaAngle, double& phiAngle) const;
    
    int Evaluate (const double x, const double y, const double z, T& f) const;
    int Evaluate (const double thetaAngle, const double phiAngle, T& f) const;

#ifndef MACOS
    // CodeWarrior doesn't know the differece between sgVec2 and
    // sgVec3, so I commented this out for Mac builds. This change is
    // related to a similar change in FGLocalWeatherDatabase module.
     
    T Evaluate(const sgVec2& p) const
    {
	T retval;
	Evaluate(p[1], p[0], retval);
	return retval;
    }
#endif

 
    T Evaluate(const sgVec3& p) const
    {
	T retval;
	Evaluate(p[1], p[0], retval);
	return retval;
    }

protected:
    int numPoints;
    double* theta;
    double* phi;
    T* func;
    mgcLinInterp2D<T>* pInterp;
};

#include "sphrintp.inl"

#endif
