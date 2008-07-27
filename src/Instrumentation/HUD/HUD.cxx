// HUD.cxx -- Head Up Display
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

#include <simgear/compiler.h>
#include <simgear/structure/exception.hxx>

#include <string>
#include <fstream>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/constants.h>
#include <simgear/misc/sg_path.hxx>
#include <osg/GLU>

#include <Main/globals.hxx>
#include <Main/viewmgr.hxx>

#include "HUD.hxx"


static float clamp(float f)
{
    return f < 0.0f ? 0.0f : f > 1.0f ? 1.0f : f;
}


HUD::HUD() :
    _current(fgGetNode("/sim/hud/current-color", true)),
    _visibility(fgGetNode("/sim/hud/visibility[1]", true)),
    _3DenabledN(fgGetNode("/sim/hud/enable3d[1]", true)),
    _antialiasing(fgGetNode("/sim/hud/color/antialiased", true)),
    _transparency(fgGetNode("/sim/hud/color/transparent", true)),
    _red(fgGetNode("/sim/hud/color/red", true)),
    _green(fgGetNode("/sim/hud/color/green", true)),
    _blue(fgGetNode("/sim/hud/color/blue", true)),
    _alpha(fgGetNode("/sim/hud/color/alpha", true)),
    _alpha_clamp(fgGetNode("/sim/hud/color/alpha-clamp", true)),
    _brightness(fgGetNode("/sim/hud/color/brightness", true)),
    _visible(false),
    _antialiased(false),
    _transparent(false),
    _a(0.67),									// FIXME better names
    _cl(0.01),
    //
    _scr_widthN(fgGetNode("/sim/startup/xsize", true)),
    _scr_heightN(fgGetNode("/sim/startup/ysize", true)),
    _unitsN(fgGetNode("/sim/startup/units", true)),
    _timer(0.0),
    //
    _font_renderer(new fntRenderer()),
    _font(0),
    _font_size(0.0),
    _style(0)
{
    SG_LOG(SG_COCKPIT, SG_INFO, "Initializing HUD Instrument");

    _visibility->addChangeListener(this);
    _3DenabledN->addChangeListener(this);
    _antialiasing->addChangeListener(this);
    _transparency->addChangeListener(this);
    _red->addChangeListener(this);
    _green->addChangeListener(this);
    _blue->addChangeListener(this);
    _alpha->addChangeListener(this);
    _alpha_clamp->addChangeListener(this);
    _brightness->addChangeListener(this);
    _current->addChangeListener(this);
    _scr_widthN->addChangeListener(this);
    _scr_heightN->addChangeListener(this);
    _unitsN->addChangeListener(this, true);
}


HUD::~HUD()
{
    _visibility->removeChangeListener(this);
    _3DenabledN->removeChangeListener(this);
    _antialiasing->removeChangeListener(this);
    _transparency->removeChangeListener(this);
    _red->removeChangeListener(this);
    _green->removeChangeListener(this);
    _blue->removeChangeListener(this);
    _alpha->removeChangeListener(this);
    _alpha_clamp->removeChangeListener(this);
    _brightness->removeChangeListener(this);
    _current->removeChangeListener(this);
    _scr_widthN->removeChangeListener(this);
    _scr_heightN->removeChangeListener(this);
    _unitsN->removeChangeListener(this);
    delete _font_renderer;

    deque<Item *>::const_iterator it, end = _items.end();
    for (it = _items.begin(); it != end; ++it)
        delete *it;
}


void HUD::init()
{
    _font_cache = globals->get_fontcache();
    if (!_font)
        _font = _font_cache->getTexFont(fgGetString("/sim/hud/font/name", "Helvetica.txf"));
    if (!_font)
        throw sg_throwable(string("/sim/hud/font/name is not a texture font"));

    _font_size = fgGetFloat("/sim/hud/font/size", 8);
    _font_renderer->setFont(_font);
    _font_renderer->setPointSize(_font_size);
    _text_list.setFont(_font_renderer);

    load(fgGetString("/sim/hud/path[1]", "Huds/default.xml"));
}


void HUD::update(double dt)
{
    _timer += dt;
}


