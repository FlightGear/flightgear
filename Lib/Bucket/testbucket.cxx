// test new bucket routines

#include "newbucket.cxx"

main() {
    double lat = 21.9625;
    double lon = -110.0 + 0.0625;

    /*
    while ( lon < 180 ) {
	FGBucket b1( lon, lat );
	long int index = b1.gen_index();
	FGBucket b2( index );

	cout << lon << "," << lat << "  ";
	cout << b2 << " " << b2.get_center_lon() << "," 
	     << b2.get_center_lat() << endl;

	lon += 0.125;
    }
    */

    FGBucket b1;

    for ( int j = 2; j >= -2; j-- ) {
	for ( int i = -2; i < 3; i++ ) {
	    b1 = fgBucketOffset(lon, lat, i, j);
	    cout << "(" << i << "," << j << ")" << b1 << "\t";
	}
	cout << endl;
    }
}
