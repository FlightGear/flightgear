#ifndef _SSG_ENTITY_ARRAY_HXX
#define _SSG_ENTITY_ARRAY_HXX

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <plib/ssg.h>


class ssgEntityArray : public ssgEntity
{
    // The replicated child
    ssgEntity *model ;

    // The one transformation node
    ssgTransform *pos;

    // The list of locations and orientations
    ssgVertexArray *locations;
    ssgVertexArray *orientations;

protected:

    virtual void copy_from ( ssgEntityArray *src, int clone_flags ) ;

public:

    virtual void zeroSpareRecursive ();

    virtual ssgBase *clone ( int clone_flags = 0 ) ;
    ssgEntityArray (void) ;
    virtual ~ssgEntityArray (void) ;

    ssgEntity *getModel () const { return model ; }
    void setModel        ( ssgEntity *entity ) { model = entity; }
    void removeModel     () ;
    void replaceModel    ( ssgEntity *new_entity ) ;

    ssgVertexArray *getLocations () const { return locations; }
    ssgVertexArray *getOrientations () const { return orientations; }

    float *getLocation ( int i ) const { return locations->get( i ); }
    float *getOrientation ( int i ) const { return orientations->get( i ); }
    void addPlacement ( sgVec3 loc, sgVec3 orient );
    virtual int getNumPlacements() const { return locations->getNum(); }
    void removeAllPlacements();

    ssgTransform *getPosTransform() { return pos; }

    virtual const char *getTypeName(void) ;
    virtual void cull          ( sgFrustum *f, sgMat4 m, int test_needed ) ;
    virtual void isect         ( sgSphere  *s, sgMat4 m, int test_needed ) ;
    virtual void hot           ( sgVec3     s, sgMat4 m, int test_needed ) ;
    virtual void los           ( sgVec3     s, sgMat4 m, int test_needed ) ;
    virtual void print         ( FILE *fd = stderr, char *indent = "", int how_much = 2 ) ;
#ifdef HAVE_PLIB_PSL
    virtual void getStats ( int *num_branches, int *num_leaves, int *num_tris, int *num_vertices ) ;
#endif
    virtual int load ( FILE *fd ) ;
    virtual int save ( FILE *fd ) ;
    virtual void recalcBSphere () ;
} ;


#endif // _SSG_ENTITY_ARRAY_HXX