void HUD::draw(osg::State&)
{
    if (!isVisible())
        return;

    if (!_items.size())
        return;

    if (is3D()) {
        draw3D();
        return;
    }

    const float normal_aspect = 640.0f / 480.0f;
    // note: aspect_ratio is Y/X
    float current_aspect = 1.0f / globals->get_current_view()->get_aspect_ratio();
    if (current_aspect > normal_aspect) {
        float aspect_adjust = current_aspect / normal_aspect;
        float adjust = 320.0f * aspect_adjust - 320.0f;
        draw2D(-adjust, 0.0f, 640.0f + adjust, 480.0f);

    } else {
        float aspect_adjust = normal_aspect / current_aspect;
        float adjust = 240.0f * aspect_adjust - 240.0f;
        draw2D(0.0f, -adjust, 640.0f, 480.0f + adjust);
    }

    glViewport(0, 0, _scr_width, _scr_height);
}


void HUD::draw3D()
{
    FGViewer* view = globals->get_current_view();

    // Standard fgfs projection, with essentially meaningless clip
    // planes (we'll map the whole HUD plane to z=-1)
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluPerspective(view->get_v_fov(), 1.0 / view->get_aspect_ratio(), 0.1, 10);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Standard fgfs view direction computation
    float lookat[3];
    lookat[0] = -sin(SG_DEGREES_TO_RADIANS * view->getHeadingOffset_deg());
    lookat[1] = tan(SG_DEGREES_TO_RADIANS * view->getPitchOffset_deg());
    lookat[2] = -cos(SG_DEGREES_TO_RADIANS * view->getHeadingOffset_deg());
    if (fabs(lookat[1]) > 9999)
        lookat[1] = 9999; // FPU sanity
    gluLookAt(0, 0, 0, lookat[0], lookat[1], lookat[2], 0, 1, 0);

    // Map the -1:1 square to a 55.0x41.25 degree wide patch at z=1.
    // This is the default fgfs field of view, which the HUD files are
    // written to assume.
    float dx = 0.52056705; // tan(55/2)
    float dy = dx * 0.75;  // assumes 4:3 aspect ratio
    float m[16];
    m[0] = dx, m[4] =  0, m[ 8] = 0, m[12] = 0;
    m[1] =  0, m[5] = dy, m[ 9] = 0, m[13] = 0;
    m[2] =  0, m[6] =  0, m[10] = 1, m[14] = 0;
    m[3] =  0, m[7] =  0, m[11] = 0, m[15] = 1;
    glMultMatrixf(m);

    // Convert the 640x480 "HUD standard" coordinate space to a square
    // about the origin in the range [-1:1] at depth of -1
    glScalef(1.0 / 320, 1.0 / 240, 1);
    glTranslatef(-320, -240, -1);

    common_draw();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}


void HUD::draw2D(GLfloat x_start, GLfloat y_start, GLfloat x_end, GLfloat y_end)
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(x_start, x_end, y_start, y_end);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    common_draw();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}


void HUD::common_draw()
{
    _text_list.erase();
    _line_list.erase();
    _stipple_line_list.erase();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    glEnable(GL_BLEND);
    if (isTransparent())
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    else
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (isAntialiased()) {
        glEnable(GL_LINE_SMOOTH);
        glAlphaFunc(GL_GREATER, alphaClamp());
        glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
        //glLineWidth(1.5);
    } else {
        //glLineWidth(1.0);
    }

    setColor();
    deque<Item *>::const_iterator it, end = _items.end();
    for (it = _items.begin(); it != end; ++it)
        if ((*it)->isEnabled())
            (*it)->draw();

    _text_list.draw();
    _line_list.draw();

    if (_stipple_line_list.size()) {
        glEnable(GL_LINE_STIPPLE);
        glLineStipple(1, 0x00FF);
        _stipple_line_list.draw();
        glDisable(GL_LINE_STIPPLE);
    }

    if (isAntialiased()) {
        glDisable(GL_ALPHA_TEST);
        glDisable(GL_LINE_SMOOTH);
        //glLineWidth(1.0);
    }

    if (isTransparent())
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}


