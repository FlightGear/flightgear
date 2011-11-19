#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "FGColor.hxx"

#include <iostream>

#include <simgear/props/props.hxx>

////////////////////////////////////////////////////////////////////////
// FGColor class.
////////////////////////////////////////////////////////////////////////

void
FGColor::print() const {
    std::cerr << "red=" << _red << ", green=" << _green
              << ", blue=" << _blue << ", alpha=" << _alpha << std::endl;
}

bool
FGColor::merge(const SGPropertyNode *node)
{
    if (!node)
        return false;

    bool dirty = false;
    const SGPropertyNode * n;
    if ((n = node->getNode("red")))
        _red = n->getFloatValue(), dirty = true;
    if ((n = node->getNode("green")))
        _green = n->getFloatValue(), dirty = true;
    if ((n = node->getNode("blue")))
        _blue = n->getFloatValue(), dirty = true;
    if ((n = node->getNode("alpha")))
        _alpha = n->getFloatValue(), dirty = true;
    return dirty;
}

bool
FGColor::merge(const FGColor *color)
{
    bool dirty = false;
    if (color && color->_red >= 0.0)
        _red = color->_red, dirty = true;
    if (color && color->_green >= 0.0)
        _green = color->_green, dirty = true;
    if (color && color->_blue >= 0.0)
        _blue = color->_blue, dirty = true;
    if (color && color->_alpha >= 0.0)
        _alpha = color->_alpha, dirty = true;
    return dirty;
}

