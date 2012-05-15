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
#include <simgear/props/props_io.hxx>
#include <osg/GLU>

#include <plib/fnt.h>

#include <Main/globals.hxx>
#include <Viewer/viewmgr.hxx>
#include <Viewer/viewer.hxx>
#include <GUI/FGFontCache.hxx>

#include "HUD.hxx"

using std::endl;
using std::ifstream;

static float clamp(float f)
{
    return f < 0.0f ? 0.0f : f > 1.0f ? 1.0f : f;
}


HUD::HUD() :
    _currentPath(fgGetNode("/sim/hud/current-path", true)),
    _currentColor(fgGetNode("/sim/hud/current-color", true)),
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
    _loaded(false),
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
    _style(0),
    _listener_active(false),
    _clip_box(0)
{
    SG_LOG(SG_COCKPIT, SG_INFO, "Initializing HUD Instrument");

    SGPropertyNode* hud = fgGetNode("/sim/hud");
    hud->addChangeListener(this);
}


HUD::~HUD()
{
    SGPropertyNode* hud = fgGetNode("/sim/hud");
    hud->removeChangeListener(this);

    deinit();
}


void HUD::init()
{
    const char* fontName = 0;
    _font_cache = globals->get_fontcache();
    if (!_font) {
        fontName = fgGetString("/sim/hud/font/name", "Helvetica.txf");
        _font = _font_cache->getTexFont(fontName);
    }
    if (!_font)
        throw sg_io_exception("/sim/hud/font/name is not a texture font",
                              sg_location(fontName));

    _font_size = fgGetFloat("/sim/hud/font/size", 8);
    _font_renderer->setFont(_font);
    _font_renderer->setPointSize(_font_size);
    _text_list.setFont(_font_renderer);
    _loaded = false;
  
    currentColorChanged();
    _currentPath->fireValueChanged();
}

void HUD::deinit()
{
  deque<Item *>::const_iterator it, end = _items.end();
    for (it = _items.begin(); it != end; ++it)
        delete *it;
    end = _ladders.end();
    for (it = _ladders.begin(); it != end; ++it)
        delete *it;
        
  _items.clear();
  _ladders.clear();
  
  delete _clip_box;
  _clip_box = NULL;
  
  _loaded = false;
}

void HUD::reinit()
{
    deinit();
    _currentPath->fireValueChanged();
}

void HUD::update(double dt)
{
    _timer += dt;
}


