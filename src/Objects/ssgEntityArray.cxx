/*
     PLIB - A Suite of Portable Game Libraries
     Copyright (C) 1998,2002  Steve Baker
 
     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Library General Public
     License as published by the Free Software Foundation; either
     version 2 of the License, or (at your option) any later version.
 
     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Library General Public License for more details.
 
     You should have received a copy of the GNU Library General Public
     License along with this library; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 
     For further information visit http://plib.sourceforge.net

     $Id$
*/


#include "ssgEntityArray.hxx"


// Forward declaration of internal ssg stuff (for hot/isec/los/etc.)
void _ssgPushPath ( ssgEntity *l ) ;
void _ssgPopPath () ;


void ssgEntityArray::copy_from ( ssgEntityArray *src, int clone_flags )
{
  ssgEntity::copy_from ( src, clone_flags ) ;

  ssgEntity *k = src -> getModel ( ) ;
  if ( k != NULL && ( clone_flags & SSG_CLONE_RECURSIVE ) )
      setModel ( (ssgEntity *)( k -> clone ( clone_flags )) ) ;
  else
      setModel ( k ) ;

  ssgTransform *t = src -> getPosTransform();
  if ( t != NULL && ( clone_flags & SSG_CLONE_RECURSIVE ) )
      pos = (ssgTransform *)( t -> clone ( clone_flags ) );
  else
      pos = t;

  ssgVertexArray *v = src -> getLocations();
  if ( v != NULL && ( clone_flags & SSG_CLONE_RECURSIVE ) )
      locations = (ssgVertexArray *)( v -> clone ( clone_flags ) );
  else
      locations = v;

  v = src -> getOrientations();
  if ( v != NULL && ( clone_flags & SSG_CLONE_RECURSIVE ) )
      orientations = (ssgVertexArray *)( v -> clone ( clone_flags ) );
  else
      orientations = v;
}

ssgBase *ssgEntityArray::clone ( int clone_flags )
{
  ssgEntityArray *b = new ssgEntityArray ;
  b -> copy_from ( this, clone_flags ) ;
  return b ;
}



ssgEntityArray::ssgEntityArray (void)
{
    type = ssgTypeBranch () ;
    pos = new ssgTransform;
    locations = new ssgVertexArray();
    orientations = new ssgVertexArray();
}


ssgEntityArray::~ssgEntityArray (void)
{
    removeModel() ;
    ssgDeRefDelete( pos );
    locations->removeAll();
    orientations->removeAll();

    delete orientations;
    delete locations;
    delete pos;
}


void ssgEntityArray::zeroSpareRecursive ()
{
  zeroSpare () ;

  model -> zeroSpareRecursive () ;
  pos -> zeroSpareRecursive () ;
  locations -> zeroSpareRecursive () ;
  orientations -> zeroSpareRecursive () ;
}


void ssgEntityArray::recalcBSphere (void)
{
  emptyBSphere () ;

  pos->removeAllKids();
  pos->addKid( model );

  for ( int i = 0; i < locations->getNum(); ++i ) {
      sgCoord c;
      sgSetCoord( &c, locations->get(i), orientations->get(i) );
      pos->setTransform( &c );
      extendBSphere( pos->getBSphere() );
  }

  pos->removeAllKids();

  /* FIXME: Traverse placement list
  for ( ssgEntity *k = getKid ( 0 ) ; k != NULL ; k = getNextKid () )
    extendBSphere ( k -> getBSphere () ) ;
  */

  bsphere_is_invalid = FALSE ;
}


void ssgEntityArray::removeModel ()
{
    model->deadBeefCheck () ;
    ssgDeRefDelete ( model ) ;
}


void ssgEntityArray::replaceModel ( ssgEntity *new_entity )
{
    removeModel();
    setModel( new_entity );
}


void ssgEntityArray::addPlacement ( sgVec3 loc, sgVec3 orient )
{
    locations->add( loc ) ;
    orientations->add( orient ) ;
    dirtyBSphere () ;
}


void ssgEntityArray::removeAllPlacements()
{
    locations->removeAll();
    orientations->removeAll();
    dirtyBSphere () ;
}


void ssgEntityArray::print ( FILE *fd, char *indent, int how_much )
{
  ssgEntity::print ( fd, indent, how_much ) ;
  fprintf ( fd, "%s  Num Kids=%d\n", indent, getNumKids() ) ;

  if ( getNumParents() != getRef() )
    ulSetError ( UL_WARNING, "Ref count doesn't tally with parent count" ) ;

	if ( how_much > 1 )
  {	if ( bsphere.isEmpty() )
			fprintf ( fd, "%s  BSphere is Empty.\n", indent ) ;
		else
			fprintf ( fd, "%s  BSphere  R=%g, C=(%g,%g,%g)\n", indent,
				bsphere.getRadius(), bsphere.getCenter()[0], bsphere.getCenter()[1], bsphere.getCenter()[2] ) ;
	}

  char in [ 100 ] ;
  sprintf ( in, "%s  ", indent ) ;

  model -> print ( fd, in, how_much ) ;
}


