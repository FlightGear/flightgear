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

#include <simgear/compiler.h>
#include STL_IOSTREAM

#include "linintp2.h"
#include <plib/sg.h>

#ifndef SG_HAVE_NATIVE_SGI_COMPILERS
SG_USING_NAMESPACE(std);
SG_USING_STD(cout);
#endif


class SphereInterpolate
{
public:
    SphereInterpolate (int n, const double* x, const double* y,
	const double* z, const unsigned int* f);
    SphereInterpolate (int n, const sgVec2* p, const unsigned int* f);
   
    ~SphereInterpolate ();
    
    void GetSphericalCoords (const double x, const double y, const double z,
	double& thetaAngle, double& phiAngle) const;
    
    int Evaluate (const double x, const double y, const double z, EvaluateData& f) const;
    int Evaluate (const double thetaAngle, const double phiAngle, EvaluateData& f) const;

#ifndef macintosh
    // CodeWarrior doesn't know the differece between sgVec2 and
    // sgVec3, so I commented this out for Mac builds. This change is
    // related to a similar change in FGLocalWeatherDatabase module.
     
    EvaluateData Evaluate(const sgVec2& p) const
    {
	EvaluateData retval;
	Evaluate(p[1], p[0], retval);
	return retval;
    }
#endif

 
    EvaluateData Evaluate(const sgVec3& p) const
    {
	EvaluateData retval;
	if (!Evaluate(p[1], p[0], retval))
        {
          cout << "Error during spherical interpolation. Point (" 
               << p[0] << "/" << p[1] << "/" << p[2] << ") was in no triangle\n";
          retval.index[0] = 0;  //fake something
          retval.index[1] = 0;
          retval.index[2] = 0;
          retval.percentage[0] = 1.0;
          retval.percentage[1] = 0.0;
          retval.percentage[2] = 0.0;
        }
	return retval;
    }

protected:
    int numPoints;
    double* theta;
    double* phi;
    unsigned int* func;
    mgcLinInterp2D* pInterp;
};

#endif
