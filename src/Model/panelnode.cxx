#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>
#include <vector>

#include <plib/sg.h>

#include <Cockpit/panel.hxx>
#include <Cockpit/panel_io.hxx>
#include "panelnode.hxx"

SG_USING_STD(vector);


// Static (!) handling for all 3D panels in the program.  Very
// clumsy.  Replace with per-aircraft handling.
vector<FGPanelNode*> all_3d_panels;
bool fgHandle3DPanelMouseEvent( int button, int updown, int x, int y )
{
    for ( unsigned int i = 0; i < all_3d_panels.size(); i++ ) {
        if ( all_3d_panels[i]->doMouseAction(button, updown, x, y) ) {
            return true;
        }
    }
    return false;
}

void fgUpdate3DPanels()
{
    for ( unsigned int i = 0; i < all_3d_panels.size(); i++ ) {
        all_3d_panels[i]->getPanel()->updateMouseDelay();
    }
}

FGPanelNode::FGPanelNode(SGPropertyNode* props)
{
    int i;

    // Make an FGPanel object.  But *don't* call init() or bind() on
    // it -- those methods touch static state.
    _panel = fgReadPanel(props->getStringValue("path"));

    // Never mind.  We *have* to call init to make sure the static
    // state is initialized (it's not, if there aren't any 2D
    // panels).  This is a memory leak and should be fixed!`
    _panel->init();

    _panel->setDepthTest( props->getBoolValue("depth-test") );

    // Initialize the matrices to the identity.  PLib prints warnings
    // when trying to invert singular matrices (e.g. when not using a
    // 3D panel).
    for(i=0; i<4; i++)
        for(int j=0; j<4; j++)
            _lastModelview[4*i+j] = _lastProjection[4*i+j] = i==j ? 1 : 0;

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

    // Now generate our transformation matrix.  For shorthand, use
    // "a", "b", and "c" as our corners and "m" as the matrix. The
    // vector u goes from a to b, v from a to c, and w is a
    // perpendicular cross product.
    float *a = _bottomLeft, *b = _bottomRight, *c = _topLeft, *m = _xform;
    float u[3], v[3], w[3];
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

    // All done.  Add us to the list
    all_3d_panels.push_back(this);
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

    // Grab the matrix state, so that we can get back from screen
    // coordinates to panel coordinates when the user clicks the
    // mouse.
    glGetFloatv(GL_MODELVIEW_MATRIX, _lastModelview);
    glGetFloatv(GL_PROJECTION_MATRIX, _lastProjection);
    glGetIntegerv(GL_VIEWPORT, _lastViewport);

    _panel->draw();


    glPopMatrix();
}

bool FGPanelNode::doMouseAction(int button, int updown, int x, int y)
{
    // Covert the screen coordinates to viewport coordinates in the
    // range [0:1], then transform to OpenGL "post projection" coords
    // in [-1:1].  Remember the difference in Y direction!
    float vx = (x + 0.5 - _lastViewport[0]) / _lastViewport[2];
    float vy = (y + 0.5 - _lastViewport[1]) / _lastViewport[3];
    vx = 2*vx - 1;
    vy = 1 - 2*vy;

    // Make two vectors in post-projection coordinates at the given
    // screen, one in the near field and one in the far field.
    sgVec3 a, b;
    a[0] = b[0] = vx;
    a[1] = b[1] = vy;
    a[2] =  0.75; // "Near" Z value
    b[2] = -0.75; // "Far" Z value

    // Run both vectors "backwards" through the OpenGL matrix
    // transformation.  Remember to w-normalize the vectors!
    sgMat4 m;
    sgMultMat4(m, *(sgMat4*)_lastProjection, *(sgMat4*)_lastModelview);
    sgInvertMat4(m);

    sgFullXformPnt3(a, m);
    sgFullXformPnt3(b, m);

    // And find their intersection on the z=0 plane.  The resulting X
    // and Y coordinates are the hit location in panel coordinates.
    float dxdz = (b[0] - a[0]) / (b[2] - a[2]);
    float dydz = (b[1] - a[1]) / (b[2] - a[2]);
    int panelX = (int)(a[0] - a[2]*dxdz + 0.5);
    int panelY = (int)(a[1] - a[2]*dydz + 0.5);

    return _panel->doLocalMouseAction(button, updown, panelX, panelY);
}

void FGPanelNode::die()
{
    SG_LOG(SG_ALL,SG_ALERT,"Unimplemented function called on FGPanelNode");
    exit(1);
}

