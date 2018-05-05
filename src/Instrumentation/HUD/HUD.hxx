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

#include <vector>
#include <deque>

#include <osg/State>

#include <simgear/math/SGLimits.hxx>
#include <simgear/constants.h>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/props/props.hxx>

class FGFontCache;
class fntRenderer;
class fntTexFont;
class FGViewer;
class ClipBox;

class LineSegment
{
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



class LineList
{
public:
    void add(const LineSegment& seg) { _list.push_back(seg); }
    void erase() { _list.erase(_list.begin(), _list.end()); }
    inline unsigned int size() const { return _list.size(); }
    inline bool empty() const { return _list.empty(); }
    void draw() {
        glBegin(GL_LINES);
        std::vector<LineSegment>::const_iterator it, end = _list.end();
        for (it = _list.begin(); it != end; ++it)
            it->draw();
        glEnd();
    }

private:
  std::vector<LineSegment> _list;
};



class HUDText
{
public:
    HUDText(fntRenderer *f, float x, float y, const char *s, int align = 0, int digits = 0);
    void draw();

private:
    fntRenderer *_fnt;
    float _x, _y;
    int _digits;
    static const int BUFSIZE = 64;
    char _msg[BUFSIZE];
};



class TextList
{
public:
    TextList() { _font = 0; }

    void setFont(fntRenderer *Renderer) { _font = Renderer; }
    void add(float x, float y, const char *s, int align = 0, int digit = 0) {
        _list.push_back(HUDText(_font, x, y, s, align, digit));
    }
    void erase() { _list.erase(_list.begin(), _list.end()); }
    void align(const char *s, int align, float *x, float *y,
            float *l, float *r, float *b, float *t) const;
    void draw();

private:
    fntRenderer *_font;
    std::vector<HUDText> _list;
};



class HUD : public SGSubsystem,
            public SGPropertyChangeListener
{
public:
    HUD();
    ~HUD();

    // Subsystem API.
    void init() override;
    void reinit() override;
    void update(double) override;

    // Subsystem identification.
    static const char* staticSubsystemClassId() { return "hud"; }

    // called from Main/renderer.cxx to draw 2D and 3D HUD
    void draw(osg::State&);

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
    static void textAlign(fntRenderer *rend, const char *s, int align, float *x, float *y,
            float *l, float *r, float *b, float *t);

    enum Units { FEET, METER };
    Units getUnits() const { return _units; }

    enum {
        HORIZONTAL = 0x0000,  // keep that at zero?
        VERTICAL   = 0x0001,
        TOP        = 0x0002,
        BOTTOM     = 0x0004,
        LEFT       = 0x0008,
        RIGHT      = 0x0010,
        NOTICKS    = 0x0020,
        NOTEXT     = 0x0040,
        BOTH       = (LEFT|RIGHT),

        // for alignment (with LEFT, RIGHT, TOP, BOTTOM)
        HCENTER    = 0x0080,
        VCENTER    = 0x0100,
        CENTER     = (HCENTER|VCENTER),
    };

protected:
    void common_draw();
    int load(const char *, float x = 320.0f, float y = 240.0f,
            int level = 0, const std::string& indent = "");

private:
    void deinit();

    void draw3D();
    void draw2D(GLfloat, GLfloat, GLfloat, GLfloat);

    void currentColorChanged();

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

    std::deque<Item *> _items;
    std::deque<Item *> _ladders;

    SGPropertyNode_ptr _currentPath;
    SGPropertyNode_ptr _currentColor;
    SGPropertyNode_ptr _visibility;
    SGPropertyNode_ptr _3DenabledN;
    SGPropertyNode_ptr _antialiasing;
    SGPropertyNode_ptr _transparency;
    SGPropertyNode_ptr _red, _green, _blue, _alpha;
    SGPropertyNode_ptr _alpha_clamp;
    SGPropertyNode_ptr _brightness;
    bool _visible;
    bool _loaded;
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
    bool _listener_active;

    ClipBox *_clip_box;
    TextList _text_list;
    LineList _line_list;
    LineList _stipple_line_list;
};

#endif // _HUD_HXX