int HUD::load(const char *file, float x, float y, int level, const string& indent)
{
    const sgDebugPriority TREE = SG_INFO;
    const int MAXNEST = 10;

    SGPath path(globals->get_fg_root());
    path.append(file);

    if (!level) {
        SG_LOG(SG_INPUT, TREE, endl << "load " << file);
        _items.erase(_items.begin(), _items.end());
    } else if (level > MAXNEST) {
        SG_LOG(SG_INPUT, SG_ALERT, "HUD: files nested more than " << MAXNEST << " levels");
        return 0x1;
    } else if (!file || !file[0]) {
        SG_LOG(SG_INPUT, SG_ALERT, "HUD: invalid filename ");
        return 0x2;
    }

    int ret = 0;
    ifstream input(path.c_str());
    if (!input.good()) {
        SG_LOG(SG_INPUT, SG_ALERT, "HUD: Cannot read configuration from " << path.str());
        return 0x4;
    }

    SGPropertyNode root;
    try {
        readProperties(input, &root);
    } catch (const sg_exception &e) {
        input.close();
        guiErrorMessage("HUD: Error ", e);
        return 0x8;
    }

    for (int i = 0; i < root.nChildren(); i++) {
        SGPropertyNode *n = root.getChild(i);
        const char *d = n->getStringValue("name", 0);
        string desc;
        if (d)
            desc = string(": \"") + d + '"';

        const char *name = n->getName();
        if (!strcmp(name, "name")) {
            continue;

        } else if (!strcmp(name, "enable3d")) {
            // set in the tree so that valueChanged() picks it up
            _3DenabledN->setBoolValue(n->getBoolValue());
            continue;

        } else if (!strcmp(name, "import")) {
            const char *fn = n->getStringValue("path", "");
            float xoffs = n->getFloatValue("x-offset", 0.0f);
            float yoffs = n->getFloatValue("y-offset", 0.0f);

            SG_LOG(SG_INPUT, TREE, indent << "|__import " << fn << desc);

            string ind = indent + string(i + 1 < root.nChildren() ? "|    " : "     ");
            ret |= load(fn, x + xoffs, y + yoffs, level + 1, ind);
            continue;
        }

        SG_LOG(SG_INPUT, TREE, indent << "|__" << name << desc);

        Item *item;
        if (!strcmp(name, "label")) {
            item = static_cast<Item *>(new Label(this, n, x, y));
        } else if (!strcmp(name, "gauge")) {
            item = static_cast<Item *>(new Gauge(this, n, x, y));
        } else if (!strcmp(name, "tape")) {
            item = static_cast<Item *>(new Tape(this, n, x, y));
        } else if (!strcmp(name, "dial")) {
            item = static_cast<Item *>(new Dial(this, n, x, y));
        } else if (!strcmp(name, "turn-bank-indicator")) {
            item = static_cast<Item *>(new TurnBankIndicator(this, n, x, y));
        } else if (!strcmp(name, "ladder")) {
            item = static_cast<Item *>(new Ladder(this, n, x, y));
        } else if (!strcmp(name, "runway")) {
            item = static_cast<Item *>(new Runway(this, n, x, y));
        } else if (!strcmp(name, "aiming-reticle")) {
            item = static_cast<Item *>(new AimingReticle(this, n, x, y));
        } else {
            SG_LOG(SG_INPUT, TREE, indent << "      \\...unsupported!");
            continue;
        }
        _items.insert(_items.begin(), item);
    }
    input.close();
    SG_LOG(SG_INPUT, TREE, indent);
    return ret;
}


