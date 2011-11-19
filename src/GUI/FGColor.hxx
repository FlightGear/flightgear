#ifndef FG_GUI_COLOR_HXX
#define FG_GUI_COLOR_HXX

// forward decls
class SGPropertyNode;

class FGColor {
public:
    FGColor() { clear(); }
    FGColor(float r, float g, float b, float a = 1.0f) { set(r, g, b, a); }
    FGColor(const SGPropertyNode *prop) { set(prop); }
    FGColor(FGColor *c) { 
        if (c) set(c->_red, c->_green, c->_blue, c->_alpha);
    }

    inline void clear() { _red = _green = _blue = _alpha = -1.0f; }
    // merges in non-negative components from property with children <red> etc.
    bool merge(const SGPropertyNode *prop);
    bool merge(const FGColor *color);

    bool set(const SGPropertyNode *prop) { clear(); return merge(prop); };
    bool set(const FGColor *color) { clear(); return merge(color); }
    bool set(float r, float g, float b, float a = 1.0f) {
        _red = r, _green = g, _blue = b, _alpha = a;
        return true;
    }
    bool isValid() const {
        return _red >= 0.0 && _green >= 0.0 && _blue >= 0.0;
    }
    void print() const;

    inline void setRed(float red) { _red = red; }
    inline void setGreen(float green) { _green = green; }
    inline void setBlue(float blue) { _blue = blue; }
    inline void setAlpha(float alpha) { _alpha = alpha; }

    inline float red() const { return clamp(_red); }
    inline float green() const { return clamp(_green); }
    inline float blue() const { return clamp(_blue); }
    inline float alpha() const { return _alpha < 0.0 ? 1.0 : clamp(_alpha); }

protected:
    float _red, _green, _blue, _alpha;

private:
    float clamp(float f) const { return f < 0.0 ? 0.0 : f > 1.0 ? 1.0 : f; }
};

#endif