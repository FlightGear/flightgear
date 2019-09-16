#ifndef _MATH_HPP
#define _MATH_HPP

#include <cmath>
#include <vector>

namespace yasim {

class Math
{
public:
    // Dumb utilities
    static inline float clamp(float val, float min, float max) {
        if(val < min) return min;
        if(val > max) return max;
        return val;
    }
    static float weightedMean(float x1, float w1, float x2, float w2) {
        return (x1*w1 + x2*w2)/(w1+w2);
    }

    // calc polynomial
    static float polynomial(const std::vector<float>& coefficients, float x) {
        float powX {1}; // init = x^0 = 1
        float result {0};
        for (auto &coeff : coefficients) {
            result += coeff * powX;
            powX *= x;
        }
        return result;
    }
    
    // Simple wrappers around library routines
    static inline float abs(float f)  { return (float)::fabs(f); }
    static inline float sqrt(float f) { return (float)::sqrt(f); }
    static inline float ceil(float f) { return (float)::ceil(f); }
    static inline float sin(float f)  { return (float)::sin(f);  }
    static inline float cos(float f)  { return (float)::cos(f);  }
    static inline float tan(float f)  { return (float)::tan(f);  }
    static inline float atan(float f) { return (float)::atan(f); }
    static inline float atan2(float y, float x) { return (float)::atan2(y,x); }
    static inline float asin(float f) { return (float)::asin(f); }
    static inline float acos(float f) { return (float)::acos(f); }
    static inline float exp(float f)  { return (float)::exp(f);  }
    static inline float sqr(float f)  { return f*f; }

    // Takes two args and runs afoul of the Koenig rules.
    static inline float pow(double base, double exp) { return (float)::pow(base, exp); }

    // double variants of the above
    static inline double abs(double f)  { return ::fabs(f); }
    static inline double sqrt(double f) { return ::sqrt(f); }
    static inline double ceil(double f) { return ::ceil(f); }
    static inline double sin(double f)  { return ::sin(f); }
    static inline double cos(double f)  { return ::cos(f); }
    static inline double tan(double f)  { return ::tan(f); }
    static inline double atan2(double y, double x) { return ::atan2(y,x); }
    static inline double floor(double x) { return ::floor(x); }

    // Some 3D vector stuff.  In all cases, it is permissible for the
    // "out" vector to be the same as one of the inputs.
    static inline void  set3(const float* v, float* out) {
        out[0] = v[0];
        out[1] = v[1];
        out[2] = v[2];
    }
    static inline void  set3(const double* v, double* out) {
        out[0] = v[0];
        out[1] = v[1];
        out[2] = v[2];
    }

    static void  zero3(float* out) { out[0] = out[1] = out[2] = 0; }
    static void  zero3(double* out) { out[0] = out[1] = out[2] = 0; }

    static inline float dot3(const float* a, const float* b) {
        return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
    }

    static inline void  cross3(const float* a, const float* b, float* out) {
        float ax=a[0], ay=a[1], az=a[2];
        float bx=b[0], by=b[1], bz=b[2];
        out[0] = ay*bz - by*az;
        out[1] = az*bx - bz*ax;
        out[2] = ax*by - bx*ay;
    }

    static inline void  mul3(const float scalar, const float* v, float* out)
    {
        out[0] = scalar * v[0];
        out[1] = scalar * v[1];
        out[2] = scalar * v[2];
    }

    static inline void  add3(const float* a, const float* b, float* out){
        out[0] = a[0] + b[0];
        out[1] = a[1] + b[1];
        out[2] = a[2] + b[2];
    }

    static inline void  sub3(const float* a, const float* b, float* out) {
        out[0] = a[0] - b[0];
        out[1] = a[1] - b[1];
        out[2] = a[2] - b[2];
    }

    static inline float mag3(const float* v) {
        return sqrt(dot3(v, v));
    }

    static inline void  unit3(const float* v, float* out) {
        float imag = 1/mag3(v);
        mul3(imag, v, out);
    }

    // Matrix array convention: 0 1 2
    //                          3 4 5
    //                          6 7 8

    static inline void set33(float* a, float* b)
    {
        for (int i=0; i < 9; i++)
            a[i] = b[i];
    }

    static inline void identity33(double* a)
    {
        for(int y = 0; y < 3; y++) {
            for(int x = 0; x < 3; x++) {
                a[y*3 + x] = ((x == y) ? 1.0 : 0.0);
            }
        }
    }