void HUD::valueChanged(SGPropertyNode *node)
{
    if (!strcmp(node->getName(), "current-color")) {
        int i = node->getIntValue();
        if (i < 0)
            i = 0;
        SGPropertyNode *n = fgGetNode("/sim/hud/palette", true);
        if ((n = n->getChild("color", i, false))) {
            if (n->hasValue("red"))
                _red->setFloatValue(n->getFloatValue("red", 1.0));
            if (n->hasValue("green"))
                _green->setFloatValue(n->getFloatValue("green", 1.0));
            if (n->hasValue("blue"))
                _blue->setFloatValue(n->getFloatValue("blue", 1.0));
            if (n->hasValue("alpha"))
                _alpha->setFloatValue(n->getFloatValue("alpha", 0.67));
            if (n->hasValue("alpha-clamp"))
                _alpha_clamp->setFloatValue(n->getFloatValue("alpha-clamp", 0.01));
            if (n->hasValue("brightness"))
                _brightness->setFloatValue(n->getFloatValue("brightness", 0.75));
            if (n->hasValue("antialiased"))
                _antialiasing->setBoolValue(n->getBoolValue("antialiased", false));
            if (n->hasValue("transparent"))
                _transparency->setBoolValue(n->getBoolValue("transparent", false));
        }
    }
    _scr_width = _scr_widthN->getIntValue();
    _scr_height = _scr_heightN->getIntValue();

    _visible = _visibility->getBoolValue();
    _3Denabled = _3DenabledN->getBoolValue();
    _transparent = _transparency->getBoolValue();
    _antialiased = _antialiasing->getBoolValue();
    float brt = _brightness->getFloatValue();
    _r = clamp(brt * _red->getFloatValue());
    _g = clamp(brt * _green->getFloatValue());
    _b = clamp(brt * _blue->getFloatValue());
    _a = clamp(_alpha->getFloatValue());
    _cl = clamp(_alpha_clamp->getFloatValue());

    _units = strcmp(_unitsN->getStringValue(), "feet") ? METER : FEET;
}


void HUD::setColor() const
{
    if (_antialiased)
        glColor4f(_r, _g, _b, _a);
    else
        glColor3f(_r, _g, _b);
}




void HUD::textAlign(fntRenderer *rend, const char *s, int align,
        float *x, float *y, float *l, float *r, float *t, float *b)
{
    fntFont *font = rend->getFont();
    float gap = font->getGap();
    float left, right, bot, top;
    font->getBBox(s, rend->getPointSize(), rend->getSlant(), &left, &right, &bot, &top);

    if (align & HUD::HCENTER)
        *x -= left - gap + (right - left - gap) / 2.0;
    else if (align & HUD::RIGHT)
        *x -= right;
    else if (align & HUD::LEFT)
        *x -= left;

    if (align & HUD::VCENTER)
        *y -= bot + (top - bot) / 2.0;
    else if (align & HUD::TOP)
        *y -= top;
    else if (align & HUD::BOTTOM)
        *y -= bot;

    *l = *x + left;
    *r = *x + right;
    *b = *y + bot;
    *t = *y + top;
}




// HUDText -- text container for TextList vector


HUDText::HUDText(fntRenderer *fnt, float x, float y, const char *s, int align, int d) :
    _fnt(fnt),
    _x(x),
    _y(y),
    _digits(d)
{
    strncpy(_msg, s, BUFSIZE);
    if (!align || !s[0])
        return;
    float ign;
    HUD::textAlign(fnt, s, align, &_x, &_y, &ign, &ign, &ign, &ign);
}


void HUDText::draw()
{
    if (!_digits) { // show all digits in same size
        _fnt->start2f(_x, _y);
        _fnt->puts(_msg);
        return;
    }

    // FIXME
    // this code is changed to display Numbers with big/small digits
    // according to MIL Standards for example Altitude above 10000 ft
    // is shown as 10ooo.

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

    float orig_size = _fnt->getPointSize();
    if (c > p) {
        _fnt->setPointSize(orig_size * 0.8);
        int p1 = c - 3;
        char *tmp1 = _msg + p1;
        int p2 = p1 * 8;

        _fnt->start2f(_x + p2, _y);
        _fnt->puts(tmp1);

        _fnt->setPointSize(orig_size * 1.2);
        char tmp2[BUFSIZE];
        strncpy(tmp2, _msg, p1);
        tmp2[p1] = '\0';

        _fnt->start2f(_x, _y);
        _fnt->puts(tmp2);
    } else {
        _fnt->setPointSize(orig_size * 1.2);
        _fnt->start2f(_x, _y);
        _fnt->puts(tmp);
    }
    _fnt->setPointSize(orig_size);
}


void TextList::align(const char *s, int align, float *x, float *y,
        float *l, float *r, float *b, float *t) const
{
    HUD::textAlign(_font, s, align, x, y, l, r, b, t);
}


void TextList::draw()
{
    assert(_font);

    // FIXME
    glPushAttrib(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);

    _font->begin();
    vector<HUDText>::iterator it, end = _list.end();
    for (it = _list.begin(); it != end; ++it)
        it->draw();
    _font->end();

    glDisable(GL_TEXTURE_2D);
    glPopAttrib();
}


