#include <Bucket/newbucket.hxx>

#include "array.hxx"

main(int argc, char **argv) {
    char tile_name[256];
    double lon, lat;

    if ( argc != 2 ) {
	cout << "Usage: " << argv[0] << " work_dir" << endl;
	exit(-1);
    }

    string work_dir = argv[1];

   
    lon = -146.248360; lat = 61.133950;  // PAVD (Valdez, AK)
    lon = -110.664244; lat = 33.352890;  // P13

    FGBucket b( lon, lat );
    string base = b.gen_base_path();
    string path = work_dir + "/Scenery/" + base;

    long int b_index = b.gen_index();
    sprintf(tile_name, "%ld", b_index);
    string arrayfile = path + "/" + tile_name + ".dem";
    cout << "arrayfile = " << arrayfile << endl;
    
    FGArray a(arrayfile);
    a.parse();

    lon *= 3600;
    lat *= 3600;
    cout << a.interpolate_altitude(lon, lat) << endl;

    a.fit(b, 100);
}
