// triangle.cxx -- "Triangle" interface class
//
// Written by Curtis Olson, started March 1999.
//
// Copyright (C) 1999  Curtis L. Olson  - curt@flightgear.org
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$
// (Log is kept at end of this file)


#include "triangle.hxx"
#include "tripoly.hxx"

// Constructor
FGTriangle::FGTriangle( void ) {
}


// Destructor
FGTriangle::~FGTriangle( void ) {
}


// populate this class based on the specified gpc_polys list
int 
FGTriangle::build( const fitnode_list& fit_list, 
		   const FGgpcPolyList& gpc_polys )
{
    FGTriPoly poly;
    int index;

    // Point3D junkp;
    // int junkc = 0;
    // char junkn[256];
    // FILE *junkfp;

    // traverse the dem fit list and gpc_polys building a unified node
    // list and converting the polygons so that they reference the
    // node list by index (starting at zero) rather than listing the
    // points explicitely

    const_fitnode_list_iterator f_current, f_last;
    f_current = fit_list.begin();
    f_last = fit_list.end();
    for ( ; f_current != f_last; ++f_current ) {
	index = trinodes.unique_add( *f_current );
    }

    gpc_polygon *gpc_poly;
    const_gpcpoly_iterator current, last;

    // process polygons in priority order
    cout << "prepairing node list and polygons" << endl;

    for ( int i = 0; i < FG_MAX_AREA_TYPES; ++i ) {
	cout << "area type = " << i << endl;
	current = gpc_polys.polys[i].begin();
	last = gpc_polys.polys[i].end();
	for ( ; current != last; ++current ) {
	    gpc_poly = *current;
	    cout << "processing a polygon, contours = " 
		 << gpc_poly->num_contours << endl;

	    if (gpc_poly->num_contours <= 0 ) {
		cout << "FATAL ERROR! no contours in this polygon" << endl;
		exit(-1);
	    }

	    if (gpc_poly->num_contours > 1 ) {
		cout << "FATAL ERROR! no multi-contour support" << endl;
		sleep(2);
		// exit(-1);
	    }

	    for ( int j = 0; j < gpc_poly->num_contours; j++ ) {

		poly.erase();

		// sprintf(junkn, "g.%d", junkc++);
		// junkfp = fopen(junkn, "w");

		for ( int k = 0; k < gpc_poly->contour[j].num_vertices; k++ ) {
		    Point3D p( gpc_poly->contour[j].vertex[k].x,
			       gpc_poly->contour[j].vertex[k].y,
			       0 );
		    index = trinodes.unique_add( p );
		    // junkp = trinodes.get_node( index );
		    // fprintf(junkfp, "%.4f %.4f\n", junkp.x(), junkp.y());
		    poly.add_node(index);
		    // cout << index << endl;
		}
		// fprintf(junkfp, "%.4f %.4f\n", 
		//    gpc_poly->contour[j].vertex[0].x, 
		//    gpc_poly->contour[j].vertex[0].y);
		// fclose(junkfp);

		poly.calc_point_inside( trinodes );

		polylist[i].push_back(poly);
	    }
	}
    }

    for ( int i = 0; i < FG_MAX_AREA_TYPES; ++i ) {
	if ( polylist[i].size() ) {
	    cout << get_area_name((AreaType)i) << " = " 
		 << polylist[i].size() << endl;
	}
    }

    // traverse the polygon lists and build the segment (edge) list
    // that is used by the "Triangle" lib.

    int i1, i2;
    for ( int i = 0; i < FG_MAX_AREA_TYPES; ++i ) {
	cout << "area type = " << i << endl;
	tripoly_list_iterator tp_current, tp_last;
	tp_current = polylist[i].begin();
	tp_last = polylist[i].end();

	// process each polygon in list
	for ( ; tp_current != tp_last; ++tp_current ) {
	    poly = *tp_current;

	    for ( int j = 0; j < (int)(poly.size()) - 1; ++j ) {
		i1 = poly.get_pt_index( j );
		i2 = poly.get_pt_index( j + 1 );
		trisegs.unique_add( FGTriSeg(i1, i2) );
	    }
	    i1 = poly.get_pt_index( 0 );
	    i2 = poly.get_pt_index( poly.size() - 1 );
	    trisegs.unique_add( FGTriSeg(i1, i2) );
	}
    }

    return 0;
}


