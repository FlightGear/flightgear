// some simple STL tests

#include <deque>
#include <map>
#include <iostream>
#include <string>

using std::deque;
using std::map;
using std::string;
using std::less;
using std::cout;

main() {
    deque < int > mylist;
    map < string, int, less<string> > mymap;
    string name;
    int i, j, size, num, age;

    // printf("max_size = %d\n", mylist.max_size());

    for ( i = 0; i < 2000; i++ ) {
	mylist.push_back(i);
    }

    size = mylist.size();

    deque < int > :: iterator current = mylist.begin();
    deque < int > :: iterator last    = mylist.end();
    
    /*
    for ( i = 0; i < 10000000; i++ ) {
	while ( current != last ) {
	    num = *current++;
	}
    }
    */
     
    /*
    for ( i = 0; i < 1000; i++ ) {
	for ( j = 0; j < size; j++ ) {
	    num = mylist[j];
	}
    }
    */

    mymap["curt"] = 30;
    mymap["doug"] = 12;
    mymap["doug"] = 28;
    mymap["dan"] = 24;

    printf("curt age = %d\n", mymap["curt"]);
    printf("dan age = %d\n", mymap["dan"]);


    map < string, int, less<string> > :: iterator test = mymap.find("doug");
    if ( test == mymap.end() ) {
	printf("doug age = not found\n");
    } else {
	printf("doug age = %d\n",  (*test).second);
        mymap.erase( "doug" );
    }

    printf("\n");
    map < string, int, less<string> > :: iterator mapcurrent = mymap.begin();
    map < string, int, less<string> > :: iterator maplast = mymap.end();
    while ( mapcurrent != maplast ) {
	name = (*mapcurrent).first;
	age = (*mapcurrent).second;
	cout << name;
	printf(" = %d\n", age);
	*mapcurrent++;
    }
}
