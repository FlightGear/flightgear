#include "Turbulence.hpp"
#include "Math.hpp"

namespace yasim {
// Typical velocity spectrum: MIN 0.017 MAX 0.72 AVG 0.30 RMS 0.33

// Maximum conceivable turbulence flow, in m/s.  In practice, most
// generated turbulence fields top out at about 70% of this number.
const float MAX_TURBULENCE = 20;

// Rate, in "meters" per second, of the default time axis motion.
// This will be multiplied by the rate-hz property to get the actual
// time axis offset.  A setting of 2.0 causes the maximum frequency
// component to arrive at 1 Hz.
const float BASE_RATE = 2.0;

// Power to which the input magnitude (always in the range [0:1]) is
// raised to get a coefficient for the turbulence velocity.  Setting
// this to 1.0 makes the scale linear.  Increasing it makes it
// curvier, with a sharp increase at the high end of the scale.
const double MAGNITUDE_EXP = 2.0;

// How many generations are "meaningful" (i.e., not part of the normal
// wind computation).  Decreasing this number will reallocate
// bandwidth to the higher frequency components.  A turbulence field
// will swing between maximal values over a distance of approximately
// 2^(MEANINGFUL_GENS-1).
const int MEANINGFUL_GENS = 7;

static const float FT2M = 0.3048;

// 8 x 32 s-box used by hashrand.  Read out of /dev/random on my Linux
// box; not analyzed for linearity or coverage issues.
static unsigned int SBOX[] = {
    0x0a881716, 0x20daa8ee, 0x61eb7d78, 0x46164e74, 0x39ab9d9d, 0x633a33f6,
    0x437c821d, 0x60a66f29, 0xc4ae45ab, 0x9a5cb3ce, 0x4a43606a, 0x56802c3c,
    0xe40d5e25, 0xa0297f41, 0x0457671e, 0xf167ab77, 0x571276db, 0x8b644e02,
    0xd5cfc592, 0x2331bfa2, 0xf9dfe7c1, 0xce9e7583, 0xfb133c29, 0x951c31c9,
    0x8e61b24e, 0xddf37570, 0x938c3b72, 0xaf907468, 0x98b77ac7, 0xe6edd515,
    0xa01f3600, 0xeafea5ad, 0x83fcce08, 0xe2e9fa9d, 0xd87727bb, 0x1945ea4c,
    0x831d295f, 0xa796ed85, 0xaa907b24, 0x69b25f12, 0xd4b27868, 0xdcde40f5,
    0x0e9e6def, 0x348a4702, 0x298389c8, 0xce405b63, 0x2e36d5a3, 0xf0569882,
    0x3beb3219, 0xf2366b9a, 0x69576cca, 0xd2725b8b, 0x6016d6f3, 0x728142ca,
    0x448b9f47, 0xe600cd4e, 0xac45d524, 0x0e32531b, 0x425d7b55, 0xc65cd9dc,
    0x58d7f9f1, 0x19f49822, 0x6786f2d3, 0x57844748, 0x523de4a3, 0x01079655,
    0x6dccea89, 0xb59278f2, 0x13a27e83, 0x19bcfc69, 0x4cff4bf5, 0xb18a3441,
    0x1e235c5e, 0xa1b47a42, 0x3bee8a5a, 0xa0962594, 0xa9b1ce4c, 0xb00399c8,
    0x83749847, 0x42c666e7, 0x08b81e57, 0xf7eee24b, 0x66720817, 0x3983f5f8,
    0x4999a817, 0x94fabd7a, 0x7aa775ef, 0xf6c1adcb, 0x5f32a695, 0x813ecf7e,
    0x66615fd5, 0xc0012e15, 0x051dd97e, 0xe6ee2803, 0x2449663c, 0x4024d59c,
    0xcb70a774, 0xacd3db94, 0x1845484e, 0xc803ef3c, 0x0662876f, 0x8794fe30,
    0xf0f0d16a, 0x41c065b8, 0xff9d5fc7, 0xa4237394, 0x8656614d, 0x26be5da9,
    0xb32bc625, 0xf215cc58, 0xc1e21848, 0xb97fe9fc, 0xbb28ef04, 0xde88eb23,
    0xe0623033, 0xa3df9e9c, 0xe9b95887, 0x3a4ab03b, 0x1cba812e, 0x174b4b37,
    0x0074d24b, 0xe5668d09, 0xf11a070a, 0x2884252b, 0x911149ea, 0x20dab459,
    0x89573d33, 0x68c2711d, 0x2b8e9781, 0xf007567b, 0x9761c8fa, 0x574d3a4e,
    0xa2ac28dd, 0x924f2211, 0xb0a91028, 0x83a90487, 0xf22cf6f8, 0x17a5dcfe,
    0x10497534, 0x27dd1316, 0x94a34815, 0x276e11ee, 0xead1d779, 0x0bfd4f20,
    0x45f2228f, 0x35d21bf8, 0x121336c0, 0x43a6538b, 0x55e950dc, 0x88a80871,
    0xfda9f61e, 0x5c76d120, 0x2eb8338f, 0x5193bb8e, 0x30a6995c, 0x500505a8,
    0x7b214f6a, 0x6a74558d, 0x040d0716, 0x4452846b, 0xd0a0e838, 0xead282e0,
    0x6bc6465c, 0xcb4ab107, 0xab990ed7, 0x72a1fe7b, 0x06901fdf, 0x18f90739,
    0x8cd2b861, 0xaea9d40c, 0x2dcf7c18, 0x45979e8a, 0x10393f0d, 0x3209d7c9,
    0x2c71378f, 0x908a692a, 0xc0e63b24, 0x05de3118, 0xfc974436, 0x1be44823,
    0x03de2f3d, 0x66cfb6e4, 0x52727bfc, 0xa7b93651, 0xd7b9035f, 0xfac28d33,
    0x59bb4457, 0xeede4004, 0x175ad747, 0x7808d123, 0xc9c97de8, 0x0c26ca26,
    0x75e62e96, 0xc8376e97, 0xf2ee6baa, 0x6a885f88, 0x352f92ab, 0x4143f4a4,
    0xb1cca58c, 0xe8fbea94, 0x5c306621, 0xfbe64c32, 0xa1ed285d, 0xca7395cf,
    0x4eed31a5, 0x31e39fee, 0x7951c585, 0x23434811, 0xfc103036, 0xef001b3c,
    0x499f5f34, 0x5f7f38f4, 0x0206d8c5, 0xcc3ee4f1, 0xbc0b485c, 0x4e4f5829,
    0x05ee6e6d, 0xc82d5353, 0x44892bec, 0x22984b53, 0x8a6374d1, 0x0850c3f9,
    0x0c06ae88, 0x2dfdc126, 0xd1edacdc, 0x1d8dbd39, 0xdeff2db8, 0xd557278d,
    0x7e9e3740, 0x49a1ecb5, 0x43f7b391, 0x50b6b9ef, 0x46b9b8f8, 0xd3f5f6d2,
    0x8d453b88, 0xc0ba5333, 0x5ab92e37, 0x6e7620a4, 0x8eb9795a, 0x30355a84,
    0xf5e4ad33, 0x7d0b4df2, 0xe0f3e2a1, 0xa466f0e6, 0x39a19c9a, 0x1b284524,
    0x854f8b3b, 0x02d10b6c, 0x44fb5d9d, 0x60c29fec, 0xda35244a, 0xb5ce6653,
    0xfd8356ad, 0xff88d46b, 0x23fd1d16, 0xdc0be23c };

// Random hash function on 32 bit integers.  Works by XORing the input
// word with s-box values looked up from each input byte.  This is
// pretty much the simplest "good" hash function of this type.  The
// instruction count is very low; depending on cache behavior with the
// 1024 byte s-box table, it may or may not be the fastest.
inline unsigned int Turbulence::hashrand(unsigned int i)
{
    i ^= SBOX[(i>> 0) & 0xff]; 
    i ^= SBOX[(i>> 8) & 0xff]; 
    i ^= SBOX[(i>>16) & 0xff]; 
    i ^= SBOX[(i>>24) & 0xff];
    return i;
}

// 32 bit integer to [0:1] (safe with 64 bit ints)
static inline float i2fu(unsigned int i) { return (1.0/0xffffffffu) * i; }

// 32 bit integer to [-1:1] (safe with 64 bit ints)
static inline float i2fs(unsigned int i) { return 2 * i2fu(i) - 1; }

// Similar conversions, for 8 bit unsigned bytes
static inline float c2fu(unsigned char c) { return (c+0.5)*(1.0/256); }
static inline unsigned char f2cu(float f) {
    int c = (int)(f * 256);
    return c == 256 ? 255 : c;
}

inline void Turbulence::turblut(int x, int y, float* out)
{
    x = x >= _sz ? x - _sz : x;
    y = y >= _sz ? y - _sz : y;

    unsigned char* turb = _data + 3*(y*_sz+x);
    out[0] = c2fu(turb[0]) * (_x1 - _x0) + _x0;
    out[1] = c2fu(turb[1]) * (_y1 - _y0) + _y0;
    out[2] = c2fu(turb[2]) * (_z1 - _z0) + _z0;
}

void Turbulence::setMagnitude(double mag)
{
    _mag = Math::pow(mag, MAGNITUDE_EXP);
}

void Turbulence::update(double dt, double rate)
{
    _timeOff += BASE_RATE * dt * rate;
}

void Turbulence::offset(float* offset)
{
    for(int i=0; i<3; i++)
        _off[i] += offset[i];
}

void Turbulence::getTurbulence(double* loc, float alt, float* up,
                               float* turbOut)
{
    // Convert to integer 2D coordinates; wrap to [0:_sz].
    double a = (loc[0] + _off[0]) + (loc[2] + _off[2]);
    double b = (loc[1] + _off[1]) + _timeOff;
    a -= _sz * Math::floor(a * (1.0/_sz));
    b -= _sz * Math::floor(b * (1.0/_sz));
    int x = ((int)Math::floor(a))&(_sz-1);
    int y = ((int)Math::floor(b))&(_sz-1);

    // Convert to fractional interpolation factors
    a -= x;
    b -= y;

    // Do the lookups
    float turb00[3], turb10[3], turb01[3], turb11[3];
    turblut(x,     y, turb00);
    turblut(x+1,   y, turb10);
    turblut(x,   y+1, turb01);
    turblut(x+1, y+1, turb11);

    // Interpolate, add in units
    float mag = _mag * MAX_TURBULENCE;
    for(int i=0; i<3; i++) {
        float avg0 = (1-a)*turb00[i] + a*turb01[i];
        float avg1 = (1-a)*turb10[i] + a*turb11[i];
        turbOut[i] = mag * ((1-b)*avg0 + b*avg1);
    }

    // Adjust for altitude effects
    if(alt < 300) {
        float altmul = 0.5 + (1-0.5) * (alt*(1.0/300));
        if(alt < 100) {
            float vmul = alt * (1.0/100);
            vmul = vmul / altmul; // pre-correct for the pending altmul
            float dot = Math::dot3(turbOut, up);
            float off[3];
            Math::mul3(dot * (vmul-1), up, off);
            Math::add3(turbOut, off, turbOut);
        }
        Math::mul3(altmul, turbOut, turbOut);
    }
}

// Associates a random number in the range [-1:1] with a given lattice
// point.
float Turbulence::lattice(unsigned int x, unsigned int y)
{
    return i2fs(hashrand((((_seed << _gens) | x) << _gens) | y));
}

// Returns a scale for a vector that normalizes it into a sphere (as
// opposed to cube) space.  This prevents the overscaling of the
// "corner" vectors you get from choosing three random turbulence
// components.
float Turbulence::cubenorm(float x, float y, float z)
{
    x = x < 0 ? -x : x;
    y = y < 0 ? -y : y;
    z = z < 0 ? -z : z;
    float max = ((x > y) && (x > z)) ? x : ((y > z) ? y : z);
    return max/Math::sqrt(x*x + y*y + z*z);
}

Turbulence::~Turbulence()
{
    delete[] _data;
}

Turbulence::Turbulence(int gens, int seed)
{
    _gens = gens;
    _sz = 1 << (_gens - 1);
    _seed = seed;
    _mag = 1;
    _x0 = _x1 = _y0 = _y1 = _z0 = _z1 = 0;
    _timeOff = 0;
    _off[0] = _off[1] = _off[2] = 0;

    float* xbuf = new float[_sz*_sz];
    float* ybuf = new float[_sz*_sz];
    float* zbuf = new float[_sz*_sz];

    mkimg(xbuf, _sz);
    _seed++;
    mkimg(ybuf, _sz);
    _seed++;
    mkimg(zbuf, _sz);

    // "Normalize" them to proper spherical magnitudes, and calculate
    // range information for the packing.
    for(int i=0; i<_sz*_sz; i++) {
        float n = cubenorm(xbuf[i], ybuf[i], zbuf[i]);
        xbuf[i] *= n;
        ybuf[i] *= n;
        zbuf[i] *= n;

        _x0 = xbuf[i] < _x0 ? xbuf[i] : _x0;
        _x1 = xbuf[i] > _x1 ? xbuf[i] : _x1;
        _y0 = ybuf[i] < _y0 ? ybuf[i] : _y0;
        _y1 = ybuf[i] > _y1 ? ybuf[i] : _y1;
        _z0 = zbuf[i] < _z0 ? zbuf[i] : _z0;
        _z1 = zbuf[i] > _z1 ? zbuf[i] : _z1;
    }
    
    // Pack into 3 byte tuples for storage.
    _data = new unsigned char[3*_sz*_sz];
    for(int i=0; i<_sz*_sz; i++) {
        float x = xbuf[i], y = ybuf[i], z = zbuf[i];
        unsigned char* tuple = _data + 3*i;
        tuple[0] = f2cu((x - _x0) / (_x1 - _x0));
        tuple[1] = f2cu((y - _y0) / (_y1 - _y0));
        tuple[2] = f2cu((z - _z0) / (_z1 - _z0));
    }

    delete[] xbuf;
    delete[] ybuf;
    delete[] zbuf;
}

// "Integer" turbulence function.  Takes coordinates in the range
// [0:1] expressed as a fraction of 2^32 (works with 64 bit ints too;
// it just doesn't use the whole range). The output range is
// guaranteed to be within [-1:1], with a typical output range of +/-
// 0.6 or so.
float Turbulence::iturb(unsigned int x, unsigned int y)
{
    float amplitude = 0.5; // start here, so it all sums to ~1.0
    float total = 0;
    int wrapmax = 2;
    int startgen = _gens - MEANINGFUL_GENS;
    for(int g=startgen; g<_gens; g++) {
        int xl = x >> (32 - g); // lattice coordinates
        int yl = y >> (32 - g);
        float xfrac = i2fu(x << g); // interpolation fractions
        float yfrac = i2fu(y << g);
        xfrac = xfrac*xfrac*(3 - 2*xfrac); // ... as cubics
        yfrac = yfrac*yfrac*(3 - 2*yfrac);

	float p00 = lattice(xl,   yl); // lattice values
        float p01 = lattice(xl,   yl+1);
        float p10 = lattice(xl+1, yl);
        float p11 = lattice(xl+1, yl+1);

        float p0 = p00 * (1-yfrac) + p01 * yfrac;
        float p1 = p10 * (1-yfrac) + p11 * yfrac;
        float p = p0 * (1-xfrac) + p1 * xfrac;
        total += p * amplitude;
        amplitude *= 0.5;
        wrapmax *= 2;
    }
    return total;
}

// Converts "real" turbulence coordinates expressed in the range
// [0:_sz] (modulo) to integers and runs them through iturb().
float Turbulence::fturb(double a, double b)
{
    a *= 1.0 / _sz;
    b *= 1.0 / _sz;
    a -= Math::floor(a);
    b -= Math::floor(b);
    return iturb((unsigned int)(a * 4294967296.0),
                 (unsigned int)(b * 4294967296.0));
}

void Turbulence::mkimg(float* buf, int sz)
{
    for(int y=0; y<sz; y++)
        for(int x=0; x<sz; x++)
            buf[y*sz+x] = fturb(x + 0.5, y + 0.5);
}

}; // namespace yasim