static void write_out_data(struct triangulateio *out) {
    FILE *node = fopen("tile.node", "w");
    fprintf(node, "%d 2 %d 0\n", 
	    out->numberofpoints, out->numberofpointattributes);
    for (int i = 0; i < out->numberofpoints; i++) {
	fprintf(node, "%d %.6f %.6f %.2f\n", 
		i, out->pointlist[2*i], out->pointlist[2*i + 1], 0.0);
    }
    fclose(node);

    FILE *ele = fopen("tile.ele", "w");
    fprintf(ele, "%d 3 0\n", out->numberoftriangles);
    for (int i = 0; i < out->numberoftriangles; i++) {
        fprintf(ele, "%d ", i);
        for (int j = 0; j < out->numberofcorners; j++) {
	    fprintf(ele, "%d ", out->trianglelist[i * out->numberofcorners + j]);
        }
        for (int j = 0; j < out->numberoftriangleattributes; j++) {
	    fprintf(ele, "%.6f ", 
		    out->triangleattributelist[i 
					      * out->numberoftriangleattributes
					      + j]
		    );
        }
	fprintf(ele, "\n");
    }
    fclose(ele);

    FILE *fp = fopen("tile.poly", "w");
    fprintf(fp, "0 2 1 0\n");
    fprintf(fp, "%d 0\n", out->numberofsegments);
    for (int i = 0; i < out->numberofsegments; ++i) {
	fprintf(fp, "%d %d %d\n", 
		i, out->segmentlist[2*i], out->segmentlist[2*i + 1]);
    }
    fprintf(fp, "%d\n", out->numberofholes);
    for (int i = 0; i < out->numberofholes; i++) {
	fprintf(fp, "%d %.6f %.6f\n", 
		i, out->holelist[2*i], out->holelist[2*i + 1]);
    }
    fprintf(fp, "%d\n", out->numberofregions);
    for (int i = 0; i < out->numberofregions; i++) {
	fprintf(fp, "%d %.6f %.6f %.6f\n", 
		i, out->regionlist[4*i], out->regionlist[4*i + 1],
		out->regionlist[4*i + 2]);
    }
}


