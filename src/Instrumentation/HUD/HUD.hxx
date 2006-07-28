// HUD.hxx -- Head Up Display
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

#ifndef _HUD_HXX
#define _HUD_HXX

#include <simgear/compiler.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <vector>
#include <deque>
#include STL_FSTREAM

SG_USING_STD(deque);
SG_USING_STD(vector);
SG_USING_NAMESPACE(std);

#include <plib/sg.h>

#include <simgear/math/SGLimits.hxx>
#include <simgear/constants.h>
#include <simgear/structure/subsystem_mgr.hxx>

#include <Airports/runways.hxx>     // FGRunway
#include <GUI/gui.h>                // fntRenderer ?   guiErrorMessage()
#include <GUI/new_gui.hxx>          // FGFontCache, FGColor
#include <Include/fg_typedefs.h>
#include <Main/fg_props.hxx>


class FGViewer;
class SGCondition;



class LineSegment {
public:
    LineSegment(GLfloat x0, GLfloat y0, GLfloat x1, GLfloat y1)
        : _x0(x0), _y0(y0), _x1(x1), _y1(y1) {}

    void draw() const {
        glVertex2f(_x0, _y0);
        glVertex2f(_x1, _y1);
    }

private:
    GLfloat _x0, _y0, _x1, _y1;
};



class LineList {
public:
    void add(const LineSegment& seg) { _list.push_back(seg); }
    void erase() { _list.erase(_list.begin(), _list.end()); }
    inline unsigned int size() const { return _list.size(); }
    void draw() {
        glBegin(GL_LINES);
        vector<LineSegment>::const_iterator it, end = _list.end();
        for (it = _list.begin(); it != end; ++it)
            it->draw();
        glEnd();
    }

private:
    vector<LineSegment> _list;
};



class HUDText {
public:
    HUDText(float x, float y, char *s, int d = 0) : _x(x), _y(y), _digits(d) {
        strncpy(_msg, s, BUFSIZE);
    }

    // this code is changed to display Numbers with big/small digits
    // according to MIL Standards for example Altitude above 10000 ft
    // is shown as 10ooo.

    void draw(fntRenderer *fnt) {
        if (!_digits) { // show all digits in same size
            fnt->start2f(_x, _y);
            fnt->puts(_msg);
            return;
        }

        int c = 0, i = 0;
        char *t = _msg;
        int p = 4;

        if (t[0] == '-') {
            //if negative value then increase the c and p values
            //for '-' sign.
            c++; // was moved to the comment. Unintentionally?   TODO
            p++;
        }
        char *tmp = _msg;
        while (tmp[i] != '\0') {
            if ((tmp[i] >= '0') && (tmp[i] <= '9'))
                c++;
            i++;
        }

        float orig_size = fnt->getPointSize();
        if (c > p) {
            fnt->setPointSize(orig_size * 0.8);
            int p1 = c - 3;
            char *tmp1 = _msg + p1;
            int p2 = p1 * 8;

            fnt->start2f(_x + p2, _y);
            fnt->puts(tmp1);

            fnt->setPointSize(orig_size * 1.2);
            char tmp2[BUFSIZE];
            strncpy(tmp2, _msg, p1);
            tmp2[p1] = '\0';

            fnt->start2f(_x, _y);
            fnt->puts(tmp2);
        } else {
            fnt->setPointSize(orig_size * 1.2);
            fnt->start2f(_x, _y);
            fnt->puts(tmp);
        }
        fnt->setPointSize(orig_size);
    }

private:
    float _x, _y;
    int _digits;
    static const int BUFSIZE = 64;
    char _msg[BUFSIZE];
};



class TextList {
public:
    TextList() { _font = 0; }

    void setFont(fntRenderer *Renderer) { _font = Renderer; }
    void add(const HUDText& String) { _list.push_back(String); }
    void erase() { _list.erase(_list.begin(), _list.end()); }
    void draw() {
        assert(_font);

        // FIXME
        glPushAttrib(GL_COLOR_BUFFER_BIT);
        glEnable(GL_BLEND);

        _font->begin();
        vector<HUDText>::iterator it, end = _list.end();
        for (it = _list.begin(); it != end; ++it)
            it->draw(_font);
        _font->end();

        glDisable(GL_TEXTURE_2D);
        glPopAttrib();
    }

private:
    fntRenderer *_font;
    vector<HUDText> _list;
};






class HUD : public SGSubsystem, public SGPropertyChangeListener {
public:
    HUD();
    ~HUD();
    void init();
    void update(double);

    // called from Main/renderer.cxx to draw 2D and 3D HUD
    void draw();

    // listener callback to read various HUD related properties
    void valueChanged(SGPropertyNode *);
    // set current glColor
    void setColor() const;
    inline bool isVisible() const { return _visible; }
    inline bool isAntialiased() const { return _antialiased; }
    inline bool isTransparent() const { return _transparent; }
    inline bool is3D() const { return _3Denabled; }
    inline float alphaClamp() const { return _cl; }
    inline double timer() const { return _timer; }

    enum Units { FEET, METER };
    Units getUnits() const { return _units; }

    enum {
        HORZ      = 0x0000,  // keep that at zero?
        AUTOTICKS = 0x0001,
        VERT      = 0x0002,
        TOP       = 0x0004,
        BOTTOM    = 0x0008,
        LEFT      = 0x0010,
        RIGHT     = 0x0020,
        BOTH      = (LEFT|RIGHT),
        NOTICKS   = 0x0040,
        ARITHTIC  = 0x0080,
        DECITICS  = 0x0100,
        NOTEXT    = 0x0200,
    };