void HUD::draw(osg::State&)
{
    if (!isVisible())
        return;

    if (!_items.size() && !_ladders.size())
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
    using namespace osg;
    FGViewer* view = globals->get_current_view();

    // Standard fgfs projection, with essentially meaningless clip
    // planes (we'll map the whole HUD plane to z=-1)
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    Matrixf proj
        = Matrixf::perspective(view->get_v_fov(), 1/view->get_aspect_ratio(),
                               0.1, 10);
    glLoadMatrix(proj.ptr());

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    // Standard fgfs view direction computation
    Vec3f lookat;
    lookat[0] = -sin(SG_DEGREES_TO_RADIANS * view->getHeadingOffset_deg());
    lookat[1] = tan(SG_DEGREES_TO_RADIANS * view->getPitchOffset_deg());
    lookat[2] = -cos(SG_DEGREES_TO_RADIANS * view->getHeadingOffset_deg());
    if (fabs(lookat[1]) > 9999)
        lookat[1] = 9999; // FPU sanity
    Matrixf mv = Matrixf::lookAt(Vec3f(0.0, 0.0, 0.0), lookat,
                                 Vec3f(0.0, 1.0, 0.0));
    glLoadMatrix(mv.ptr());

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
    using namespace osg;
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    Matrixf proj = Matrixf::ortho2D(x_start, x_end, y_start, y_end);
    glLoadMatrix(proj.ptr());

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
    _clip_box->set();

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

    // ladders last, as they can have their own clip planes
    end = _ladders.end();
    for (it = _ladders.begin(); it != end; ++it)
        if ((*it)->isEnabled())
            (*it)->draw();

    _clip_box->unset();

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

    SGPath path(globals->resolve_maybe_aircraft_path(file));
    if (path.isNull())
    {
        SG_LOG(SG_INPUT, SG_ALERT, "HUD: Cannot find configuration file '" << file << "'.");
        return 0x2;
    }

    if (!level) {
        SG_LOG(SG_INPUT, TREE, endl << "load " << file);
        _items.erase(_items.begin(), _items.end());
        _ladders.erase(_ladders.begin(), _ladders.end());
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
        SG_LOG(SG_INPUT, SG_ALERT, "HUD: Cannot read configuration from '" << path.c_str() << "'");
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

    delete _clip_box;
    _clip_box = new ClipBox(fgGetNode("/sim/hud/clipping"), x, y);

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
            _ladders.insert(_ladders.begin(), item);
            continue;
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
    if (_listener_active)
        return;
    _listener_active = true;
  
    bool loadNow = false;
    _visible = _visibility->getBoolValue();
    if (_visible && !_loaded) {
      loadNow = true;
    }
  
    if (!strcmp(node->getName(), "current-path") && _visible) {
      loadNow = true;
    }
  
    if (loadNow) {
      int pathIndex = _currentPath->getIntValue();
      SGPropertyNode* pathNode = fgGetNode("/sim/hud/path", pathIndex);
      std::string path("Huds/default.xml");
      if (pathNode && pathNode->hasValue()) {
        path = pathNode->getStringValue();
        SG_LOG(SG_INSTR, SG_INFO, "will load Hud from " << path);
      }
      
      _loaded = true;
      load(path.c_str());
    }
  
    if (!strcmp(node->getName(), "current-color")) {
        currentColorChanged();
    }
    
    _scr_width = _scr_widthN->getIntValue();
    _scr_height = _scr_heightN->getIntValue();

    
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
    _listener_active = false;
}

void HUD::currentColorChanged()
{
  SGPropertyNode *n = fgGetNode("/sim/hud/palette", true);
  int index = _currentColor->getIntValue();
  if (index < 0) {
    index = 0;
  }
  
  n = n->getChild("color", index, false);
  if (!n) {
    return;
  }
  
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

void HUD::setColor() const
{
    if (_antialiased)
        glColor4f(_r, _g, _b, _a);
    else
        glColor3f(_r, _g, _b);
}


void HUD::textAlign(fntRenderer *rend, const char *s, int align,
        float *x, float *y, float *l, float *r, float *b, float *t)
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


ClipBox::ClipBox(const SGPropertyNode *n, float xoffset, float yoffset) :
    _active(false),
    _xoffs(xoffset),
    _yoffs(yoffset)
{
    if (!n)
        return;

    // const_cast is necessary because ATM there's no matching getChild(const ...)
    // prototype and getNode(const ..., <bool>) is wrongly interpreted as
    // getNode(const ..., <int>)
    _top_node = (const_cast<SGPropertyNode *>(n))->getChild("top", 0, true);
    _bot_node = (const_cast<SGPropertyNode *>(n))->getChild("bottom", 0, true);
    _left_node = (const_cast<SGPropertyNode *>(n))->getChild("left", 0, true);
    _right_node = (const_cast<SGPropertyNode *>(n))->getChild("right", 0, true);

    _left[0] = 1.0, _left[1] = _left[2] = 0.0;
    _right[0] = -1.0, _right[1] = _right[2] = 0.0;
    _top[0] = 0.0, _top[1] = -1.0, _top[2] = 0.0;
    _bot[0] = 0.0, _bot[1] = 1.0, _bot[2] = 0.0;
    _active = true;
}


void ClipBox::set()
{
    if (!_active)
        return;

    _left[3] = -_left_node->getDoubleValue() - _xoffs;
    _right[3] = _right_node->getDoubleValue() + _xoffs;
    _bot[3] = -_bot_node->getDoubleValue() - _yoffs;
    _top[3] = _top_node->getDoubleValue() + _yoffs;

    glClipPlane(GL_CLIP_PLANE0, _top);
    glEnable(GL_CLIP_PLANE0);
    glClipPlane(GL_CLIP_PLANE1, _bot);
    glEnable(GL_CLIP_PLANE1);
    glClipPlane(GL_CLIP_PLANE2, _left);
    glEnable(GL_CLIP_PLANE2);
    glClipPlane(GL_CLIP_PLANE3, _right);
    glEnable(GL_CLIP_PLANE3);
}


void ClipBox::unset()
{
    if (_active) {
        glDisable(GL_CLIP_PLANE0);
        glDisable(GL_CLIP_PLANE1);
        glDisable(GL_CLIP_PLANE2);
        glDisable(GL_CLIP_PLANE3);
    }
}
