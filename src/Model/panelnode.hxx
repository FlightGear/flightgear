#ifndef FG_PANELNODE_HXX
#define FG_PANELNODE_HXX


#include <plib/ssg.h>

class FGPanel;
class SGPropertyNode;

// PanelNode defines an SSG leaf object that draws a FGPanel object
// into the scene graph.  Note that this is an incomplete SSG object,
// many methods, mostly involved with modelling and runtime
// inspection, are unimplemented.

// Static mouse handler for all FGPanelNodes.  Very clumsy; this
// should really be done through our container (an aircraft model,
// typically).
bool fgHandle3DPanelMouseEvent(int button, int updown, int x, int y);
void fgUpdate3DPanels();

class FGPanelNode : public ssgLeaf 
{
protected:
    virtual void draw_geometry();

public:
    FGPanelNode(SGPropertyNode* props);
    virtual ~FGPanelNode();

    virtual void draw();
    bool doMouseAction(int button, int updown, int x, int y);

    FGPanel* getPanel() { return _panel; }

    virtual void recalcBSphere() { bsphere_is_invalid = 0; }

    //
    // A bunch of Plib functions that aren't implemented.  I don't
    // even know what many of them do, but they're pure virtual and
    // require implementation.
    //
    virtual int getNumTriangles() { return 0; }
    virtual void getTriangle(int n, short* v1, short* v2, short* v3) { die(); }
    virtual int getNumLines() { die(); return 0; }
    virtual void getLine(int n, short* v1, short* v2) { die(); }
    
    virtual void drawHighlight(sgVec4 colour) { die(); }
    virtual void drawHighlight(sgVec4 colour, int i) { die(); }
    virtual float* getVertex(int i) { die(); return 0; }
    virtual float* getNormal(int i) { die(); return 0; }
    virtual float* getColour(int i) { die(); return 0; }
    virtual float* getTexCoord(int i) { die(); return 0; }
    virtual void pick(int baseName) { die(); }
    virtual void isect_triangles(sgSphere* s, sgMat4 m, int testNeeded) { die(); }
    virtual void hot_triangles(sgVec3 s, sgMat4 m, int testNeeded) { die(); }
    virtual void los_triangles(sgVec3 s, sgMat4 m, int testNeeded) { die(); }
    virtual void transform(const sgMat4 m) { die(); }

private:
    // Handler for all the unimplemented methods above
    void die();

    FGPanel* _panel;

    // Panel corner coordinates
    float _bottomLeft[3], _topLeft[3], _bottomRight[3];

    // The input range expected in the panel definition.  These x/y
    // coordinates will map to the right/top sides.
    float _xmax, _ymax;
    
    // The matrix that results, which transforms 2D x/y panel
    // coordinates into 3D coordinates of the panel quadrilateral.
    GLfloat _xform[16];

    // The matrix transformation state that was active the last time
    // we were rendered.  Used by the mouse code to compute
    // intersections.
    GLfloat _lastModelview[16];
    GLfloat _lastProjection[16];
    GLint   _lastViewport[4];
};


#endif // FG_PANELNODE_HXX
