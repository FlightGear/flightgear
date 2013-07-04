// HUD_private.hxx -- Intenral delcerations for the HUD
//
// Written by Michele America, started September 1997.
//
// Copyright (C) 1997  Michele F. America  [micheleamerica#geocities:com]
// Copyright (C) 2006  Melchior FRANZ  [mfranz#aon:at]
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifndef _HUD_PRIVATE_HXX
#define _HUD_PRIVATE_HXX

#include <simgear/compiler.h>
#include <simgear/props/condition.hxx>

#include <vector>
#include <deque>
#include <cassert>

#include <osg/State>

#include <simgear/math/SGLimits.hxx>
#include <simgear/constants.h>
#include <simgear/props/props.hxx>

#include <plib/sg.h> // for lingering sgdVec3 usage below

class FGFontCache;
class fntRenderer;
class fntTexFont;
class FGViewer;
class FGRunway;

class ClipBox {
public:
  ClipBox(const SGPropertyNode *, float xoffset = 0, float yoffset = 0);
  void set();
  void unset();
  
private:
  bool _active;
  float _xoffs, _yoffs;
  SGConstPropertyNode_ptr _top_node;
  SGConstPropertyNode_ptr _bot_node;
  SGConstPropertyNode_ptr _left_node;
  SGConstPropertyNode_ptr _right_node;
  GLdouble _top[4];
  GLdouble _bot[4];
  GLdouble _left[4];
  GLdouble _right[4];
};


class HUD::Input {
public:
    Input(const SGPropertyNode *n, float factor = 1.0, float offset = 0.0,
          float min = -SGLimitsf::max(), float max = SGLimitsf::max());
  
    bool getBoolValue() const {
        assert(_property);
        return _property->getBoolValue();
    }

    const char *getStringValue() const {
        assert(_property);
        return _property->getStringValue();
    }

    float getFloatValue() {
        assert(_property);
        float f = _property->getFloatValue() * _factor + _offset;
        if (_damped == SGLimitsf::max())
            _damped = f;
        if (_coeff > 0.0f)
            f = _damped = f * (1.0f - _coeff) + _damped * _coeff;
        return clamp(f);
    }

    inline float isValid() const { return _valid; }
    inline float min() const { return _min; }
    inline float max() const { return _max; }
    inline float factor() const { return _factor; }
    float clamp(float v) const { return v < _min ? _min : v > _max ? _max : v; }

    void set_min(float m, bool force = true) {
        if (force || _min == -SGLimitsf::max())
            _min = m;
    }
    void set_max(float m, bool force = true) {
        if (force || _max == SGLimitsf::max())
            _max = m;
    }

private:
    bool _valid;
    SGConstPropertyNode_ptr _property;
    float _factor;
    float _offset;
    float _min;
    float _max;
    float _coeff;
    float _damped;
};



class HUD::Item {
public:
    Item(HUD *parent, const SGPropertyNode *, float x = 0.0f, float y = 0.0f);
    virtual ~Item () {}
    virtual void draw() = 0;
    virtual bool isEnabled();

protected:
    enum Format {
        INVALID,
        NONE,
        INT,
        LONG,
        FLOAT,
        DOUBLE,
        STRING,
    };

    Format  check_format(const char *) const;
    inline float get_span()       const { return _scr_span; }
    inline int   get_digits()     const { return _digits; }

    inline bool option_vert()    const { return (_options & VERTICAL) == VERTICAL; }
    inline bool option_left()    const { return (_options & LEFT) == LEFT; }
    inline bool option_right()   const { return (_options & RIGHT) == RIGHT; }
    inline bool option_both()    const { return (_options & BOTH) == BOTH; }
    inline bool option_noticks() const { return (_options & NOTICKS) == NOTICKS; }
    inline bool option_notext()  const { return (_options & NOTEXT) == NOTEXT; }
    inline bool option_top()     const { return (_options & TOP) == TOP; }
    inline bool option_bottom()  const { return (_options & BOTTOM) == BOTTOM; }

