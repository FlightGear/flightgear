// genfans.cxx -- Combine individual triangles into more optimal fans.
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


#include "genfans.hxx"


// make sure the list is expanded at least to hold "n" and then push
// "i" onto the back of the "n" list.
void FGGenFans::add_and_expand( reverse_list& by_node, int n, int i ) {
    int_list empty;

    int size = (int)by_node.size();
    if ( size > n ) {
	// ok
    } else {
	// cout << "capacity = " << by_node.capacity() << endl;
	// cout << "size = " << size << "  n = " << n
	//      << " need to push = " << n - size + 1 << endl;
	for ( int i = 0; i < n - size + 1; ++i ) {
	    by_node.push_back(empty);
	}
    }

    by_node[n].push_back(i);
}


// given an input triangle, shuffle nodes so that "center" is the
// first node, but maintain winding order.
static FGTriEle canonify( const FGTriEle& t, int center ) {
    if ( t.get_n1() == center ) {
	// already ok
	return t;
    } else if ( t.get_n2() == center ) {
	return FGTriEle( t.get_n2(), t.get_n3(), t.get_n1(), 0.0 );
    } else if ( t.get_n3() == center ) {
	return FGTriEle( t.get_n3(), t.get_n1(), t.get_n2(), 0.0 );
    } else {
	cout << "ERROR, index doesn't refer to this triangle!!!" << endl;
	exit(-1);
    }
}

// returns a list of triangle indices
static int_list make_best_fan( const triele_list& master_tris, 
			       const int center, const int_list& local_tris )
{
    int_list best_result;

    // try starting with each of local_tris to find the best fan
    // arrangement
    for ( int start = 0; start < (int)local_tris.size(); ++start ) {
	// cout << "trying with first triangle = " << local_tris[start] << endl;

	int_list tmp_result;
	tmp_result.clear();

	FGTriEle current_tri;
	FGTriEle test;
	current_tri = canonify( master_tris[local_tris[start]], center );
	tmp_result.push_back( local_tris[start] );

	// follow the ring
	int next = -1;
	bool matches = true;
	while ( (next != start) && matches ) {
	    // find next triangle in ring
	    matches = false;
	    for ( int i = 0; i < (int)local_tris.size(); ++i ) {
		test = canonify( master_tris[local_tris[i]], center );
		if ( current_tri.get_n3() == test.get_n2() ) {
		    if ( i != start ) {
			// cout << " next triangle = " << local_tris[i] << endl;
			current_tri = test;
			tmp_result.push_back( local_tris[i] );
			matches = true;
			next = i;
			break;
		    }
		}
	    }
	}

	if ( tmp_result.size() == local_tris.size() ) {
	    // we found a complete usage, no need to go on
	    // cout << "we found a complete usage, no need to go on" << endl;
	    best_result = tmp_result;
	    break;
	} else if ( tmp_result.size() > best_result.size() ) {
	    // we found a better way to fan
	    // cout << "we found a better fan arrangement" << endl;
	    best_result = tmp_result;
	}
    }

    return best_result;
}


static bool in_fan(int index, const int_list& fan ) {
    const_int_list_iterator current = fan.begin();
    const_int_list_iterator last = fan.end();

    for ( ; current != last; ++current ) {
	if ( index == *current ) {
	    return true;
	}
    }

    return false;
}


// recursive build fans from triangle list
fan_list FGGenFans::greedy_build( triele_list tris ) {
    cout << "starting greedy build of fans" << endl;

    fans.clear();

    while ( ! tris.empty() ) {
	// cout << "building reverse_list" << endl;
	reverse_list by_node;
	by_node.clear();

	// traverse the triangle list and for each node, build a list of
	// triangles that attach to it.

	for ( int i = 0; i < (int)tris.size(); ++i ) {
	    int n1 = tris[i].get_n1();
	    int n2 = tris[i].get_n2();
	    int n3 = tris[i].get_n3();

	    add_and_expand( by_node, n1, i );
	    add_and_expand( by_node, n2, i );
	    add_and_expand( by_node, n3, i );
	}

	// find the node in the tris list that attaches to the most
	// triangles

	// cout << "find most connected node" << endl;

	int_list biggest_group;
	reverse_list_iterator r_current = by_node.begin();
	reverse_list_iterator r_last = by_node.end();
	int index = 0;
	int counter = 0;
	for ( ; r_current != r_last; ++r_current ) {
	    if ( r_current->size() > biggest_group.size() ) {
		biggest_group = *r_current;
		index = counter;
	    }
	    ++counter;
	}
	// cout << "triangle pool = " << tris.size() << endl;
	// cout << "biggest_group = " << biggest_group.size() << endl;
	// cout << "center node = " << index << endl;

	// make the best fan we can out of this group
	// cout << "before make_best_fan()" << endl;
	int_list best_fan = make_best_fan( tris, index, biggest_group );
	// cout << "after make_best_fan()" << endl;

	// generate point form of best_fan
	int_list node_list;
	node_list.clear();

	int_list_iterator i_start = best_fan.begin();
	int_list_iterator i_current = i_start;
	int_list_iterator i_last = best_fan.end();
	for ( ; i_current != i_last; ++i_current ) {
	    FGTriEle t = canonify( tris[*i_current], index );
	    if ( i_start == i_current ) {
		node_list.push_back( t.get_n1() );
		node_list.push_back( t.get_n2() );
	    }
	    node_list.push_back( t.get_n3() );
	}
	// cout << "best list size = " << node_list.size() << endl;

	// add this fan to the fan list
	fans.push_back( node_list );

	// delete the triangles in best_fan out of tris and repeat
	triele_list_iterator t_current = tris.begin();
	triele_list_iterator t_last = tris.end();
	counter = 0;
	while ( t_current != t_last ) {
	    if ( in_fan(counter, best_fan) ) {
		// cout << "erasing " 
		//      << t_current->get_n1() << ","
		//      << t_current->get_n2() << ","
		//      << t_current->get_n3()
		//      << " from master tri pool"
		//      << endl;
		tris.erase( t_current );
	    } else {
		++t_current;
	    }
	    ++counter;
	}
    }

    cout << "end of greedy build of fans" << endl;
    cout << "average fan size = " << ave_size() << endl;

    return fans;
}


// report average fan size
double FGGenFans::ave_size() {
    double sum = 0.0;

    fan_list_iterator current = fans.begin();
    fan_list_iterator last = fans.end();
    for ( ; current != last; ++current ) {
	sum += current->size();
    }

    return sum / (double)fans.size();
}


