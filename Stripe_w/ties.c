/********************************************************************/
/*   STRIPE: converting a polygonal model to triangle strips    
     Francine Evans, 1996.
     SUNY @ Stony Brook
     Advisors: Steven Skiena and Amitabh Varshney
*/
/********************************************************************/

/*---------------------------------------------------------------------*/
/*	STRIPE: ties.c
     This file will contain all the routines used to determine the next face if there
	is a tie
*/
/*---------------------------------------------------------------------*/

#include <stdlib.h>
#include "polverts.h"
#include "ties.h"
#include "sturctsex.h"
#include "triangulatex.h"
#include "options.h"
#include "common.h"
#include "util.h"

#define MAX_TIE 60
int ties_array[60];
int last = 0;

void Clear_Ties()
{
	/*	Clear the buffer, because we do not have the tie
		any more that we had before */
	last = 0;
}

void Add_Ties(int id)
{
	/*	We have a tie to add to the buffer */
	ties_array[last++] = id;
}

int Alternate_Tie()
{
	/*	Alternate in what we choose to break the tie 
		We are just alternating between the first and
		second thing that we found
	*/
	static int x = 0;
	register int t;
	
	t = ties_array[x];
	x++;
	if (x == 2)
		x = 0;
	return t;
}

int Random_Tie()
{
	/*	Randomly choose the next face with which
		to break the tie
	*/
	register int num;

	num = rand();
	while (num >= last)
		num = num/20;
	return (ties_array[num]);
}

int Look_Ahead(int id)
{
	/*	Look ahead at this face and save the minimum
		adjacency of all the faces that are adjacent to
		this face.
	*/
	return Min_Adj(id);
}

int Random_Look(int id[],int count)
{
	/*	We had a tie within a tie in the lookahead, 
		break it randomly 
	*/
	register int num;

	num = rand();
	while (num >= count)
		num = num/20;
	return (id[num]);
}


int Look_Ahead_Tie()
{
	/*	Look ahead and find the face to go to that
		will give the least number of adjacencies
	*/
	int id[60],t,x,f=0,min = 60;

	for (x = 0; x < last; x++)
	{
		t = Look_Ahead(ties_array[x]);
		/*	We have a tie */
		if (t == min)
			id[f++] = ties_array[x];
		if (t < min)
		{
			f = 0;
			min = t;
			id[f++] = ties_array[x];
		}
	}
	/*	No tie within the tie */
	if ( f == 1)
		return id[0];
	/*	Or ties, but we are at the end of strips */
	if (min == 0)
		return id[0];
	return (Random_Look(id,f));
}


int Sequential_Tri(int *index)
{
    /*  We have a triangle and need to break the ties at it.
        We will choose the edge that is sequential. There
        is definitely one since we know we have a triangle
        and that there is a tie and there are only 2 edges
        for the tie.
    */
    int e1,e2,e3,output1,output2,output3,output4;

    /*  e2 and e3 are the input edge to the triangle */
    Last_Edge(&e1,&e2,&e3,0);
    
    if ((e2 == 0) && (e3 == 0))
        /*  Starting the strip, don't need to do this */
        return ties_array[0];

    /*  For the 2 ties find the edge adjacent to face id */
    Get_EdgeEx(&output1,&output2,index,ties_array[0],3,0,0);
    Get_EdgeEx(&output3,&output4,index,ties_array[1],3,0,0);

    if ((output1 == e3) || (output2 == e3))
        return ties_array[0];
    if ((output3 == e3) || (output4 == e3))
        return ties_array[1];
    printf("There is an error trying to break sequential triangle \n");
}

int Sequential_Quad(int *index,	int triangulate)
{
    /*  We have a quad that need to break its ties, we will try
        and choose a side that is sequential, otherwise use lookahead
    */
    int output1,output2,x,e1,e2,e3;

    /*  e2 and e3 are the input edge to the quad */
    Last_Edge(&e1,&e2,&e3,0);

    /*  No input edge */
    if ((e2 == 0) && (e3 == 0))
        return ties_array[0];

	/*  Go through the ties and see if there is a sequential one */
    for (x = 0; x < last; x++)
	{
        Get_EdgeEx(&output1,&output2,index,ties_array[x],4,0,0);
        /*  Partial and whole triangulation will have different requirements */
        if (((output1 == e3) || (output2 == e3)) && (triangulate == PARTIAL))
            return ties_array[x];
        if (((output1 != e3) && (output1 != e2) &&
                (output2 != e3) && (output2 != e2)))
            return ties_array[x];
    }
    /*  There was not a tie that was sequential */
	return Look_Ahead_Tie();
}