    void draw_line(float x1, float y1, float x2, float y2);
    void draw_stipple_line(float x1, float y1, float x2, float y2);
    void draw_text(float x, float y, const char *msg, int align = 0, int digit = 0);
    void draw_circle(float x1, float y1, float r) const;
    void draw_arc(float x1, float y1, float t0, float t1, float r) const;
    void draw_bullet(float, float, float);

    HUD         *_hud;
    std::string      _name;
    int         _options;
    float       _x, _y, _w, _h;
    float       _center_x, _center_y;

private:
    SGSharedPtr<SGCondition> _condition;
    float       _scr_span;      // Working values for draw;
    int         _digits;
};



class HUD::Label : public Item {
public:
    Label(HUD *parent, const SGPropertyNode *, float x, float y);
    virtual void draw();

private:
    bool    blink();

    Input   _input;
    Format  _mode;
    std::string  _format;
    int     _halign;    // HUDText alignment
    bool    _box;
    float   _pointer_width;
    float   _pointer_length;

    SGSharedPtr<SGCondition> _blink_condition;
    double  _blink_interval;
    double  _blink_target;  // time for next blink state change
    bool    _blink_state;
};



// abstract base class for both moving scale and moving needle (fixed scale)
// indicators.
//
class HUD::Scale : public Item {
public:
    Scale(HUD *parent, const SGPropertyNode *, float x, float y);
    virtual void draw    ( void ) {}  // No-op here. Defined in derived classes.

protected:
    inline float factor() const { return _display_factor; }
    inline float range_to_show() const { return _range_shown; }

    Input _input;
    float _major_divs;      // major division marker units
    float _minor_divs;      // minor division marker units
    unsigned int _modulo;   // Roll over point

private:
    float _range_shown;     // Width Units.
    float _display_factor;  // factor => screen units/range values.
};


class HUD::Gauge : public Scale {
public:
    Gauge(HUD *parent, const SGPropertyNode *, float x, float y);
    virtual void draw();
};



// displays the indicated quantity on a scale that moves past the
// pointer. It may be horizontal or vertical.
//
class HUD::Tape : public Scale {
public:
    Tape(HUD *parent, const SGPropertyNode *, float x, float y);
    virtual void draw();

protected:
    void draw_vertical(float);
    void draw_horizontal(float);
    void draw_fixed_pointer(float, float, float, float, float, float);
    char *format_value(float);

private:
    float  _val_span;
    float  _half_width_units;
    bool   _draw_tick_bottom;
    bool   _draw_tick_top;
    bool   _draw_tick_right;
    bool   _draw_tick_left;
    bool   _draw_cap_bottom;
    bool   _draw_cap_top;
    bool   _draw_cap_right;
    bool   _draw_cap_left;
    float  _marker_offset;
    float  _label_offset;
    float  _label_gap;
    bool   _pointer;
    Format _label_fmt;
    std::string _format;
    int    _div_ratio;          // _major_divs/_minor_divs
    bool   _odd_type;           // whether to put numbers at 0/2/4 or 1/3/5

    enum { BUFSIZE = 64 };
    char   _buf[BUFSIZE];

    enum PointerType { FIXED, MOVING } _pointer_type;
    enum TickType { LINE, CIRCLE } _tick_type;
    enum TickLength { VARIABLE, CONSTANT } _tick_length;
};



class HUD::Dial : public Scale {
public:
    Dial(HUD *parent, const SGPropertyNode *, float x, float y);
    virtual void draw();

private:
    float  _radius;
    int    _divisions;
};



class HUD::TurnBankIndicator : public Item {
public:
    TurnBankIndicator(HUD *parent, const SGPropertyNode *, float x, float y);
    virtual void draw();

private:
    void draw_scale();
    void draw_tee();
    void draw_line(float, float, float, float);
    void draw_tick(float angle, float r1, float r2, int side);

    Input _bank;
    Input _sideslip;

    float _gap_width;
    bool  _bank_scale;
};



