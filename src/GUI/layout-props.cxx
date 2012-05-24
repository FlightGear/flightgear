#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <plib/pu.h>
#include <simgear/props/props.hxx>

#include "layout.hxx"

// This file contains the code implementing the LayoutWidget class in
// terms of a PropertyNode (plus a tiny bit of PUI glue).  See
// layout.cxx for the actual layout engine.

puFont LayoutWidget::FONT;

void LayoutWidget::setDefaultFont(puFont* font, int pixels)
{
    UNIT = (int)(pixels * (1/3.) + 0.999);
    FONT = *font;
}

int LayoutWidget::stringLength(const char* s)
{
    return (int)(FONT.getFloatStringWidth(s) + 0.999);
}

const char* LayoutWidget::type()
{
    const char* t = _prop->getName();
    return (*t == 0) ? "dialog" : t;
}

bool LayoutWidget::hasParent()
{
    return _prop->getParent() ? true : false;
}

LayoutWidget LayoutWidget::parent()
{
    return LayoutWidget(_prop->getParent());
}

int LayoutWidget::nChildren()
{
    // Hack: assume that any non-leaf nodes but "hrule" and "vrule"
    // are widgets...
    int n = 0;
    for(int i=0; i<_prop->nChildren(); i++) {
        SGPropertyNode* p = _prop->getChild(i);
        const char* name = p->getName();
        if(p->nChildren() != 0 || !strcmp(name, "hrule")
                || !strcmp(name, "vrule"))
            n++;
    }
    return n;
}

LayoutWidget LayoutWidget::getChild(int idx)
{
    // Same hack.  Note that access is linear time in the number of
    // children...
    int n = 0;
    for(int i=0; i<_prop->nChildren(); i++) {
        SGPropertyNode* p = _prop->getChild(i);
        const char* name = p->getName();
        if(p->nChildren() != 0 || !strcmp(name, "hrule")
                || !strcmp(name, "vrule")) {
            if(idx == n) return LayoutWidget(p);
            n++;
        }
    }
    return LayoutWidget(0);
}

bool LayoutWidget::hasField(const char* f)
{
    return _prop->hasChild(f);
}

int LayoutWidget::getNum(const char* f)
{
    return _prop->getIntValue(f);
}

bool LayoutWidget::getBool(const char* f, bool dflt)
{
    return _prop->getBoolValue(f, dflt);
}

const char* LayoutWidget::getStr(const char* f)
{
    return _prop->getStringValue(f);
}

void LayoutWidget::setNum(const char* f, int num)
{
    _prop->setIntValue(f, num);
}

