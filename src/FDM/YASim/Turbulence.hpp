namespace yasim {

class Turbulence {
public:
    Turbulence(int gens, int seed);
    ~Turbulence();
    void update(double dt, double rate);
    void setMagnitude(double mag) { _mag = mag; }
    void getTurbulence(double* loc, float* turbOut);

private:
    unsigned int hashrand(unsigned int i);
    float lattice(unsigned int x, unsigned int y);
    float iturb(unsigned int x, unsigned int y);
    float fturb(double a, double b);
    void mkimg(float* buf, int sz);
    float cubenorm(float x, float y, float z);
    void turblut(int x, int y, float* out);

    int _gens;
    int _sz;
    int _seed;

    double _currTime;
    double _mag;
    float _x0, _x1, _y0, _y1, _z0, _z1;
    unsigned char* _data;
};

}; // namespace yasim