class HUD::Ladder : public Item {
public:
    Ladder(HUD *parent, const SGPropertyNode *, float x, float y);
    ~Ladder();
    virtual void draw();

private:
    void draw_zenith(float, float);
    void draw_nadir(float, float);

    void draw_text(float x, float y, const char *s, int align = 0) {
        _locTextList.add(x, y, s, align, 0);
    }

    void draw_line(float x1, float y1, float x2, float y2, bool stipple = false) {
        if (stipple)
            _locStippleLineList.add(LineSegment(x1, y1, x2, y2));
        else
            _locLineList.add(LineSegment(x1, y1, x2, y2));
    }

    enum   Type { PITCH, CLIMB_DIVE } _type;
    Input  _pitch;
    Input  _roll;
    float  _width_units;
    int    _div_units;
    float  _scr_hole;
    float  _zero_bar_overlength;
    bool   _dive_bar_angle;
    float  _tick_length;
    float  _vmax;
    float  _vmin;
    float  _compression;
    bool   _dynamic_origin;
    bool   _frl;               // fuselage reference line
    bool   _target_spot;
    bool   _target_markers;
    bool   _velocity_vector;
    bool   _drift_marker;
    bool   _alpha_bracket;
    bool   _energy_marker;
    bool   _climb_dive_marker;
    bool   _glide_slope_marker;
    float  _glide_slope;
    bool   _energy_worm;
    bool   _waypoint_marker;
    bool   _zenith;
    bool   _nadir;
    bool   _hat;

    ClipBox *_clip_box;
    // The Ladder has its own temporary display lists
    TextList _locTextList;
    LineList _locLineList;
    LineList _locStippleLineList;
};



// responsible for rendering the active runway in the hud (if visible).
//
class HUD::Runway : public Item {
public:
    Runway(HUD *parent, const SGPropertyNode *, float x, float y);
    virtual void draw();

private:
    void boundPoint(const sgdVec3& v, sgdVec3& m);
    bool boundOutsidePoints(sgdVec3& v, sgdVec3& m);
    bool drawLine(const sgdVec3& a1, const sgdVec3& a2, const sgdVec3& p1, const sgdVec3& p2);
    void drawArrow();
    FGRunway* get_active_runway();
    void get_rwy_points(sgdVec3 *points);
    void setLineWidth();

    SGPropertyNode_ptr _agl;
    sgdVec3 _points3d[6], _points2d[6];
    double _mm[16];
    double _pm[16];
    double _arrow_scale;  // scales of runway indication arrow
    double _arrow_radius;
    double _line_scale;   // maximum line scale
    double _scale_dist;   // distance where to start scaling the lines
    double _default_pitch;
    double _default_heading;
    GLint  _view[4];
    FGRunway* _runway;
    unsigned short _stipple_out;    // stipple pattern of the outline of the runway
    unsigned short _stipple_center; // stipple pattern of the center line of the runway
    bool   _draw_arrow;             // draw arrow when runway is not visible in HUD
    bool   _draw_arrow_always;      // always draws arrow
    float  _left, _right, _top, _bottom;
};


class HUD::AimingReticle : public Item {
public:
    AimingReticle(HUD *parent, const SGPropertyNode *, float x, float y);
    virtual void draw();

private:
    SGSharedPtr<SGCondition> _active_condition;  // stadiametric (true) or standby (false)
    SGSharedPtr<SGCondition> _tachy_condition;  // tachymetric (true) or standby (false)
    SGSharedPtr<SGCondition> _align_condition;  // tachymetric (true) or standby (false)

    Input   _diameter;               // inner/outer radius relation
    Input  _pitch;
    Input  _yaw;
    Input  _speed;
    Input  _range;
    Input  _t0;
    Input  _t1;
    Input  _offset_x;
    Input  _offset_y;

    float   _bullet_size;
    float   _inner_radius;
    float   _compression;
    float  _limit_x;
    float  _limit_y;

};


#endif // _HUD_HXX
