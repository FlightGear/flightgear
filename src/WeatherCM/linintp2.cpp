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
#include STL_IOSTREAM
#include <simgear/debug/logstream.hxx>

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include "linintp2.h"

SG_USING_NAMESPACE(std);
SG_USING_STD(cout);

//---------------------------------------------------------------------------
mgcLinInterp2D::mgcLinInterp2D (int _numPoints, double* x, double* y, 
                                   unsigned int* _f)
{
    if ( (numPoints = _numPoints) < 3 )
    {
        point = 0;
        edge = 0;
        triangle = 0;
        numTriangles = 0;
        return;
    }

    SG_LOG(SG_MATH, SG_DEBUG, "[ 20%] allocating memory");

    point = new double*[numPoints];
    tmppoint = new double*[numPoints+3];
    f = new unsigned int[numPoints];
    int i;
    for (i = 0; i < numPoints; i++)
        point[i] = new double[2];
    for (i = 0; i < numPoints+3; i++)
        tmppoint[i] = new double[2];
    for (i = 0; i < numPoints; i++)
    {
        point[i][0] = tmppoint[i][0] = x[i];
        point[i][1] = tmppoint[i][1] = y[i];

	f[i] = _f[i];
    }

    SG_LOG(SG_MATH, SG_DEBUG, "[ 30%] creating delaunay diagram");

    Delaunay2D();
}
//---------------------------------------------------------------------------
mgcLinInterp2D::~mgcLinInterp2D ()
{
    if ( numPoints < 3 )
        return;

    int i;

    if ( point )
    {
        for (i = 0; i < numPoints; i++)
            delete[] point[i];
        delete[] point;
    }
    if ( tmppoint )
    {
        for (i = 0; i < numPoints+3; i++)
            delete[] tmppoint[i];
        delete[] tmppoint;
    }

    delete[] f;
    delete[] edge;
    delete[] triangle;
}
//---------------------------------------------------------------------------
void mgcLinInterp2D::ComputeBarycenter (Vertex& v0, Vertex& v1, Vertex& v2, 
                                           Vertex& ver, double c[3])
{
    double a0 = v0.x-v2.x;
    double b0 = v0.y-v2.y;
    double a1 = v1.x-v2.x;
    double b1 = v1.y-v2.y;
    double a2 = ver.x-v2.x;
    double b2 = ver.y-v2.y;

    double m00 = a0*a0+b0*b0;
    double m01 = a0*a1+b0*b1;
    double m11 = a1*a1+b1*b1;
    double r0 = a2*a0+b2*b0;
    double r1 = a2*a1+b2*b1;
    double det = m00*m11-m01*m01;

    c[0] = (m11*r0-m01*r1)/det;
    c[1] = (m00*r1-m01*r0)/det;
    c[2] = 1-c[0]-c[1];
}
//---------------------------------------------------------------------------
int mgcLinInterp2D::InTriangle (Vertex& v0, Vertex& v1, Vertex& v2, 
                                   Vertex& test)
{
    const double eps = 1e-08;
    double tx, ty, nx, ny;

    // test against normal to first edge
    tx = test.x - v0.x;
    ty = test.y - v0.y;
    nx = v0.y - v1.y;
    ny = v1.x - v0.x;
    if ( tx*nx + ty*ny < -eps )
        return 0;

    // test against normal to second edge
    tx = test.x - v1.x;
    ty = test.y - v1.y;
    nx = v1.y - v2.y;
    ny = v2.x - v1.x;
    if ( tx*nx + ty*ny < -eps )
        return 0;

    // test against normal to third edge
    tx = test.x - v2.x;
    ty = test.y - v2.y;
    nx = v2.y - v0.y;
    ny = v0.x - v2.x;
    if ( tx*nx + ty*ny < -eps )
        return 0;

    return 1;
}
//---------------------------------------------------------------------------
int mgcLinInterp2D::Evaluate (double x, double y, EvaluateData& F)
{
    Vertex ver = { x, y };
    // determine which triangle contains the target point

    int i; 
    Vertex v0, v1, v2;
    for (i = 0; i < numTriangles; i++)
    {
        Triangle& t = triangle[i];
        v0.x = point[t.vertex[0]][0];
        v0.y = point[t.vertex[0]][1];
        v1.x = point[t.vertex[1]][0];
        v1.y = point[t.vertex[1]][1];
        v2.x = point[t.vertex[2]][0];
        v2.y = point[t.vertex[2]][1];

        if ( InTriangle(v0,v1,v2,ver) )
            break;
    }

    if ( i == numTriangles )  // point is outside interpolation region
    {
	return 0;
    }

    Triangle& t = triangle[i];  // (x,y) is in this triangle

    // compute barycentric coordinates with respect to subtriangle
    double bary[3];
    ComputeBarycenter(v0,v1,v2,ver,bary);

    // compute barycentric combination of function values at vertices
    F.index[0] = f[t.vertex[0]];
    F.index[1] = f[t.vertex[1]];
    F.index[2] = f[t.vertex[2]];
    F.percentage[0] = bary[0];
    F.percentage[1] = bary[1];
    F.percentage[2] = bary[2];

    return 1;
}
//---------------------------------------------------------------------------
int mgcLinInterp2D::Delaunay2D ()
{
    int result;

    const double EPSILON = 1e-12;
    const int TSIZE = 75;
    const double RANGE = 10.0;

    xmin = tmppoint[0][0];
    xmax = xmin;
    ymin = tmppoint[0][1];
    ymax = ymin;

    int i;
    for (i = 0; i < numPoints; i++)
    {
        double value = tmppoint[i][0];
        if ( xmax < value )
            xmax = value;
        if ( xmin > value )
            xmin = value;

        value = tmppoint[i][1];
        if ( ymax < value )
            ymax = value;
        if ( ymin > value )
            ymin = value;
    }

    double xrange = xmax-xmin, yrange = ymax-ymin;
    double maxrange = xrange;
    if ( maxrange < yrange )
        maxrange = yrange;

    // need to scale the data later to do a correct triangle count
    double maxrange2 = maxrange*maxrange;

    // tweak the points by very small random numbers
    double bgs = EPSILON*maxrange;
    srand(367);   
    for (i = 0; i < numPoints; i++) 
    {
        tmppoint[i][0] += bgs*(0.5 - rand()/double(RAND_MAX));
        tmppoint[i][1] += bgs*(0.5 - rand()/double(RAND_MAX));
    }

    double wrk[2][3] =
    {
        { 5*RANGE, -RANGE, -RANGE },
        { -RANGE, 5*RANGE, -RANGE }
    };
    for (i = 0; i < 3; i++)
    {
        tmppoint[numPoints+i][0] = xmin+xrange*wrk[0][i];
        tmppoint[numPoints+i][1] = ymin+yrange*wrk[1][i];
    }

    int i0, i1, i2, i3, i4, i5, i6, i7, i8, i9, i11;
    int nts, ii[3];
    double xx;

    int tsz = 2*TSIZE;
    int** tmp = new int*[tsz+1];
    tmp[0] = new int[2*(tsz+1)];
    for (i0 = 1; i0 < tsz+1; i0++)
        tmp[i0] = tmp[0] + 2*i0;
    i1 = 2*(numPoints + 2);

    int* id = new int[i1];
    for (i0 = 0; i0 < i1; i0++) 
        id[i0] = i0; 

    int** a3s = new int*[i1];
    a3s[0] = new int[3*i1];
    for (i0 = 1; i0 < i1; i0++)
        a3s[i0] = a3s[0] + 3*i0;
    a3s[0][0] = numPoints;
    a3s[0][1] = numPoints+1;
    a3s[0][2] = numPoints+2;

    double** ccr = new double*[i1];  // circumscribed centers and radii
    ccr[0] = new double[3*i1];
    for (i0 = 1; i0 < i1; i0++)
        ccr[i0] = ccr[0] + 3*i0;
    ccr[0][0] = 0.0;
    ccr[0][1] = 0.0;
    ccr[0][2] = FLT_MAX;

    nts = 1;  // number of triangles
    i4 = 1;

    SG_LOG(SG_MATH, SG_DEBUG, "[ 40%] create triangulation");

    // compute triangulation
    for (i0 = 0; i0 < numPoints; i0++)
    {  
        i1 = i7 = -1;
        i9 = 0;
        for (i11 = 0; i11 < nts; i11++)
        {
            i1++;
            while ( a3s[i1][0] < 0 ) 
                i1++;
            xx = ccr[i1][2];
            for (i2 = 0; i2 < 2; i2++)
            {  
                double z = tmppoint[i0][i2]-ccr[i1][i2];
                xx -= z*z;
                if ( xx < 0 ) 
                    goto Corner3;
            }
            i9--;
            i4--;
            id[i4] = i1;
            for (i2 = 0; i2 < 3; i2++)
            {  
                ii[0] = 0;
                if (ii[0] == i2) 
                    ii[0]++;
                for (i3 = 1; i3 < 2; i3++)
                {  
                    ii[i3] = ii[i3-1] + 1;
                    if (ii[i3] == i2) 
                        ii[i3]++;
                }
                if ( i7 > 1 )
                {  
                    i8 = i7;
                    for (i3 = 0; i3 <= i8; i3++)
                    {  
                        for (i5 = 0; i5 < 2; i5++) 
                            if ( a3s[i1][ii[i5]] != tmp[i3][i5] ) 
                                goto Corner1;
                        for (i6 = 0; i6 < 2; i6++) 
                            tmp[i3][i6] = tmp[i8][i6];
                        i7--;
                        goto Corner2;
Corner1:;
                    }
                }
                if ( ++i7 > tsz )
                {
                    // temporary storage exceeded, increase TSIZE
                    result = 0;
                    goto ExitDelaunay;
                }
                for (i3 = 0; i3 < 2; i3++) 
                    tmp[i7][i3] = a3s[i1][ii[i3]];
Corner2:;
            }
            a3s[i1][0] = -1;
Corner3:;
        }

        for (i1 = 0; i1 <= i7; i1++)
        {  
            for (i2 = 0; i2 < 2; i2++)
                for (wrk[i2][2] = 0, i3 = 0; i3 < 2; i3++)
                {  
                    wrk[i2][i3] = tmppoint[tmp[i1][i2]][i3]-tmppoint[i0][i3];
                    wrk[i2][2] += 
                        0.5*wrk[i2][i3]*(tmppoint[tmp[i1][i2]][i3]+
                        tmppoint[i0][i3]);
                }

            xx = wrk[0][0]*wrk[1][1]-wrk[1][0]*wrk[0][1];
            ccr[id[i4]][0] = (wrk[0][2]*wrk[1][1]-wrk[1][2]*wrk[0][1])/xx;
            ccr[id[i4]][1] = (wrk[0][0]*wrk[1][2]-wrk[1][0]*wrk[0][2])/xx;

            for (ccr[id[i4]][2] = 0, i2 = 0; i2 < 2; i2++) 
            {  
                double z = tmppoint[i0][i2]-ccr[id[i4]][i2];
                ccr[id[i4]][2] += z*z;
                a3s[id[i4]][i2] = tmp[i1][i2];
            }

            a3s[id[i4]][2] = i0;
            i4++;
            i9++;
        }
        nts += i9;
    }

    // count the number of triangles
    SG_LOG(SG_MATH, SG_DEBUG, "[ 50%] count the number of triangles");

    numTriangles = 0;
    i0 = -1;
    for (i11 = 0; i11 < nts; i11++)
    {  
        i0++;
        while ( a3s[i0][0] < 0 ) 
            i0++;
        if ( a3s[i0][0] < numPoints )
        {  
            for (i1 = 0; i1 < 2; i1++) 
                for (i2 = 0; i2 < 2; i2++) 
                    wrk[i1][i2] = 
                        tmppoint[a3s[i0][i1]][i2]-tmppoint[a3s[i0][2]][i2];

            if ( fabs(wrk[0][0]*wrk[1][1]-wrk[0][1]*wrk[1][0]) > EPSILON*maxrange2 )
                numTriangles++;
        }
    }

    // create the triangles
    SG_LOG(SG_MATH, SG_DEBUG "[ 60%] create the triangles");

    triangle = new Triangle[numTriangles];

    numTriangles = 0;
    i0 = -1;
    for (i11 = 0; i11 < nts; i11++)
    {  
        i0++;
        while ( a3s[i0][0] < 0 ) 
            i0++;
        if ( a3s[i0][0] < numPoints )
        {  
            for (i1 = 0; i1 < 2; i1++) 
                for (i2 = 0; i2 < 2; i2++) 
                    wrk[i1][i2] = 
                        tmppoint[a3s[i0][i1]][i2]-tmppoint[a3s[i0][2]][i2];
            xx = wrk[0][0]*wrk[1][1]-wrk[0][1]*wrk[1][0];
            if ( fabs(xx) > EPSILON*maxrange2 )
            {  
                int delta = xx < 0 ? 1 : 0;
                Triangle& tri = triangle[numTriangles];
                tri.vertex[0] = a3s[i0][0];
                tri.vertex[1] = a3s[i0][1+delta];
                tri.vertex[2] = a3s[i0][2-delta];
                tri.adj[0] = -1;
                tri.adj[1] = -1;
                tri.adj[2] = -1;
                numTriangles++;
            }
        }
    }

    // build edge table
    SG_LOG(SG_MATH, SG_DEBUG, "[ 70%] build the edge table");

    numEdges = 0;
    edge = new Edge[3*numTriangles];

    int j, j0, j1;
    for (i = 0; i < numTriangles; i++)
    {
	if ( (i%500) == 0)
	    SG_LOG(SG_MATH, SG_BULK, "[ 7" << 10*i/numTriangles << "%] build the edge table");

        Triangle& t = triangle[i];

        for (j0 = 0, j1 = 1; j0 < 3; j0++, j1 = (j1+1)%3)
        {
            for (j = 0; j < numEdges; j++)
            {
                Edge& e = edge[j];
                if ( (t.vertex[j0] == e.vertex[0] 
                   && t.vertex[j1] == e.vertex[1])
                ||   (t.vertex[j0] == e.vertex[1] 
                   && t.vertex[j1] == e.vertex[0]) )
                    break;
            }
            if ( j == numEdges )  // add edge to table
            {
                edge[j].vertex[0] = t.vertex[j0];
                edge[j].vertex[1] = t.vertex[j1];
                edge[j].triangle[0] = i;
                edge[j].index[0] = j0;
                edge[j].triangle[1] = -1;
                numEdges++;
            }
            else  // edge already exists, add triangle to table
            {
                edge[j].triangle[1] = i;
                edge[j].index[1] = j0;
            }
        }
    }

    // establish links between adjacent triangles
    SG_LOG(SG_MATH, SG_DEBUG, "[ 80%] establishing links between adjacent triangles");

    for (i = 0; i < numEdges; i++)
    {
        if ( edge[i].triangle[1] != -1 )
        {
            j0 = edge[i].triangle[0];
            j1 = edge[i].triangle[1];
            triangle[j0].adj[edge[i].index[0]] = j1;
            triangle[j1].adj[edge[i].index[1]] = j0;
        }
    }

    result = 1;

ExitDelaunay:;
    delete[] tmp[0];
    delete[] tmp;
    delete[] id;
    delete[] a3s[0];
    delete[] a3s;
    delete[] ccr[0];
    delete[] ccr;

    SG_LOG(SG_MATH, SG_DEBUG, "[ 90%] finsishes delauney triangulation");

    return result;
}
//---------------------------------------------------------------------------
void mgcLinInterp2D::GetPoint (int i, double& x, double& y)
{
    // assumes i is valid [can use PointCount() before passing i]
    x = point[i][0];
    y = point[i][1];
}
//---------------------------------------------------------------------------
void mgcLinInterp2D::GetEdge (int i, double& x0, double& y0, double& x1,
                                 double& y1)
{
    // assumes i is valid [can use EdgeCount() before passing i]
    int v0 = edge[i].vertex[0], v1 = edge[i].vertex[1];

    x0 = point[v0][0];
    y0 = point[v0][1];
    x1 = point[v1][0];
    y1 = point[v1][1];
}
//---------------------------------------------------------------------------
void mgcLinInterp2D::GetTriangle (int i, double& x0, double& y0, double& x1,
                                     double& y1, double& x2, double& y2)
{
    // assumes i is valid [can use TriangleCount() before passing i]
    int v0 = triangle[i].vertex[0];
    int v1 = triangle[i].vertex[1];
    int v2 = triangle[i].vertex[2];

    x0 = point[v0][0];
    y0 = point[v0][1];
    x1 = point[v1][0];
    y1 = point[v1][1];
    x2 = point[v2][0];
    y2 = point[v2][1];
}
//---------------------------------------------------------------------------

