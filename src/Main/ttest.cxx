// This should be ported to plib/sg before you try to compile it.  If
// you do the port please send it to me. :-)

#include <Math/mat3.h>
#include <Include/fg_constants.h>

main() {
    MAT3mat R_Phi, R_Theta, R_Psi, R_Lat, R_Lon, T_view;
    MAT3mat TMP, AIRCRAFT, WORLD, EYE_TO_WORLD, WORLD_TO_EYE;
    MAT3hvec vec, eye, vec1;

    double FG_Phi = 0.00;
    double FG_Theta = 0.00;
    double FG_Psi = 0.00;
    // double FG_Latitude = 33.3528917 * DEG_TO_RAD;
    double FG_Latitude = 0.0;
    // double FG_Longitude = -110.6642444 * DEG_TO_RAD;
    double FG_Longitude = 90.0 * DEG_TO_RAD;
    // double view_pos[] = {2936.3222, 1736.9243, 3689.5359, 1.0};
    double view_pos[] = {0.0, 0.0, 0.0, 1.0};

    // Roll Matrix
    MAT3_SET_HVEC(vec, 0.0, 0.0, -1.0, 1.0);
    MAT3rotate(R_Phi, vec, FG_Phi);
    printf("Roll matrix (Phi)\n");
    MAT3print(R_Phi, stdout);

    // Pitch Matrix
    MAT3_SET_HVEC(vec, 1.0, 0.0, 0.0, 1.0);
    MAT3rotate(R_Theta, vec, FG_Theta);
    printf("\nPitch matrix (Theta)\n");
    MAT3print(R_Theta, stdout);

    // Yaw Matrix
    MAT3_SET_HVEC(vec, 0.0, -1.0, 0.0, 1.0);
    MAT3rotate(R_Psi, vec, SGD_PI + FG_Psi);
    printf("\nYaw matrix (Psi)\n");
    MAT3print(R_Psi, stdout);

    // Latitude
    MAT3_SET_HVEC(vec, 1.0, 0.0, 0.0, 1.0);
    // R_Lat = rotate about X axis
    MAT3rotate(R_Lat, vec, FG_Latitude);
    printf("\nLatitude matrix\n");
    MAT3print(R_Lat, stdout);

    // Longitude
    MAT3_SET_HVEC(vec, 0.0, 0.0, 1.0, 1.0);
    // R_Lon = rotate about Z axis
    MAT3rotate(R_Lon, vec, FG_Longitude - SGD_PI_2 );
    printf("\nLongitude matrix\n");
    MAT3print(R_Lon, stdout);

    // View position in scenery centered coordinates
    MAT3translate(T_view, view_pos);
    printf("\nTranslation matrix\n");
    MAT3print(T_view, stdout);

    // aircraft roll/pitch/yaw
    MAT3mult(TMP, R_Phi, R_Theta);
    MAT3mult(AIRCRAFT, TMP, R_Psi);
    printf("\naircraft roll pitch yaw\n");
    MAT3print(AIRCRAFT, stdout);

    // lon/lat
    MAT3mult(WORLD, R_Lat, R_Lon);
    printf("\nworld\n");
    MAT3print(WORLD, stdout);

    MAT3mult(EYE_TO_WORLD, AIRCRAFT, WORLD);
    MAT3mult(EYE_TO_WORLD, EYE_TO_WORLD, T_view);
    printf("\nEye to world\n");
    MAT3print(EYE_TO_WORLD, stdout);

    MAT3invert(WORLD_TO_EYE, EYE_TO_WORLD);
    printf("\nWorld to eye\n");
    MAT3print(WORLD_TO_EYE, stdout);

    MAT3_SET_HVEC(eye, 0.0, 0.0, 0.0, 1.0);
    MAT3mult_vec(vec, eye, EYE_TO_WORLD);
    printf("\neye -> world = %.2f %.2f %.2f\n", vec[0], vec[1], vec[2]);

    MAT3_SET_HVEC(vec1, 0.0, 6378138.12, 0.0, 1.0);
    MAT3mult_vec(vec, vec1, EYE_TO_WORLD);
    printf( "\n+y (eye) -> +y (world) = %.2f %.2f %.2f\n", 
	    vec[0], vec[1], vec[2]);

    MAT3mult_vec(vec1, vec, WORLD_TO_EYE);
    printf( "\n+y (world) -> +y (eye) = %.2f %.2f %.2f\n", 
	    vec1[0], vec1[1], vec1[2]);
}





