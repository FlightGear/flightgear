/*****************************************************************************

 Module:       FGVoronoi.cpp
 Author:       Christian Mayer
 Date started: 28.05.99

 -------- Copyright (C) 1999 Christian Mayer (fgfs@christianmayer.de) --------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 details.

 You should have received a copy of the GNU General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU General Public License can also be found on
 the world wide web at http://www.gnu.org.

FUNCTIONAL DESCRIPTION
------------------------------------------------------------------------------
library for Voronoi Diagram calculation based on Steven Fortune 'Sweep2'
FGVoronoi is the wraper to feed the voronoi calulation with a vetor of points
and any class you want, as it uses templates
NOTE: Sweep2 didn't free *any* memory. So I'm doing what I can, but that's not
      good enough...

HISTORY
------------------------------------------------------------------------------
30.05.99   Christian Mayer	Created
16.06.99   Durk Talsma		Portability for Linux
11.10.1999 Christian Mayer	changed set<> to map<> on Bernie Bright's 
				suggestion
19.10.1999 Christian Mayer	change to use PLIB's sg instead of Point[2/3]D
				and lots of wee code cleaning
*****************************************************************************/

/****************************************************************************/
/* INCLUDES								    */
/****************************************************************************/
#include "FGVoronoi.h"
#include <Voronoi/voronoi.h>
#include <Voronoi/my_memory.h>

extern "C" {
#include <Voronoi/defs.h>

//forward definitions
void voronoi(int triangulate, struct Site *(*nextsite)());
void geominit(void);
void freeinit(struct Freelist *fl, int size);
struct Site *nextone(void);
bool readsites(PointList input);
};

/****************************************************************************/
/********************************** CODE ************************************/
/****************************************************************************/
FGVoronoiOutputList Voronoiate(const FGVoronoiInputList& input)
{
    FGVoronoiOutputList ret_list;

    PointList p2ds;

    FGVoronoiInputList::const_iterator it1;

    //get the points
    for (it1 = input.begin(); it1 != input.end(); it1++)
    {
	p2ds.push_back(VoronoiPoint(it1->position[0], it1->position[1]));
    }

    cl.clear();	//make sure that it's empty

    if (readsites(p2ds) == false)
	return ret_list;

    freeinit(&sfl, sizeof *sites);
    siteidx = 0;
    geominit();
    voronoi(triangulate, nextone); 

    _my_free();
    /*free(sites);    //to prevent a *big* memory leak...
    free(ELhash);   //Sweep2 didn't free *any* memory...
    free(PQhash);*/

    for (cellList::iterator it2 = cl.begin(); it2 != cl.end(); it2++)
    {	
	it2->sort();
	it2->strip();

	//uncomment for debugging
	//cout << *it2;

	Point2DList boundary;

	//copy points
	PointList::iterator it3 = it2->boundary.begin();
	
	if (it3->infinity == false)
	    boundary.push_back( sgVec2Wrap(it3->p) );
	else
	{
	    sgVec2 direction_vector;
	    sgCopyVec2(direction_vector, it3->p);

	    it3++;
	    sgAddVec2(direction_vector, it3->p);

	    boundary.push_back( direction_vector );
	}
	
	for (; it3 != it2->boundary.end(); it3++)
	{
	    boundary.push_back( sgVec2Wrap(it3->p) );
	    
	}

	it3--;
	if (it3->infinity == true)
	{
	    sgVec2 direction_vector;
	    sgCopyVec2(direction_vector, it3->p);

	    it3--;
	    sgAddVec2(direction_vector, it3->p);

	    boundary.pop_back();
	    boundary.push_back(direction_vector);
	}

	ret_list.push_back(FGVoronoiOutput(boundary, input[it2->ID].value));

    }

    return ret_list;

}

extern "C" 
{

/* return a single in-storage site */
struct Site *nextone(void)
{
    struct Site *s;
    if(siteidx < nsites)
    {	
	s = &sites[siteidx];
	siteidx ++;
	return(s);
    }
    else	
	return( (struct Site *)NULL);
}


/* sort sites on y, then x, coord */
//int scomp(const struct Point *s1, const struct Point *s2)
int scomp(const void *s1, const void *s2)
{
    if(((Point*)s1) -> y < ((Point*)s2) -> y) return(-1);
    if(((Point*)s1) -> y > ((Point*)s2) -> y) return(1);
    if(((Point*)s1) -> x < ((Point*)s2) -> x) return(-1);
    if(((Point*)s1) -> x > ((Point*)s2) -> x) return(1);
    return(0);
}



/* read all sites, sort, and compute xmin, xmax, ymin, ymax */
bool readsites(PointList input)
{
    int i;

    if (input.size() == 0)
	return false;	//empty array

    PointList::iterator It = input.begin();

    nsites=0;
    sites = (struct Site *) myalloc(4000*sizeof(*sites));

    while(It != input.end())
    {	
	sites[nsites].coord.x = It->p[0];
	sites[nsites].coord.y = It->p[1];

	sites[nsites].sitenbr = nsites;
	sites[nsites].refcnt = 0;
	nsites ++;
	It++;

	if (nsites % 4000 == 0) 
	    sites = (struct Site *) my_realloc(sites,(nsites+4000)*sizeof(*sites));
    };

    qsort(sites, nsites, sizeof (*sites), scomp);
    xmin=sites[0].coord.x; 
    xmax=sites[0].coord.x;
    
    for(i=1; i<nsites; i+=1)
    {	
	if(sites[i].coord.x < xmin) 
	    xmin = sites[i].coord.x;
	if(sites[i].coord.x > xmax) 
	    xmax = sites[i].coord.x;
    };
    
    ymin = sites[0].coord.y;
    ymax = sites[nsites-1].coord.y;

    return true;
}


}