void Whole_Output(int in1,int in2, int *index, int size, int *out1, int *out2)
{
    /*  Used to sequentially break ties in the whole triangulation for polygons
        greater than 4 sides. We will find the output edge that is good
        for sequential triangulation.
    */

    int half;
    
    /*  Put the input edge first in the list */
    Rearrange_IndexEx(index,size);

    if (!(EVEN(size)))
    {
        if (*(index) == in1)
            half = size/2 ;
        else
            half = size/2 +1;
    }
    else
        half = size/2;

    *out1 = *(index+half);
    *out2 = *(index+half+1);
}

int Sequential_Poly(int size, int *index, int triangulate)
{
    /*  We have a polygon of greater than 4 sides and wish to break the
        tie in the most sequential manner.
    */

    int x,output1,output2,e1,e2,e3,saved1=-1,saved2=-1,output3,output4;
    
    /*  e2 and e3 are the input edge to the quad */
    Last_Edge(&e1,&e2,&e3,0);

    /*  If we are using whole, find the output edge that is sequential */
    if (triangulate == WHOLE)
        Whole_Output(e2,e3,index,size,&output3,&output4);

    /*  No input edge */
    if ((e2 == 0) && (e3 == 0))
        return ties_array[0];
    
    for (x = 0; x < last ; x++)
    {
        Get_EdgeEx(&output1,&output2,index,ties_array[x],size,0,0);
        /*  Partial that can be removed in just one triangle */
        if (((output1 == e3) || (output2 == e3)) && (triangulate == PARTIAL))
            saved1 = ties_array[x];
        /*  Partial removed in more than one triangle */
        if ((output1 != e3) && (output1 != e2) && (output2 != e3) && (output2 != e2) &&
            (triangulate == PARTIAL) && (saved2 != -1))
            saved2 = ties_array[x];
        /*  Whole is not so easy, since the whole polygon must be done. Given
            an input edge there is only one way to come out, approximately half
            way around the polygon.
        */
        if (((output1 == output3) && (output2 == output4)) ||
            ((output1 == output4) && (output2 == output3)) &&
            (triangulate == WHOLE))
            return ties_array[x];
    }
    
    if (saved1 != -1)
        return saved1;
    if (saved2 != -1)
        return saved2;
    
    /*  There was not a tie that was sequential */
    return Look_Ahead_Tie();
}

int Sequential_Tie(int face_id, int triangulate)
{
    /*  Break the tie by choosing the face that will
        not give us a swap and is sequential. If there
        is not one, then do the lookahead to break the
        tie.
    */
    /*  Separate into 3 cases for simplicity, if the current
        polygon has 3 sides, 4 sides or if the sides were 
        greater. We can do the smaller cases faster, so that
        is why I separated the cases.
    */

     ListHead *pListFace;
	PF_FACES face;

    /*	Get the polygon with id face_id */
	pListFace  = PolFaces[face_id];
	face = (PF_FACES) PeekList(pListFace,LISTHEAD,0);

     if (face->nPolSize == 3)
        return(Sequential_Tri(face->pPolygon));
     if (face->nPolSize == 4)
        return(Sequential_Quad(face->pPolygon,triangulate));
     else
        return(Sequential_Poly(face->nPolSize,face->pPolygon,triangulate));

}

int Get_Next_Face(int t, int face_id, int triangulate)
{
	/*	Get the next face depending on what
		the user specified
	*/
	
	/*	Did not have a tie, don't do anything */
	if (last == 1)
		return(ties_array[0]);
	if (t == RANDOM)
		return Random_Tie();
	if (t == ALTERNATE)
		return Alternate_Tie();
	if (t == LOOK)
		return Look_Ahead_Tie();
     if (t == SEQUENTIAL)
        return Sequential_Tie(face_id,triangulate);

	printf("Illegal option specified for ties, using first \n");
     return (ties_array[0]);
}
