#ifndef _MATH_HPP
#define _MATH_HPP

namespace yasim {

class Math
{
public:
    // Dumb utilities
    static float clamp(float val, float min, float max);

    // Simple wrappers around library routines
    static float abs(float f);
    static float sqrt(float f);
    static float ceil(float f);
    static float sin(float f);
    static float cos(float f);
    static float tan(float f);
    static float atan(float f);
    static float atan2(float y, float x);
    static float asin(float f);
    static float acos(float f);
    static float exp(float f);
    static float sqr(float f) {return f*f;}

    // Takes two args and runs afoul of the Koenig rules.
    static float pow(double base, double exp);

    // double variants of the above
    static double abs(double f);
    static double sqrt(double f);
    static double ceil(double f);
    static double sin(double f);
    static double cos(double f);
    static double tan(double f);
    static double atan2(double y, double x);
    static double floor(double x);

    // Some 3D vector stuff.  In all cases, it is permissible for the
    // "out" vector to be the same as one of the inputs.
    static void  set3(float* v, float* out);
    static float dot3(float* a, float* b);
    static void  cross3(float* a, float* b, float* out);
    static void  mul3(float scalar, float* v, float* out);
    static void  add3(float* a, float* b, float* out);
    static void  sub3(float* a, float* b, float* out);
    static float mag3(float* v);
    static void  unit3(float* v, float* out);

    // Matrix array convention: 0 1 2
    //                          3 4 5
    //                          6 7 8

    // Multiply two matrices
    static void mmul33(float* a, float* b, float* out);

    // Multiply by vector
    static void vmul33(float* m, float* v, float* out);

    // Multiply the vector by the matrix transpose.  Or pre-multiply the
    // matrix by v as a row vector.  Same thing.
    static void tmul33(float* m, float* v, float* out);

    // Invert matrix
    static void invert33(float* m, float* out);

    // Transpose matrix (for an orthonormal orientation matrix, this
    // is the same as the inverse).
    static void trans33(float* m, float* out);

    // Generates an orthonormal basis:
    //   xOut becomes the unit vector in the direction of x
    //   yOut is perpendicular to xOut in the x/y plane
    //   zOut becomes the unit vector: (xOut cross yOut)
    static void ortho33(float* x, float* y,
                        float* xOut, float* yOut, float* zOut);
};

}; // namespace yasim
#endif // _MATH_HPP