    enum Adjust {
        LEFT_ALIGN,
        CENTER_ALIGN,
        RIGHT_ALIGN
    };

protected:
    void common_draw();
    int load(const char *, float x = 320.0f, float y = 240.0f,
            int level = 0, const string& indent = "");

private:
    void draw3D();
    void draw2D(GLfloat, GLfloat, GLfloat, GLfloat);

    class Input;
    class Item;
    class Label;
    class Scale;
    class Gauge;
    class Tape;
    class Dial;
    class TurnBankIndicator;
    class Ladder;
    class Runway;
    class AimingReticle;

    deque<Item *> _items;

    SGPropertyNode_ptr _current;
    SGPropertyNode_ptr _visibility;
    SGPropertyNode_ptr _3DenabledN;
    SGPropertyNode_ptr _antialiasing;
    SGPropertyNode_ptr _transparency;
    SGPropertyNode_ptr _red, _green, _blue, _alpha;
    SGPropertyNode_ptr _alpha_clamp;
    SGPropertyNode_ptr _brightness;
    bool _visible;
    bool _3Denabled;
    bool _antialiased;
    bool _transparent;
    float _r, _g, _b, _a, _cl;

    SGPropertyNode_ptr _scr_widthN, _scr_heightN;
    int _scr_width, _scr_height;
    SGPropertyNode_ptr _unitsN;
    Units _units;
    double _timer;

    fntRenderer *_font_renderer;
    FGFontCache *_font_cache;
    fntTexFont *_font;
    float _font_size;
    int _style;

    TextList _text_list;
    LineList _line_list;
    LineList _stipple_line_list;
};



class HUD::Input {
public:
    Input(const SGPropertyNode *n, float factor = 1.0, float offset = 0.0,
            float min = -SGLimitsf::max(), float max = SGLimitsf::max()) :
        _valid(false),
        _property(0),
        _damped(SGLimitsf::max())
    {
        if (!n)
            return;
        _factor = n->getFloatValue("factor", factor);
        _offset = n->getFloatValue("offset", offset);
        _min = n->getFloatValue("min", min);
        _max = n->getFloatValue("max", max);
        _coeff = 1.0 - 1.0 / powf(10, fabsf(n->getFloatValue("damp", 0.0)));
        SGPropertyNode *p = ((SGPropertyNode *)n)->getNode("property", false);
        if (p) {
            const char *path = p->getStringValue();
            if (path && path[0]) {
                _property = fgGetNode(path, true);
                _valid = true;
            }
        }
    }

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
    inline float get_span()       const { return _scr_span; }
    inline int   get_digits()     const { return _digits; }

    inline bool option_vert()    const { return (_options & VERT) == VERT; }
    inline bool option_left()    const { return (_options & LEFT) == LEFT; }
    inline bool option_right()   const { return (_options & RIGHT) == RIGHT; }
    inline bool option_both()    const { return (_options & BOTH) == BOTH; }
    inline bool option_noticks() const { return (_options & NOTICKS) == NOTICKS; }
    inline bool option_notext()  const { return (_options & NOTEXT) == NOTEXT; }
    inline bool option_top()     const { return (_options & TOP) == TOP; }
    inline bool option_bottom()  const { return (_options & BOTTOM) == BOTTOM; }

    void draw_line( float x1, float y1, float x2, float y2);
    void draw_stipple_line( float x1, float y1, float x2, float y2);
    void draw_text( float x, float y, char *msg, int digit);
    float text_width(char *str) const;
    void draw_circle(float x1, float y1, float r) const;
    void draw_bullet(float, float, float);

    HUD         *_hud;
    string      _name;
    int         _options;
    float       _x, _y, _w, _h;
    float       _center_x, _center_y;

private:
    SGCondition *_condition;
    float       _disp_factor;   // Multiply by to get numbers shown on scale.
    float       _scr_span;      // Working values for draw;
    int         _digits;
};



class HUD::Label : public Item {
public:
    Label(HUD *parent, const SGPropertyNode *, float x, float y);
    virtual void draw();

private:
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
    bool    blink();

    Input   _input;
    Format  _mode;
    string  _format;
    Adjust  _halign;
    int     _blink;
    bool    _box;
    float   _text_y;
    float   _pointer_width;
    float   _pointer_length;

    SGCondition *_blink_condition;
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
    void fixed(float, float, float, float, float, float);
    void zoomed_scale(int, int);

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
    bool   _pointer;
    int    _zoom;
    float  _0_ht;

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
    virtual void draw();

private:
    void draw_zenith(float, float);
    void draw_nadir(float, float);

    void draw_text(float x, float y, char *s) {
        _locTextList.add(HUDText(x, y, s));
    }

    void draw_line(float x1, float y1, float x2, float y2) {
        _locLineList.add(LineSegment(x1, y1, x2, y2));
    }

    void draw_stipple_line(float x1, float y1, float x2, float y2) {
        _locStippleLineList.add(LineSegment(x1, y1, x2, y2));
    }

    enum   Type { PITCH, CLIMB_DIVE } _type;
    Input  _pitch;
    Input  _roll;
    float  _width_units;
    int    _div_units;
    unsigned int _scr_hole;
    float  _vmax;
    float  _vmin;
    float  _compression;
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
    bool get_active_runway(FGRunway& rwy);
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
    FGRunway _runway;
    FGViewer* _cockpit_view;
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
    SGCondition *_active_condition;  // stadiametric (true) or standby (false)
    Input   _diameter;               // inner/outer radius relation
    float   _bullet_size;
    float   _inner_radius;
};



#endif // _HUD_HXX