    // Multiply two matrices
    static void mmul33(const float* a, const float* b, float* out) {
        float tmp[9];
        tmp[0] = a[0]*b[0] + a[1]*b[3] + a[2]*b[6];
        tmp[3] = a[3]*b[0] + a[4]*b[3] + a[5]*b[6];
        tmp[6] = a[6]*b[0] + a[7]*b[3] + a[8]*b[6];

        tmp[1] = a[0]*b[1] + a[1]*b[4] + a[2]*b[7];
        tmp[4] = a[3]*b[1] + a[4]*b[4] + a[5]*b[7];
        tmp[7] = a[6]*b[1] + a[7]*b[4] + a[8]*b[7];

        tmp[2] = a[0]*b[2] + a[1]*b[5] + a[2]*b[8];
        tmp[5] = a[3]*b[2] + a[4]*b[5] + a[5]*b[8];
        tmp[8] = a[6]*b[2] + a[7]*b[5] + a[8]*b[8];

        for(int i=0; i<9; ++i)
            out[i] = tmp[i];
    }

    // Multiply by vector
    static inline void vmul33(const float* m, const float* v, float* out) {
        float x = v[0], y = v[1], z = v[2];
        out[0] = x*m[0] + y*m[1] + z*m[2];
        out[1] = x*m[3] + y*m[4] + z*m[5];
        out[2] = x*m[6] + y*m[7] + z*m[8];
    }

    static inline void vmul33(float* m, double* v, double* out) {
        double x = v[0], y = v[1], z = v[2];
        out[0] = x*m[0] + y*m[1] + z*m[2];
        out[1] = x*m[3] + y*m[4] + z*m[5];
        out[2] = x*m[6] + y*m[7] + z*m[8];
    }

    static inline void vmul33(double* m, double* v, double* out) {
        double x = v[0], y = v[1], z = v[2];
        out[0] = x*m[0] + y*m[1] + z*m[2];
        out[1] = x*m[3] + y*m[4] + z*m[5];
        out[2] = x*m[6] + y*m[7] + z*m[8];
    }

    // Multiply the vector by the matrix transpose.  Or pre-multiply the
    // matrix by v as a row vector.  Same thing.
    static inline void tmul33(const float* m, const float* v, float* out) {
        float x = v[0], y = v[1], z = v[2];
        out[0] = x*m[0] + y*m[3] + z*m[6];
        out[1] = x*m[1] + y*m[4] + z*m[7];
        out[2] = x*m[2] + y*m[5] + z*m[8];
    }

    static inline void tmul33(float* m, double* v, double* out) {
        double x = v[0], y = v[1], z = v[2];
        out[0] = x*m[0] + y*m[3] + z*m[6];
        out[1] = x*m[1] + y*m[4] + z*m[7];
        out[2] = x*m[2] + y*m[5] + z*m[8];
    }

    static inline void tmul33(double* m, double* v, double* out) {
        double x = v[0], y = v[1], z = v[2];
        out[0] = x*m[0] + y*m[3] + z*m[6];
        out[1] = x*m[1] + y*m[4] + z*m[7];
        out[2] = x*m[2] + y*m[5] + z*m[8];
    }

    /// Invert symmetric matrix; ~1/3 less calculations due to symmetry
    static void invert33_sym(const float* m, float* out) {
        // Compute the inverse as the adjoint matrix times 1/(det M).
        // A, B ... I are the cofactors of a b c
        //                                 d e f
        //                                 g h i
        // symetric: d=b, g=c, h=f
        float a=m[0], b=m[1], c=m[2];
        float         e=m[4], f=m[5];
        float                 i=m[8];

        float A =  (e*i - f*f);
        float B = -(b*i - c*f);
        float C =  (b*f - c*e);
        float E =  (a*i - c*c);
        float F = -(a*f - c*b);
        float I =  (a*e - b*b);

        float id = 1/(a*A + b*B + c*C);

        out[0] = id*A;   out[1] = id*B;   out[2] = id*C;
        out[3] = out[1]; out[4] = id*E;   out[5] = id*F;
        out[6] = out[2]; out[7] = out[5]; out[8] = id*I;
    }

    // Transpose matrix (for an orthonormal orientation matrix, this
    // is the same as the inverse).
    static inline void trans33(const float* m, float* out) {
        // 0 1 2   Elements 0, 4, and 8 are the same
        // 3 4 5   Swap elements 1/3, 2/6, and 5/7
        // 6 7 8
        out[0] = m[0];
        out[4] = m[4];
        out[8] = m[8];

        float tmp = m[1];
        out[1] = m[3];
        out[3] = tmp;

        tmp = m[2];
        out[2] = m[6];
        out[6] = tmp;

        tmp = m[5];
        out[5] = m[7];
        out[7] = tmp;
    }

    // Generates an orthonormal basis:
    //   xOut becomes the unit vector in the direction of x
    //   yOut is perpendicular to xOut in the x/y plane
    //   zOut becomes the unit vector: (xOut cross yOut)
    static void ortho33(const float* x, const float* y,
                        float* xOut, float* yOut, float* zOut) {
        float x0[3], y0[3];
        set3(x, x0);
        set3(y, y0);

        unit3(x0, xOut);
        cross3(xOut, y0, zOut);
        unit3(zOut, zOut);
        cross3(zOut, xOut, yOut);
    }
};

}; // namespace yasim
#endif // _MATH_HPP
