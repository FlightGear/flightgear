#ifndef FAKEFGAIAIRCRAFT
#define FAKEFGAIAIRCRAFT

struct Globals {
    Globals(void) { root.getNode("environment/density-slugft3", true); }
    SGPropertyNode* get_props(void) { return &root; }
    SGPropertyNode root;
};

struct PerformanceData {
    double _span, _chord, _weight;
    PerformanceData(double s, double c, double w)
        : _span(s), _chord(c), _weight(w) {}
    double wingSpan(void) const { return _span; }
    double wingChord(void) const { return _chord; }
    double weight(void) const { return _weight; }
};

class FGAIAircraft {
public:
    explicit FGAIAircraft(int id)
        : _id(id), _pos(SGVec3d::zeros()), _heading(0.0), _pitch(0.0),
          _vel(0.0), _perf(0.0, 0.0, 0.0) {}
    void setPosition(const SGVec3d& pos) { _pos = pos; }
    void setOrientation(double heading, double pitch)
    {
        _heading = heading;
        _pitch = pitch;
    }
    void setGeom(double s, double c, double w)
    {
        _perf._span = s;
        _perf._chord = c;
        _perf._weight = w;
    }
    void setVelocity(double v) { _vel = v; }
    const SGVec3d& getCartPos(void) const { return _pos; }
    double _getHeading(void) const { return _heading; }
    double getPitch(void) const { return _pitch; }
    int getID(void) const { return _id; }
    PerformanceData* getPerformance(void) { return &_perf; }
    std::string getAcType(void) const { return "The AC type"; }
    std::string _getName(void) const { return "The AC name"; }
    double getSpeed(void) const { return _vel * SG_FPS_TO_KT; }
    double getVerticalSpeed(void) const { return 0.0; }
    double getAltitude(void) const { return 3000.; }
    double getRoll(void) const { return 0.0; }

private:
    int _id;
    SGVec3d _pos;
    double _heading, _pitch, _vel;
    PerformanceData _perf;
};

extern Globals* globals;

#endif
