#include "Math.hpp"
#include "Glue.hpp"
namespace yasim {

void Glue::calcAlphaBeta(State* s, float* wind, float* alpha, float* beta)
{
    // Convert the velocity to the aircraft frame.
    float v[3];
    Math::sub3(s->v, wind, v);
    Math::vmul33(s->orient, v, v);

    // By convention, positive alpha is an up pitch, and a positive
    // beta is yawed to the right.
    *alpha = -Math::atan2(v[2], v[0]);
    *beta = Math::atan2(v[1], v[0]);
}

void Glue::calcEulerRates(State* s, float* roll, float* pitch, float* hdg)
{
    // This one is easy, the projection of the rotation vector around
    // the "up" axis.
    float up[3];
    geodUp(s->pos, up);
    *hdg = -Math::dot3(up, s->rot); // negate for "NED" conventions

    // A bit harder: the X component of the rotation vector expressed
    // in airframe coordinates.
    float lr[3];
    Math::vmul33(s->orient, s->rot, lr);
    *roll = lr[0];

    // Hardest: the component of rotation along the direction formed
    // by the cross product of (and thus perpendicular to) the
    // aircraft's forward vector (i.e. the first three elements of the
    // orientation matrix) and the "up" axis.
    float pitchAxis[3];
    Math::cross3(s->orient, up, pitchAxis);
    Math::unit3(pitchAxis, pitchAxis);
    *pitch = Math::dot3(pitchAxis, s->rot);
}

void Glue::xyz2nedMat(double lat, double lon, float* out)
{
    // Shorthand for our output vectors:
    float *north = out, *east = out+3, *down = out+6;

    float slat = (float) Math::sin(lat);
    float clat = (float)Math::cos(lat);
    float slon = (float)Math::sin(lon);
    float clon = (float)Math::cos(lon);

    north[0] = -clon * slat;
    north[1] = -slon * slat;
    north[2] = clat;

    east[0] = -slon;
    east[1] = clon;
    east[2] = 0;

    down[0] = -clon * clat;
    down[1] = -slon * clat;
    down[2] = -slat;
}

void Glue::euler2orient(float roll, float pitch, float hdg, float* out)
{
    // To translate a point in aircraft space to the output "NED"
    // frame, first negate the Y and Z axes (ugh), then roll around
    // the X axis, then pitch around Y, then point to the correct
    // heading about Z.  Expressed as a matrix multiplication, then,
    // the transformation from aircraft to local is HPRN.  And our
    // desired output is the inverse (i.e. transpose) of that.  Since
    // all rotations are 2D, they have a simpler form than a generic
    // rotation and are done out longhand below for efficiency.

    // Init to the identity matrix
    int i, j;
    for(i=0; i<3; i++)
        for(j=0; j<3; j++)
            out[3*i+j] = (i==j) ? 1.0f : 0.0f;

    // Negate Y and Z
    out[4] = out[8] = -1;
    
    float s = Math::sin(roll);
    float c = Math::cos(roll);
    int col;
    for(col=0; col<3; col++) {
	float y=out[col+3], z=out[col+6];
	out[col+3] = c*y - s*z;
	out[col+6] = s*y + c*z;
    }

    s = Math::sin(pitch);
    c = Math::cos(pitch);
    for(col=0; col<3; col++) {
	float x=out[col], z=out[col+6];
	out[col]   = c*x + s*z;
	out[col+6] = c*z - s*x;
    }

    s = Math::sin(hdg);
    c = Math::cos(hdg);
    for(col=0; col<3; col++) {
	float x=out[col], y=out[col+3];
	out[col]   = c*x - s*y;
	out[col+3] = s*x + c*y;
    }

    // Invert:
    Math::trans33(out, out);
}

void Glue::orient2euler(float* o, float* roll, float* pitch, float* hdg)
{
    // The airplane's "pointing" direction in NED coordinates is vx,
    // and it's y (left-right) axis is vy.
    float vx[3], vy[3];
    vx[0]=o[0], vx[1]=o[1], vx[2]=o[2];
    vy[0]=o[3], vy[1]=o[4], vy[2]=o[5];

    // The heading is simply the rotation of the projection onto the
    // XY plane
    *hdg = Math::atan2(vx[1], vx[0]);

    // The pitch is the angle between the XY plane and vx, remember
    // that rotations toward positive Z are _negative_
    float projmag = Math::sqrt(vx[0]*vx[0]+vx[1]*vx[1]);
    *pitch = -Math::atan2(vx[2], projmag);

    // Roll is a bit harder.  Construct an "unrolled" orientation,
    // where the X axis is the same as the "rolled" one, and the Y
    // axis is parallel to the XY plane.  These two can give you an
    // "unrolled" Z axis as their cross product.  Now, take the "vy"
    // axis, which points out the left wing.  The projections of this
    // along the "unrolled" YZ plane will give you the roll angle via
    // atan().
    float* ux = vx;
    float  uy[3], uz[3];

    uz[0] = 0; uz[1] = 0; uz[2] = 1;
    Math::cross3(uz, ux, uy);
    Math::unit3(uy, uy);
    Math::cross3(ux, uy, uz);

    float py = -Math::dot3(vy, uy);
    float pz = -Math::dot3(vy, uz);
    *roll = Math::atan2(pz, py);
}

void Glue::geodUp(double lat, double lon, float* up)
{
    double coslat = Math::cos(lat);
    up[0] = (float)(Math::cos(lon) * coslat);
    up[1] = (float)(Math::sin(lon) * coslat);
    up[2] = (float)(Math::sin(lat));
}

// FIXME: Hardcoded WGS84 numbers...
void Glue::geodUp(double* pos, float* up)
{
    const double SQUASH  = 0.9966471893352525192801545;
    const double STRETCH = 1.0033640898209764189003079;
    float x = (float)(pos[0] * SQUASH);
    float y = (float)(pos[1] * SQUASH);
    float z = (float)(pos[2] * STRETCH);
    float norm = 1/Math::sqrt(x*x + y*y + z*z);
    up[0] = x * norm;
    up[1] = y * norm;
    up[2] = z * norm;
}

}; // namespace yasim

