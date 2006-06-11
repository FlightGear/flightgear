// hitlist.hxx
// Height Over Terrain and Assosciated Routines for FlightGear based Scenery
// Written by Norman Vine, started 2000.

#ifndef _HITLIST_HXX
#define _HITLIST_HXX

#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   

#include <simgear/compiler.h>

#include <vector>
SG_USING_STD(vector);

#include <plib/ssg.h>

class ssgEntity;
class ssgBranch;

class FGHitRec {

private:
    ssgEntity *ent;
    int index;
    sgdVec3 point;
    sgdVec3 normal;

public:

    FGHitRec( ssgEntity *e, int idx, sgdVec3 p, sgdVec3 n ) {
        ent = e;
        index = idx;
        sgdSetVec3(point,p[0],p[1],p[2]);
        sgdSetVec3(normal,n[0],n[1],n[2]);
    }

    ssgEntity *get_entity(void) { return ent; }
    int get_face(void)          { return index; }
    double *get_point(void)     { return point; }
    double *get_normal(void)    { return normal; }
};


class FGHitList {

private:

    ssgEntity *last;
    vector < FGHitRec > list;
    double test_dist;

public:

    FGHitList();
    ~FGHitList();

    void init(void) { list.clear(); test_dist=DBL_MAX; }
    void clear(void) { init(); last = NULL; }
    void add( ssgEntity *ent, int idx, sgdVec3 point, sgdVec3 normal ) {
        list.push_back( FGHitRec( ent,idx,point,normal) );
        last = ent;
    }
    int num_hits(void) { return list.size(); }
    ssgEntity *get_entity(int i)  { return list[i].get_entity(); }
    ssgEntity *last_hit(void)     { return last; }
    int get_face(int i)           { return list[i].get_face(); }
    double *get_point(int i)      { return list[i].get_point(); }
    double *get_normal(int i)     { return list[i].get_normal(); }

    void Intersect( ssgBranch *branch, sgdVec3 orig, sgdVec3 dir );
    void Intersect( ssgBranch *scene, sgdMat4 m, sgdVec3 orig, sgdVec3 dir );

    void IntersectBranch( ssgBranch *branch, sgdMat4 m, sgdVec3 orig, sgdVec3 dir);

    int IntersectLeaf( ssgLeaf *leaf, sgdMat4 m,
                       sgdVec3 orig, sgdVec3 dir );

    int IntersectLeaf( ssgLeaf *leaf, sgdMat4 m,
                       sgdVec3 orig, sgdVec3 dir,
                       GLenum primType );
};


// Associated functions, assuming a wgs84 world with 0,0,0 at the
// center, find the current terrain intersection elevation for the
// point specified.
bool fgCurrentElev( sgdVec3 abs_view_pos,
                    double max_alt_m,
                    sgdVec3 scenery_center,
                    ssgBranch *branch,
                    FGHitList *hit_list,
                    double *terrain_elev,
                    double *radius,
                    double *normal,
		    int & this_hit );

bool fgCurrentElev( sgdVec3 abs_view_pos,
                    double max_alt_m,
                    sgdVec3 scenery_center,
                    FGHitList *hit_list,
                    double *terrain_elev,
                    double *radius,
                    double *normal );

#endif // _HITLIST_HXX
