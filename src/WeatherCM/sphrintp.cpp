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

#include <simgear/compiler.h>
#include <iostream>

#include <math.h>
#include "sphrintp.h"

SG_USING_NAMESPACE(std);
SG_USING_STD(cout);


static const double PI = 4.0*atan(1.0);
static const double TWOPI = 2.0*PI;

//---------------------------------------------------------------------------
SphereInterpolate::SphereInterpolate (int n, const double* x,
				         const double* y, const double* z,
				         const unsigned int* f)
{
    // Assumes (x[i],y[i],z[i]) is unit length for all 0 <= i < n.
    // For complete spherical coverage, include the two antipodal points
    // (0,0,1,f(0,0,1)) and (0,0,-1,f(0,0,-1)) in the data set.
    
    cout << "Initialising spherical interpolator.\n";
    cout << "[  0%] Allocating memory                                           \r";

    theta = new double[3*n];
    phi = new double[3*n];
    func = new unsigned int[3*n];
    
    // convert data to spherical coordinates
    int i;

    for (i = 0; i < n; i++)
    {
	GetSphericalCoords(x[i],y[i],z[i],theta[i],phi[i]);
	func[i] = f[i];
    }
    
    // use periodicity to get wrap-around in the Delaunay triangulation
    cout << "[ 10%] copying vertices for wrap-around\r";
    int j, k;
    for (i = 0, j = n, k = 2*n; i < n; i++, j++, k++)
    {
	theta[j] = theta[i]+TWOPI;
	theta[k] = theta[i]-TWOPI;
	phi[j] = phi[i];
	phi[k] = phi[i];
	func[j] = func[i];
	func[k] = func[i];
    }
    
    pInterp = new mgcLinInterp2D(3*n,theta,phi,func);

    cout << "[100%] Finished initialising spherical interpolator.                   \n";
}

SphereInterpolate::SphereInterpolate (int n, const sgVec2* p, const unsigned int* f)
{
    // Assumes (x[i],y[i],z[i]) is unit length for all 0 <= i < n.
    // For complete spherical coverage, include the two antipodal points
    // (0,0,1,f(0,0,1)) and (0,0,-1,f(0,0,-1)) in the data set.
    cout << "Initialising spherical interpolator.\n";
    cout << "[  0%] Allocating memory                                           \r";

    theta = new double[3*n];
    phi = new double[3*n];
    func = new unsigned int[3*n];

    // convert data to spherical coordinates
    cout << "[ 10%] copying vertices for wrap-around                              \r";

    int i, j, k;
    for (i = 0, j = n, k = 2*n; i < n; i++, j++, k++)
    {
	phi[i] = p[i][0];
	theta[i] = p[i][1];
	func[i] = f[i];

	// use periodicity to get wrap-around in the Delaunay triangulation
	phi[j] = phi[i];
	phi[k] = phi[i];
	theta[j] = theta[i]+TWOPI;
	theta[k] = theta[i]-TWOPI;
	func[j] = func[i];
	func[k] = func[i];
    }
    
    pInterp = new mgcLinInterp2D(3*n,theta,phi,func);

    cout << "[100%] Finished initialising spherical interpolator.                   \n";
}
//---------------------------------------------------------------------------
SphereInterpolate::~SphereInterpolate ()
{
    delete pInterp;
    delete[] theta;
    delete[] phi;
    delete[] func;
}
//---------------------------------------------------------------------------
void SphereInterpolate::GetSphericalCoords (const double x, const double y, const double z,
					       double& thetaAngle,
					       double& phiAngle) const
{
    // Assumes (x,y,z) is unit length.  Returns -PI <= thetaAngle <= PI
    // and 0 <= phiAngle <= PI.
    
    if ( z < 1.0f )
    {
	if ( z > -1.0f )
	{
	    thetaAngle = atan2(y,x);
	    phiAngle = acos(z);
	}
	else
	{
	    thetaAngle = -PI;
	    phiAngle = PI;
	}
    }
    else
    {
	thetaAngle = -PI;
	phiAngle = 0.0f;
    }
}
//---------------------------------------------------------------------------
int SphereInterpolate::Evaluate (const double x, const double y, const double z, EvaluateData& f) const 
{
    // assumes (x,y,z) is unit length
    
    double thetaAngle, phiAngle;
    GetSphericalCoords(x,y,z,thetaAngle,phiAngle);
    return pInterp->Evaluate(thetaAngle,phiAngle,f);
}
//---------------------------------------------------------------------------
int SphereInterpolate::Evaluate (const double thetaAngle, const double phiAngle, EvaluateData& f) const 
{
    return pInterp->Evaluate(thetaAngle,phiAngle,f);
}
//---------------------------------------------------------------------------
