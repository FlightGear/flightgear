#include <GL/gl.h>
#include <Main/fg_props.hxx>
#include <Cockpit/panel.hxx>
#include <Cockpit/panel_io.hxx>
#include "panelnode.hxx"

FGPanelNode::FGPanelNode(SGPropertyNode* props)
{
    // Make an FGPanel object.  But *don't* call init() or bind() on
    // it -- those methods touch static state.
    _panel = fgReadPanel(props->getStringValue("path"));

    // Read out the pixel-space info
    _xmax = _panel->getWidth();
    _ymax = _panel->getHeight();

    // And the corner points
    SGPropertyNode* pt = props->getChild("bottom-left");
    _bottomLeft[0] = pt->getFloatValue("x-m");
    _bottomLeft[1] = pt->getFloatValue("y-m");
    _bottomLeft[2] = pt->getFloatValue("z-m");

    pt = props->getChild("top-left");
    _topLeft[0] = pt->getFloatValue("x-m");
    _topLeft[1] = pt->getFloatValue("y-m");
    _topLeft[2] = pt->getFloatValue("z-m");

    pt = props->getChild("bottom-right");
    _bottomRight[0] = pt->getFloatValue("x-m");
    _bottomRight[1] = pt->getFloatValue("y-m");
    _bottomRight[2] = pt->getFloatValue("z-m");

    // Now generate out transformation matrix.  For shorthand, use
    // "a", "b", and "c" as our corners and "m" as the matrix. The
    // vector u goes from a to b, v from a to c, and w is a
    // perpendicular cross product.
    float *a = _bottomLeft, *b = _bottomRight, *c = _topLeft, *m = _xform;
    float u[3], v[3], w[3];
    int i;
    for(i=0; i<3; i++) u[i] = b[i] - a[i]; // U = B - A
    for(i=0; i<3; i++) v[i] = c[i] - a[i]; // V = C - A

    w[0] = u[1]*v[2] - v[1]*u[2];          // W = U x V
    w[1] = u[2]*v[0] - v[2]*u[0];
    w[2] = u[0]*v[1] - v[0]*u[1];

    // Now generate a trivial basis transformation matrix.  If we want
    // to map the three unit vectors to three arbitrary vectors U, V,
    // and W, then those just become the columns of the 3x3 matrix.
    m[0] = u[0]; m[4] = v[0]; m[8]  = w[0]; m[12] = a[0]; //     |Ux Vx Wx|
    m[1] = u[1]; m[5] = v[1]; m[9]  = w[1]; m[13] = a[1]; // m = |Uy Vy Wy|
    m[2] = u[2]; m[6] = v[2]; m[10] = w[2]; m[14] = a[2]; //     |Uz Vz Wz|
    m[3] = 0;    m[7] = 0;    m[11] = 0;    m[15] = 1;

    // The above matrix maps the unit (!) square to the panel
    // rectangle.  Postmultiply scaling factors that match the
    // pixel-space size of the panel.
    for(i=0; i<4; i++) {
        m[0+i] *= 1.0/_xmax;
        m[4+i] *= 1.0/_ymax;
    }

    // Now plib initialization.  The bounding sphere is defined nicely
    // by our corner points:
    float cx = (b[0]+c[0])/2;
    float cy = (b[1]+c[1])/2;
    float cz = (b[2]+c[2])/2;
    float r = sqrt((cx-a[0])*(cx-a[0]) +
                   (cy-a[1])*(cy-a[1]) +
                   (cz-a[2])*(cz-a[2]));
    bsphere.setCenter(cx, cy, cz);
    bsphere.setRadius(r);
}

FGPanelNode::~FGPanelNode()
{
    delete _panel;
}

void FGPanelNode::draw()
{
    // What's the difference?
    draw_geometry();
}

void FGPanelNode::draw_geometry()
{
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glMultMatrixf(_xform);
    _panel->draw();
    glPopMatrix();
}

void FGPanelNode::die()
{
    SG_LOG(SG_ALL,SG_ALERT,"Unimplemented function called on FGPanelNode");
    *(int*)0=0;
}

