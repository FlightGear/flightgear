// HUD_label.cxx -- HUD Label
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

#include <simgear/props/condition.hxx>
#include "HUD.hxx"


HUD::Label::Label(HUD *hud, const SGPropertyNode *n, float x, float y) :
    Item(hud, n, x, y),
    _input(n->getNode("input", false)),
    _fontsize(fgGetInt("/sim/startup/xsize") > 1000 ? FONT_LARGE : FONT_SMALL),		// FIXME
    _box(n->getBoolValue("box", false)),
    _blink_condition(0),
    _blink_interval(n->getFloatValue("blinking/interval", -1.0f)),
    _blink_target(0.0),
    _blink_state(true)
{
    const SGPropertyNode *node = n->getNode("blinking/condition");
    if (node)
       _blink_condition = sgReadCondition(globals->get_props(), node);

    const char *halign = n->getStringValue("halign", "center");
    if (!strcmp(halign, "left"))
        _halign = LEFT_ALIGN;
    else if (!strcmp(halign, "right"))
        _halign = RIGHT_ALIGN;
    else
        _halign = CENTER_ALIGN;

    const char *pre = n->getStringValue("prefix", 0);
    const char *post = n->getStringValue("postfix", 0);
    const char *fmt = n->getStringValue("format", 0);

    if (pre)
        _format = pre;
    if (fmt)
        _format += fmt;
    else
        _format += "%s";
    if (post)
        _format += post;

    _mode = check_format(_format.c_str());
    if (_mode == INVALID) {
        SG_LOG(SG_INPUT, SG_ALERT, "HUD: invalid format '" << _format.c_str() << '\'');
        _format = "INVALID";
        _mode = NONE;
    }

    blink();
}


void HUD::Label::draw(void)
{
    if (!(_mode == NONE || _input.isValid() && blink()))
        return;

    Rect scrn_rect = get_location();

    if (_box) {
        float x = scrn_rect.left;
        float y = scrn_rect.top;
        float w = scrn_rect.right;
        float h = _hud->_font_size;			// FIXME

        glPushMatrix();
        glLoadIdentity();

        glBegin(GL_LINES);
        glVertex2f(x - 2.0,  y - 2.0);
        glVertex2f(x + w + 2.0, y - 2.0);
        glVertex2f(x + w + 2.0, y + h + 2.0);
        glVertex2f(x - 2.0,  y + h + 2.0);
        glEnd();

        glEnable(GL_LINE_STIPPLE);
        glLineStipple(1, 0xAAAA);

        glBegin(GL_LINES);
        glVertex2f(x + w + 2.0, y - 2.0);
        glVertex2f(x + w + 2.0, y + h + 2.0);
        glVertex2f(x - 2.0, y + h + 2.0);
        glVertex2f(x - 2.0, y - 2.0);
        glEnd();

        glDisable(GL_LINE_STIPPLE);
        glPopMatrix();
    }

    const int BUFSIZE = 256;
    char buf[BUFSIZE];
    if (_mode == NONE)
        snprintf(buf, BUFSIZE, _format.c_str());
    else if (_mode == STRING)
        snprintf(buf, BUFSIZE, _format.c_str(), _input.getStringValue());
    else if (_mode == INT)
        snprintf(buf, BUFSIZE, _format.c_str(), int(_input.getFloatValue()));
    else if (_mode == LONG)
        snprintf(buf, BUFSIZE, _format.c_str(), long(_input.getFloatValue()));
    else if (_mode == FLOAT)
        snprintf(buf, BUFSIZE, _format.c_str(), float(_input.getFloatValue()));
    else if (_mode == DOUBLE) // not really supported yet
        snprintf(buf, BUFSIZE, _format.c_str(), double(_input.getFloatValue()));

    float posincr;
    float lenstr = text_width(buf);

    if (_halign == RIGHT_ALIGN)
        posincr = scrn_rect.right - lenstr;
    else if (_halign == CENTER_ALIGN)
        posincr = get_span() - (lenstr / 2);		// FIXME get_span() ? really?
    else // LEFT_ALIGN
        posincr = 0;

    if (_fontsize == FONT_SMALL)
        draw_text(scrn_rect.left + posincr, scrn_rect.top, buf, get_digits());
    else if (_fontsize == FONT_LARGE)
        draw_text(scrn_rect.left + posincr, scrn_rect.top, buf, get_digits());
}


// make sure the format matches '[ -+#]?\d*(\.\d*)?(l?[df]|s)'
//
HUD::Label::Format HUD::Label::check_format(const char *f) const
{
    bool l = false;
    Format fmt = STRING;

    for (; *f; f++) {
        if (*f == '%') {
            if (f[1] == '%')
                f++;
            else
                break;
        }
    }
    if (*f++ != '%')
        return NONE;
    if (*f == ' ' || *f == '+' || *f == '-' || *f == '#')
        f++;
    while (*f && isdigit(*f))
        f++;
    if (*f == '.') {
        f++;
        while (*f && isdigit(*f))
            f++;
    }
    if (*f == 'l')
        l = true, f++;

    if (*f == 'd')
        fmt = l ? LONG : INT;
    if (*f == 'f')
        fmt = l ? DOUBLE : FLOAT;
    else if (*f == 's') {
        if (l)
            return INVALID;
        fmt = STRING;
    } else
        return INVALID;

    for (++f; *f; f++) {
        if (*f == '%') {
            if (f[1] == '%')
                f++;
            else
                return INVALID;
        }
    }
    return fmt;
}


bool HUD::Label::blink()
{
    if (_blink_interval < 0.0f)
        return true;

    if (_blink_condition && !_blink_condition->test())
        return true;

    if (_hud->timer() < _blink_target)
        return _blink_state;

    _blink_target = _hud->timer() + _blink_interval;
    return _blink_state = !_blink_state;
}


