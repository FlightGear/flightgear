#include "Math.hpp"
#include "Integrator.hpp"
namespace yasim {

void Integrator::setBody(RigidBody* body)
{
    _body = body;
}

void Integrator::setEnvironment(BodyEnvironment* env)
{
    _env = env;
}

void Integrator::setInterval(float dt)
{
    _dt = dt;
}

float Integrator::getInterval()
{
    return _dt;
}

void Integrator::setState(State* s)
{
    _s = *s;
}

State* Integrator::getState()
{
    return &_s;
}

// Transforms a "local" vector to a "global" vector (not coordinate!)
// using the specified orientation.
void Integrator::l2gVector(float* orient, float* v, float* out)
{
    Math::tmul33(orient, v, out);
}

// Updates a position vector for a body c.g. motion with velocity v
// over time dt, from orientation o0 to o1.  Because the position
// references the local coordinate origin, but the velocity is that of
// the c.g., this gets a bit complicated.
void Integrator::extrapolatePosition(double* pos, float* v, float dt,
                                     float* o1, float* o2)
{
    // Remember that it's the c.g. that's moving, so account for
    // changes in orientation.  The motion of the coordinate
    // frame will be l2gOLD(cg) + deltaCG - l2gNEW(cg)
    float cg[3], tmp[3];

    _body->getCG(cg);
    l2gVector(o1, cg, cg);    // cg = l2gOLD(cg) ("cg0")
    Math::set3(v, tmp);       // tmp = vel
    Math::mul3(dt, tmp, tmp); //     = vel*dt ("deltaCG")
    Math::add3(cg, tmp, tmp); //     = cg0 + deltaCG
    _body->getCG(cg);
    l2gVector(o2, cg, cg);    // cg = l2gNEW(cg) ("cg1")
    Math::sub3(tmp, cg, tmp); // tmp = cg0 + deltaCG - cg1

    pos[0] += tmp[0];         // p1 = p0 + (cg0+deltaCG-cg1) 
    pos[1] += tmp[1];         //  (positions are doubles, so we
    pos[2] += tmp[2];         //   can't use Math::add3)    
}

#if 0
// A straight euler integration, for reference.  Don't use.
void Integrator::calcNewInterval()
{
    float tmp[3];
    State s = _s;

    float dt = _dt / 4;

    orthonormalize(_s.orient);
    int i;
    for(i=0; i<4; i++) {
	_body->reset();
	_env->calcForces(&s);
	
	_body->getAccel(s.acc);
 	l2gVector(_s.orient, s.acc, s.acc);

	_body->getAngularAccel(s.racc);
 	l2gVector(_s.orient, s.racc, s.racc);

	float rotmat[9];
	rotMatrix(s.rot, dt, rotmat);
	Math::mmul33(_s.orient, rotmat, s.orient);
	
	extrapolatePosition(s.pos, s.v, dt, _s.orient, s.orient);
	
	Math::mul3(dt, s.acc, tmp);
	Math::add3(tmp, s.v, s.v);
	
	Math::mul3(dt, s.racc, tmp);
	Math::add3(tmp, s.rot, s.rot);

	_s = s;
    }

    _env->newState(&_s);
}
#endif

void Integrator::calcNewInterval()
{
    // In principle, these could be changed for something other than
    // a 4th order integration.  I doubt if anyone cares.
    const static int NITER=4;
    static float TIMESTEP[] = { 1.0, 0.5, 0.5, 1.0 };
    static float WEIGHTS[]  = { 6.0, 3.0, 3.0, 6.0 };

    // Scratch pads:
    double pos[NITER][3]; float vel[NITER][3]; float acc[NITER][3];
    float  ori[NITER][9]; float rot[NITER][3]; float rac[NITER][3];

    float *currVel = _s.v;
    float *currAcc = _s.acc;
    float *currRot = _s.rot;
    float *currRac = _s.racc;
    
    // First off, sanify the initial orientation
    orthonormalize(_s.orient);

    int i;
    for(i=0; i<NITER; i++) {
	//
	// extrapolate forward based on current values of the
	// derivatives and the ORIGINAL values of the
	// position/orientation.
	//
	float dt = _dt * TIMESTEP[i];
	float tmp[3];

	// "add" rotation to orientation (generate a rotation matrix)
	float rotmat[9];
	rotMatrix(currRot, dt, rotmat);
	Math::mmul33(_s.orient, rotmat, ori[i]);

	// add velocity to (original!) position
	int j;
	for(j=0; j<3; j++) pos[i][j] = _s.pos[j];
        extrapolatePosition(pos[i], currVel, dt, _s.orient, ori[i]);

	// add acceleration to (original!) velocity
	Math::set3(currAcc, tmp); 
	Math::mul3(dt, tmp, tmp);
	Math::add3(_s.v, tmp, vel[i]);

	// add rotational acceleration to rotation
	Math::set3(currRac, tmp); 
	Math::mul3(dt, tmp, tmp);
	Math::add3(_s.rot, tmp, rot[i]);

	//
	// Tell the environment to generate new forces on the body,
	// extract the accelerations, and convert to vectors in the
	// global frame.
	//
        _body->reset();

        // FIXME.  Copying into a state object is clumsy!  The
        // per-coordinate arrays should be changed into a single array
        // of State objects.  Ick.
        State stmp;
        for(j=0; j<3; j++) {
            stmp.pos[j] = pos[i][j];
            stmp.v[j] = vel[i][j];
            stmp.rot[j] = rot[i][j];
        }
        for(j=0; j<9; j++)
            stmp.orient[j] = ori[i][j];
	_env->calcForces(&stmp);

	_body->getAccel(acc[i]);
	_body->getAngularAccel(rac[i]);
 	l2gVector(_s.orient, acc[i], acc[i]);
 	l2gVector(_s.orient, rac[i], rac[i]);

	//
	// Save the resulting derivatives for the next iteration
	// 
	currVel = vel[i]; currAcc = acc[i];
	currRot = rot[i]; currRac = rac[i];
    }

    // Average the resulting derivatives together according to their
    // weights.  Yes, we're "averaging" rotations, which isn't
    // stricly correct -- rotations live in a non-cartesian space.
    // But the space is "locally" cartesian.
    State derivs;
    float tot = 0;
    for(i=0; i<NITER; i++) {
	float wgt = WEIGHTS[i];
        tot += wgt;
        int j;
        for(j=0; j<3; j++) {
            derivs.v[j]   += wgt*vel[i][j];  derivs.rot[j]    += wgt*rot[i][j];
            derivs.acc[j] += wgt*acc[i][j];  derivs.racc[j] += wgt*rac[i][j];
        }
    }
    float itot = 1/tot;
    for(i=0; i<3; i++) {
        derivs.v[i]   *= itot;  derivs.rot[i]    *= itot;
        derivs.acc[i] *= itot;  derivs.racc[i] *= itot;
    }

    // And finally extrapolate once more, using the averaged
    // derivatives, to the final position and orientation.  This code
    // is essentially identical to the position extrapolation step
    // inside the loop.

    // save the starting orientation
    float orient0[9];
    for(i=0; i<9; i++) orient0[i] = _s.orient[i];

    float rotmat[9];
    rotMatrix(derivs.rot, _dt, rotmat);
    Math::mmul33(orient0, rotmat, _s.orient);

    extrapolatePosition(_s.pos, derivs.v, _dt, orient0, _s.orient);

    float tmp[3];
    Math::mul3(_dt, derivs.acc, tmp);
    Math::add3(_s.v, tmp, _s.v);

    Math::mul3(_dt, derivs.racc, tmp);
    Math::add3(_s.rot, tmp, _s.rot);
    
    for(i=0; i<3; i++) {
	_s.acc[i] = derivs.acc[i];
	_s.racc[i] = derivs.racc[i];
    }
    
    // Tell the environment about our decision
    _env->newState(&_s);
}

// Generates a matrix that rotates about axis r through an angle equal
// to (|r| * dt).  That is, a rotation effected by rotating with rate
// r for dt "seconds" (or whatever time unit is in use).
// Implementation shamelessly cribbed from the OpenGL specification.
//
// NOTE: we're actually returning the _transpose_ of the rotation
// matrix!  This is becuase we store orientations as global-to-local
// transformations.  Thus, we want to rotate the ROWS of the old
// matrix to get the new one.
void Integrator::rotMatrix(float* r, float dt, float* out)
{
    // Normalize the vector, and extract the angle through which we'll
    // rotate.
    float mag = Math::mag3(r);
    float angle = dt*mag;

    // Tiny rotations result in degenerate (zero-length) rotation
    // vectors, so clamp to an identity matrix.  1e-06 radians
    // per 1/30th of a second (typical dt unit) corresponds to one
    // revolution per 2.4 days, or significantly less than the
    // coriolis rotation.  And it's still preserves half the floating
    // point precision of a radian-per-iteration rotation.
    if(angle < 1e-06) {
        out[0] = 1; out[1] = 0; out[2] = 0; 
        out[3] = 0; out[4] = 1; out[5] = 0; 
        out[6] = 0; out[7] = 0; out[8] = 1; 
        return;
    }

    float runit[3];
    Math::mul3(1/mag, r, runit);

    float s    = Math::sin(angle);
    float c    = Math::cos(angle);
    float c1   = 1-c;
    float c1rx = c1*runit[0];
    float c1ry = c1*runit[1];
    float xx   = c1rx*runit[0];
    float xy   = c1rx*runit[1];
    float xz   = c1rx*runit[2];
    float yy   = c1ry*runit[1];
    float yz   = c1ry*runit[2];
    float zz   = c1*runit[2]*runit[2];
    float xs   = runit[0]*s;
    float ys   = runit[1]*s;
    float zs   = runit[2]*s;

    out[0] = xx+c ; out[3] = xy-zs; out[6] = xz+ys;
    out[1] = xy+zs; out[4] = yy+c ; out[7] = yz-xs;
    out[2] = xz-ys; out[5] = yz+xs; out[8] = zz+c ;
}

// Does a Gram-Schmidt orthonormalization on the rows of a
// global-to-local orientation matrix.  The order of normalization
// here is x, z, y.  This is because of the convention of "x" being
// the forward vector and "z" being up in the body frame.  These two
// vectors are the most important to keep correct.
void Integrator::orthonormalize(float* m)
{
    // The 1st, 2nd and 3rd rows of the matrix store the global frame
    // equivalents of the x, y, and z unit vectors in the local frame.
    float *x = m, *y = m+3, *z = m+6;

    Math::unit3(x, x);                  // x = x/|x|

    float v[3];
    Math::mul3(Math::dot3(x, z), z, v); // v = z(x dot z)
    Math::sub3(z, v, z);                // z = z - z*(x dot z)
    Math::unit3(z, z);                  // z = z/|z|

    Math::cross3(z, x, y);              // y = z cross x
}

}; // namespace yasim
