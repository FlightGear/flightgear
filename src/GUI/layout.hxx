#ifndef __LAYOUT_HXX
#define __LAYOUT_HXX

#include <simgear/props/props.hxx>

class puFont;

// For the purposes of doing layout management, widgets have a type,
// zero or more children, and string-indexed "fields" which can be
// constraints, parameters or x/y/width/height geometry values.  It
// can provide a "preferred" width and height to its parent, and is
// capable of laying itself out into a specified x/y/w/h box.  The
// widget "type" is not a field for historical reasons having to do
// with the way the dialog property format works.
//
// Note that this is a simple wrapper around an SGPropertyNode
// pointer.  The intent is that these objects will be created on the
// stack as needed and passed by value.  All persistent data is stored
// in the wrapped properties.
class LayoutWidget {
public:
    static void setDefaultFont(puFont* font, int pixels);

    LayoutWidget() { _prop = 0; }
    LayoutWidget(SGPropertyNode* p) { _prop = p; }

    const char*  type();
    bool         hasParent();
    LayoutWidget parent();
    int          nChildren();
    LayoutWidget getChild(int i);
    bool         hasField(const char* f);
    int          getNum(const char* f);
    bool         getBool(const char* f, bool dflt = false);
    const char*  getStr(const char* f);
    void         setNum(const char* f, int num);

    void calcPrefSize(int* w, int* h);
    void layout(int x, int y, int w, int h);

private:
    static int UNIT;
    static puFont FONT;

    static bool eq(const char* a, const char* b);
    bool isType(const char* t) { return eq(t, type()); }

    int padding();
    int stringLength(const char* s); // must handle null argument

    void doHVBox(bool doLayout, bool vertical, int* w=0, int* h=0);
    void doTable(bool doLayout, int* w=0, int* h=0);

    SGPropertyNode_ptr _prop;
};

#endif // __LAYOUT_HXX
