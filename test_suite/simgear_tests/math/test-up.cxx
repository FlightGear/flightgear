// do some test relating to the concept of "up"

#include <iostream>

#include <simgear/math/sg_geodesy.hxx>

#include "testSGGeodesy.hxx"


using std::cout;
using std::endl;


// The test-up unit test.
void MathGeodesyTests::testUp()
{
    // for each lat/lon given in goedetic coordinates, calculate
    // geocentric coordinates, cartesian coordinates, the local "up"
    // vector (based on original geodetic lat/lon), as well as the "Z"
    // intercept (for which 0 = center of earth)


    double lon = 0;
    double alt = 0;
    int i;
    const int n = 19;
    double lat[n] = {0,  5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90};

    // Hard coded expected results.
    double lat_geoc_expected[n] = {
        0,
        4.96669608700923,
        9.93439421027913,
        14.9040671396528,
        19.8766298623747,
        24.8529125604827,
        29.8336358098291,
        34.8193887023496,
        39.8106105519284,
        44.8075767840180,
        49.8103895262907,
        54.8189733092144,
        59.8330761504926,
        64.8522761370246,
        69.8759934364236,
        74.9035074740039,
        79.9339788099657,
        84.9664750566862,
        90
    };
    double cart_expected[n][3] = {
        {6378137, 0, 0},
        {6353866.26310279, 555891.26758132, 0},
        {6281238.76737403, 1107551.86696002, 0},
        {6160807.25190988, 1650783.32787306, 0},
        {5993488.27326157, 2181451.33089075, 0},
        {5780555.22988658, 2695517.17208404, 0},
        {5523628.67081747, 3189068.5, 0},
        {5224663.96230526, 3658349.09101875, 0},
        {4885936.40630155, 4099787.43648327, 0},
        {4510023.92403682, 4510023.92403682, 0},
        {4099787.43648328, 4885936.40630155, 0},
        {3658349.09101875, 5224663.96230526, 0},
        {3189068.5, 5523628.67081747, 0},
        {2695517.17208404, 5780555.22988658, 0},
        {2181451.33089075, 5993488.27326157, 0},
        {1650783.32787306, 6160807.25190988, 0},
        {1107551.86696002, 6281238.76737403, 0},
        {555891.26758132, 6353866.26310279, 0},
        {3.90548253078665e-10, 6378137, 0}
    };
    double up_expected[n][3] = {
        {6378137, 0, -0},
        {6354027.82056206, 0, -552183.96002777},
        {6281872.82960345, 0, -1100248.54773536},
        {6162189.08803834, 0, -1640100.14019589},
        {5995836.38389635, 0, -2167696.78782876},
        {5784014.11472572, 0, -2679074.46295779},
        {5528256.63929284, 0, -3170373.73538364},
        {5230426.84020036, 0, -3637866.9093781},
        {4892707.60007269, 0, -4077985.57220038},
        {4517590.87884893, 0, -4487348.40886592},
        {4107864.09120678, 0, -4862789.03770643},
        {3666593.52237417, 0, -5201383.52320227},
        {3197104.58692395, 0, -5500477.13393864},
        {2702958.82600294, 0, -5757709.84149597},
        {2187927.64927902, 0, -5971040.00711856},
        {1655962.9523645, 0, -6138765.68235824},
        {1111164.87081001, 0, -6259542.96102869},
        {557747.059253584, 0, -6332400.86398617},
        {3.91862092481447e-10, 0, -6356752.31424518}
    };
    double geod_intercept[n] = {
        0,
        552169.92020293,
        1100137.4939038,
        1639732.35699488,
        2166847.86675177,
        2677472.35586416,
        3167719.66364641,
        3633858.71206688,
        4072341.90150465,
        4479832.11012469,
        4853228.0913901,
        5189688.07642079,
        5486651.40157058,
        5741857.99662372,
        5953365.5852939,
        6119564.46711958,
        6239189.76825642,
        6311331.06793104,
        6335439.32729282
    };
    double geoc_intercept[n] = {
        0,
        552169.92020293,
        1100137.4939038,
        1639732.35699488,
        2166847.86675177,
        2677472.35586416,
        3167719.66364641,
        3633858.71206688,
        4072341.90150465,
        4479832.11012469,
        4853228.0913901,
        5189688.07642079,
        5486651.40157058,
        5741857.99662372,
        5953365.5852939,
        6119564.46711958,
        6239189.76825642,
        6311331.06793104,
        6378137    // Sole difference with geod, due to truncation artifacts.
    };

    for (i=0; i < n; i++) {
        cout << "lon = " << lon << "  geod lat = " << lat[i];

        double sl_radius, lat_geoc;
        sgGeodToGeoc( lat[i] * SGD_DEGREES_TO_RADIANS, alt, &sl_radius, &lat_geoc );
        cout << "  geoc lat = " << lat_geoc * SGD_RADIANS_TO_DEGREES << endl;
        CPPUNIT_ASSERT_DOUBLES_EQUAL(lat_geoc_expected[i], lat_geoc*SGD_RADIANS_TO_DEGREES, 1e-10);

        double pc[3];
        sgGeodToCart( lon * SGD_DEGREES_TO_RADIANS, lat[i] * SGD_DEGREES_TO_RADIANS, 0.0, pc );
        cout << "  cartesian = " << SGVec3d(pc[0], pc[1], pc[2]) << endl;
        for (int j=0; j < 3; j++)
            CPPUNIT_ASSERT_DOUBLES_EQUAL(cart_expected[i][j], pc[j], 1e-7);

        SGGeod geod_up = SGGeod::fromDeg(lon, -lat[i]);
        SGVec3d geod_up_vect = SGVec3d::fromGeod(geod_up);
        cout << "  geod up = " << geod_up_vect << endl;
        for (int j=0; j < 3; j++)
            CPPUNIT_ASSERT_DOUBLES_EQUAL(up_expected[i][j], geod_up_vect[j], 1e-7);

        SGGeoc geoc_up = SGGeoc::fromGeod(geod_up);
        SGVec3d geoc_up_vect = SGVec3d::fromGeoc(geoc_up);
        cout << "  geoc up = " << geoc_up_vect << endl;
        for (int j=0; j < 3; j++)
            CPPUNIT_ASSERT_DOUBLES_EQUAL(up_expected[i][j], geoc_up_vect[j], 1e-7);

        double slope = geod_up_vect[2] / geod_up_vect[0];
        double intercept = pc[2] - slope * pc[0];
        cout << "  Z intercept (based on geodetic up) = " << intercept << endl;
        CPPUNIT_ASSERT_DOUBLES_EQUAL(geod_intercept[i], intercept, 1e-7);

        slope = geoc_up_vect[2] / geoc_up_vect[0];
        intercept = pc[2] - slope * pc[0];
        cout << "  Z intercept (based on geocentric up) = " << intercept << endl;
        CPPUNIT_ASSERT_DOUBLES_EQUAL(geoc_intercept[i], intercept, 1e-7);
    }
}