void ssgEntityArray::getStats ( int *num_branches, int *num_leaves, int *num_tris, int *num_verts )
{
  int nb, nl, nt, nv ;

  *num_branches = 1 ;   /* this! */
  *num_leaves   = 0 ;
  *num_tris     = 0 ;
  *num_verts    = 0 ;

  model -> getStats ( & nb, & nl, & nt, & nv ) ;
  *num_branches += nb * locations->getNum() ;
  *num_leaves   += nl * locations->getNum() ;
  *num_tris     += nt * locations->getNum() ;
  *num_verts    += nv * locations->getNum() ;
}


void ssgEntityArray::cull ( sgFrustum *f, sgMat4 m, int test_needed )
{
  if ( ! preTravTests ( &test_needed, SSGTRAV_CULL ) )
    return ;

  int cull_result = cull_test ( f, m, test_needed ) ;

  if ( cull_result == SSG_OUTSIDE )
    return ;

  pos->removeAllKids();
  pos->addKid( model );

  for ( int i = 0; i < locations->getNum(); ++i ) {
      sgCoord c;
      sgSetCoord( &c, locations->get(i), orientations->get(i) );
      pos->setTransform( &c );
      pos->cull( f, m, cull_result != SSG_INSIDE );
  }

  pos->removeAllKids();

  postTravTests ( SSGTRAV_CULL ) ; 
}



void ssgEntityArray::hot ( sgVec3 s, sgMat4 m, int test_needed )
{
  if ( ! preTravTests ( &test_needed, SSGTRAV_HOT ) )
    return ;

  int hot_result = hot_test ( s, m, test_needed ) ;

  if ( hot_result == SSG_OUTSIDE )
    return ;

  _ssgPushPath ( this ) ;

  pos->removeAllKids();
  pos->addKid( model );

  for ( int i = 0; i < locations->getNum(); ++i ) {
      sgCoord c;
      sgSetCoord( &c, locations->get(i), orientations->get(i) );
      pos->setTransform( &c );
      pos->hot ( s, m, hot_result != SSG_INSIDE );
  }

  pos->removeAllKids();

  _ssgPopPath () ;

  postTravTests ( SSGTRAV_HOT ) ;
}



void ssgEntityArray::los ( sgVec3 s, sgMat4 m, int test_needed )
{
  if ( ! preTravTests ( &test_needed, SSGTRAV_LOS ) )
    return ;

  int los_result = los_test ( s, m, test_needed ) ;

  if ( los_result == SSG_OUTSIDE )
    return ;

  _ssgPushPath ( this ) ;

  pos->removeAllKids();
  pos->addKid( model );

  for ( int i = 0; i < locations->getNum(); ++i ) {
      sgCoord c;
      sgSetCoord( &c, locations->get(i), orientations->get(i) );
      pos->setTransform( &c );
      pos->los ( s, m, los_result != SSG_INSIDE ) ;
  }

  pos->removeAllKids();

  _ssgPopPath () ;

  postTravTests ( SSGTRAV_LOS) ;
}


void ssgEntityArray::isect ( sgSphere *s, sgMat4 m, int test_needed )
{
  if ( ! preTravTests ( &test_needed, SSGTRAV_ISECT ) )
    return ;

  int isect_result = isect_test ( s, m, test_needed ) ;

  if ( isect_result == SSG_OUTSIDE )
    return ;

  _ssgPushPath ( this ) ;

  pos->removeAllKids();
  pos->addKid( model );

  for ( int i = 0; i < locations->getNum(); ++i ) {
      sgCoord c;
      sgSetCoord( &c, locations->get(i), orientations->get(i) );
      pos->setTransform( &c );
      pos->isect ( s, m, isect_result != SSG_INSIDE ) ;
  }

  pos->removeAllKids();

  _ssgPopPath () ;

  postTravTests ( SSGTRAV_ISECT ) ; 
}


#if 0
int ssgEntityArray::load ( FILE *fd )
{
  int nkids ;

  _ssgReadInt ( fd, & nkids ) ;

  if ( ! ssgEntity::load ( fd ) )
    return FALSE ;

  for ( int i = 0 ; i < nkids ; i++ )
  {
    ssgEntity *kid ;

    if ( ! _ssgLoadObject ( fd, (ssgBase **) &kid, ssgTypeEntity () ) )
      return FALSE ;

    addKid ( kid ) ;
  }

  return TRUE ;
}


int ssgEntityArray::save ( FILE *fd )
{
  _ssgWriteInt ( fd, getNumKids() ) ;

  if ( ! ssgEntity::save ( fd ) )
    return FALSE ;

  for ( int i = 0 ; i < getNumKids() ; i++ )
  {
    if ( ! _ssgSaveObject ( fd, getKid ( i ) ) )
       return FALSE ;
  }

  return TRUE ;
}
#endif