// triangulate each of the polygon areas
int FGTriangle::run_triangulate() {
    FGTriPoly poly;
    Point3D p;
    struct triangulateio in, out, vorout;
    int counter;

    // point list
    trinode_list node_list = trinodes.get_node_list();
    in.numberofpoints = node_list.size();
    in.pointlist = (REAL *) malloc(in.numberofpoints * 2 * sizeof(REAL));

    trinode_list_iterator tn_current, tn_last;
    tn_current = node_list.begin();
    tn_last = node_list.end();
    counter = 0;
    for ( ; tn_current != tn_last; ++tn_current ) {
	in.pointlist[counter++] = tn_current->x();
	in.pointlist[counter++] = tn_current->y();
    }

    in.numberofpointattributes = 1;
    in.pointattributelist = (REAL *) malloc(in.numberofpoints *
					    in.numberofpointattributes *
					    sizeof(REAL));
    for ( int i = 0; i < in.numberofpoints * in.numberofpointattributes; i++) {
	in.pointattributelist[i] = 0.0;
    }

    in.pointmarkerlist = (int *) malloc(in.numberofpoints * sizeof(int));
    for ( int i = 0; i < in.numberofpoints; i++) {
	in.pointmarkerlist[i] = 0;
    }

    // triangle list
    in.numberoftriangles = 0;

    // segment list
    triseg_list seg_list = trisegs.get_seg_list();
    in.numberofsegments = seg_list.size();
    in.segmentlist = (int *) malloc(in.numberofsegments * 2 * sizeof(int));

    triseg_list_iterator s_current, s_last;
    s_current = seg_list.begin();
    s_last = seg_list.end();
    counter = 0;
    for ( ; s_current != s_last; ++s_current ) {
	in.segmentlist[counter++] = s_current->get_n1();
	in.segmentlist[counter++] = s_current->get_n2();
    }

    // hole list (make holes for airport ignore areas)
    in.numberofholes = polylist[(int)AirportIgnoreArea].size();
    in.holelist = (REAL *) malloc(in.numberofholes * 2 * sizeof(REAL));

    tripoly_list_iterator h_current, h_last;
    h_current = polylist[(int)AirportIgnoreArea].begin();
    h_last = polylist[(int)AirportIgnoreArea].end();
    counter = 0;
    for ( ; h_current != h_last; ++h_current ) {
	poly = *h_current;
	p = poly.get_point_inside();
	in.holelist[counter++] = p.x();
	in.holelist[counter++] = p.y();
    }

    // region list
    in.numberofregions = 0;
    for ( int i = 0; i < FG_MAX_AREA_TYPES; ++i ) {
	in.numberofregions += polylist[i].size();
    }

    in.regionlist = (REAL *) malloc(in.numberofregions * 4 * sizeof(REAL));
    counter = 0;
    for ( int i = 0; i < FG_MAX_AREA_TYPES; ++i ) {
	tripoly_list_iterator h_current, h_last;
	h_current = polylist[(int)i].begin();
	h_last = polylist[(int)i].end();
	for ( ; h_current != h_last; ++h_current ) {
	    poly = *h_current;
	    p = poly.get_point_inside();
	    in.regionlist[counter++] = p.x();  // x coord
	    in.regionlist[counter++] = p.y();  // y coord
	    in.regionlist[counter++] = i;      // region attribute
	    in.regionlist[counter++] = -1.0;   // area constraint (unused)
	}
    }

    // prep the output structures
    out.pointlist = (REAL *) NULL;        // Not needed if -N switch used.
    // Not needed if -N switch used or number of point attributes is zero:
    out.pointattributelist = (REAL *) NULL;
    out.pointmarkerlist = (int *) NULL;   // Not needed if -N or -B switch used.
    out.trianglelist = (int *) NULL;      // Not needed if -E switch used.
    // Not needed if -E switch used or number of triangle attributes is zero:
    out.triangleattributelist = (REAL *) NULL;
    out.neighborlist = (int *) NULL;      // Needed only if -n switch used.
    // Needed only if segments are output (-p or -c) and -P not used:
    out.segmentlist = (int *) NULL;
    // Needed only if segments are output (-p or -c) and -P and -B not used:
    out.segmentmarkerlist = (int *) NULL;
    out.edgelist = (int *) NULL;          // Needed only if -e switch used.
    out.edgemarkerlist = (int *) NULL;    // Needed if -e used and -B not used.
  
    vorout.pointlist = (REAL *) NULL;     // Needed only if -v switch used.
    // Needed only if -v switch used and number of attributes is not zero:
    vorout.pointattributelist = (REAL *) NULL;
    vorout.edgelist = (int *) NULL;       // Needed only if -v switch used.
    vorout.normlist = (REAL *) NULL;      // Needed only if -v switch used.
    
    // TEMPORARY
    // write_out_data(&in);

    // Triangulate the points.  Switches are chosen to read and write
    // a PSLG (p), preserve the convex hull (c), number everything
    // from zero (z), assign a regional attribute to each element (A),
    // and produce an edge list (e), and a triangle neighbor list (n).

    triangulate("pczq15Aen", &in, &out, &vorout);

    // TEMPORARY
    write_out_data(&out);

    // free mem allocated to the "Triangle" structures
    free(in.pointlist);
    free(in.pointattributelist);
    free(in.pointmarkerlist);
    free(in.regionlist);
    free(out.pointlist);
    free(out.pointattributelist);
    free(out.pointmarkerlist);
    free(out.trianglelist);
    free(out.triangleattributelist);
    // free(out.trianglearealist);
    free(out.neighborlist);
    free(out.segmentlist);
    free(out.segmentmarkerlist);
    free(out.edgelist);
    free(out.edgemarkerlist);
    free(vorout.pointlist);
    free(vorout.pointattributelist);
    free(vorout.edgelist);
    free(vorout.normlist);

    return 0;
}


// $Log$
// Revision 1.9  1999/03/21 15:48:02  curt
// Removed Dem2node from the Tools fold.
// Tweaked the triangulator options to add quality mesh refinement.
//
// Revision 1.8  1999/03/21 14:02:06  curt
// Added a mechanism to dump out the triangle structures for viewing.
// Fixed a couple bugs in first pass at triangulation.
// - needed to explicitely initialize the polygon accumulator in triangle.cxx
//   before each polygon rather than depending on the default behavior.
// - Fixed a problem with region attribute propagation where I wasn't generating
//   the hole points correctly.
//
// Revision 1.7  1999/03/20 20:32:55  curt
// First mostly successful tile triangulation works.  There's plenty of tweaking
// to do, but we are marching in the right direction.
//
// Revision 1.6  1999/03/20 13:22:11  curt
// Added trisegs.[ch]xx tripoly.[ch]xx.
//
// Revision 1.5  1999/03/20 02:21:52  curt
// Continue shaping the code towards triangulation bliss.  Added code to
// calculate some point guaranteed to be inside a polygon.
//
// Revision 1.4  1999/03/19 22:29:04  curt
// Working on preparationsn for triangulation.
//
// Revision 1.3  1999/03/19 00:27:10  curt
// Continued work on triangulation preparation.
//
// Revision 1.2  1999/03/18 04:31:11  curt
// Let's not pass copies of huge structures on the stack ... ye might see a
// segfault ... :-)
//
// Revision 1.1  1999/03/17 23:51:59  curt
// Initial revision.
//
