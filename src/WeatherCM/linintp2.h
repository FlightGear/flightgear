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

#ifndef LININTP2_H
#define LININTP2_H

template<class T>
class mgcLinInterp2D
{
public:
    mgcLinInterp2D (int _numPoints, double* x, double* y, T* _f);

    ~mgcLinInterp2D ();

    double XMin () { return xmin; }
    double XMax () { return xmax; }
    double XRange () { return xmax-xmin; }
    double YMin () { return ymin; }
    double YMax () { return ymax; }
    double YRange () { return ymax-ymin; }

    int PointCount () { return numPoints; }
    void GetPoint (int i, double& x, double& y);

    int EdgeCount () { return numEdges; }
    void GetEdge (int i, double& x0, double& y0, double& x1, double& y1);

    int TriangleCount () { return numTriangles; }
    void GetTriangle (int i, double& x0, double& y0, double& x1, double& y1,
        double& x2, double& y2);

    int Evaluate (double x, double y, T& F);
    
private:
    typedef struct
    {
        double x, y;
    }
    Vertex;

    typedef struct
    {
        int vertex[3];  // listed in counterclockwise order

        int adj[3];
            // adj[0] points to triangle sharing edge (vertex[0],vertex[1])
            // adj[1] points to triangle sharing edge (vertex[1],vertex[2])
            // adj[2] points to triangle sharing edge (vertex[2],vertex[0])
    }
    Triangle;

    typedef struct 
    {
        int vertex[2];
        int triangle[2];
        int index[2];
    } 
    Edge;

    int numPoints;
    double** point;
    double** tmppoint;
    T* f;

    double xmin, xmax, ymin, ymax;


    int numEdges;
    Edge* edge;

    int numTriangles;
    Triangle* triangle;

    int Delaunay2D ();
    void ComputeBarycenter (Vertex& v0, Vertex& v1, Vertex& v2, Vertex& ver, 
                            double c[3]);
    int InTriangle (Vertex& v0, Vertex& v1, Vertex& v2, Vertex& test);
};

#include "linintp2.inl"

#endif
