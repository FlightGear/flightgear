// RenderArea2D.cxx - a class to manage 2D polygon-based drawing
//                    for a complex instrument (eg. GPS).
//
// Written by David Luff, started 2005.
//
// Copyright (C) 2005 - David C Luff - daveluff AT ntlworld.com
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
//
// $Id$


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "render_area_2d.hxx"

RA2DPrimitive::RA2DPrimitive() {
    invert = false;
    debug = false;
}
                                         
RenderArea2D::RenderArea2D(int logx, int logy, int sizex, int sizey, int posx, int posy) {
    _logx = logx;
    _logy = logy;
    _sizex = sizex;
    _sizey = sizey;
    _posx = posx;
    _posy = posy;
    _clipx1 = 0;
    _clipx2 = _logx - 1;
    _clipy1 = 0;
    _clipy2 = _logy - 1;
    
    // Default to black background / white text.
    _backgroundColor[0] = 0.0;
    _backgroundColor[1] = 0.0;
    _backgroundColor[2] = 0.0;
    _backgroundColor[3] = 1.0;
    _pixelColor[0] = 1.0;
    _pixelColor[1] = 1.0;
    _pixelColor[2] = 1.0;
    _pixelColor[3] = 1.0;
    
    _ra2d_debug = false;
}

void RenderArea2D::Draw(osg::State& state) {
    
    static osg::ref_ptr<osg::StateSet> renderArea2DStateSet;
    if(!renderArea2DStateSet.valid()) {
        renderArea2DStateSet = new osg::StateSet;
        renderArea2DStateSet->setTextureMode(0, GL_TEXTURE_2D, osg::StateAttribute::OFF);
        renderArea2DStateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    }
    
    state.pushStateSet(renderArea2DStateSet.get());
    state.apply();
    state.setActiveTextureUnit(0);
    state.setClientActiveTextureUnit(0);
    
    // DCL - the 2 lines below are copied verbatim from the hotspot drawing code.
    // I am not sure if they are needed here or not.
    glPushAttrib(GL_ENABLE_BIT);
    glDisable(GL_COLOR_MATERIAL);
    
    // FIXME - disabling all clip planes causes bleed-through through the splash screen.
    glDisable(GL_CLIP_PLANE0);
    glDisable(GL_CLIP_PLANE1);
    glDisable(GL_CLIP_PLANE2);
    glDisable(GL_CLIP_PLANE3);

    DoDrawBackground();
    
    for(unsigned int i = 0; i < drawing_list.size(); ++i) {
        RA2DPrimitive prim = drawing_list[i];
        switch(prim.type) {
        case RA2D_LINE:
            DoDrawLine(prim.x1, prim.y1, prim.x2, prim.y2);
            break;
        case RA2D_QUAD:
            if(prim.debug) {
                //cout << "Clipping = " << _clipx1 << ", " << _clipy1 << " to " << _clipx2 << ", " << _clipy2 << '\n';
                //cout << "Drawing quad " << prim.x1 << ", " << prim.y1 << " to " << prim.x2 << ", " << prim.y2 << '\n';
            }
            DoDrawQuad(prim.x1, prim.y1, prim.x2, prim.y2, prim.invert);
            break;
        case RA2D_PIXEL:
            DoDrawPixel(prim.x1, prim.y1, prim.invert);
            break;
        }
    }
    
    drawing_list.clear();
    
    glPopAttrib();
    
    state.popStateSet();
    state.apply();
    state.setActiveTextureUnit(0);
    state.setClientActiveTextureUnit(0);
}

void RenderArea2D::Flush() {
    drawing_list.clear();
}

void RenderArea2D::SetPixelColor(const float* rgba) {
    _pixelColor[0] = rgba[0];
    _pixelColor[1] = rgba[1];
    _pixelColor[2] = rgba[2];
    _pixelColor[3] = rgba[3];
}

// Set clipping region in logical units
void RenderArea2D::SetClipRegion(int x1, int y1, int x2, int y2) {
    _clipx1 = x1;
    _clipx2 = x2;
    _clipy1 = y1;
    _clipy2 = y2;
    //cout << "Set clip region, clip region = "  << _clipx1 << ", " << _clipy1 << " to " << _clipx2 << ", " << _clipy2 << '\n';
}

// Set clip region to be the same as the rendered area (default)
void RenderArea2D::ResetClipRegion() {
    _clipx1 = 0;
    _clipx2 = _logx - 1;
    _clipy1 = 0;
    _clipy2 = _logy - 1;
    //cout << "Reset clip region, clip region = "  << _clipx1 << ", " << _clipy1 << " to " << _clipx2 << ", " << _clipy2 << '\n';
}

void RenderArea2D::SetPosition(int posx, int posy) {
    _posx = posx;
    _posy = posy;
}

void RenderArea2D::SetLogicalSize(int logx, int logy) {
    _logx = logx;
    _logy = logy;
}

void RenderArea2D::SetActualSize(int sizex, int sizey) {
    _sizex = sizex;
    _sizey = sizey;
}

void RenderArea2D::DrawPixel(int x, int y, bool invert) {
    // Clip
    if(x < _clipx1 || x > _clipx2 || y < _clipy1 || y > _clipy2) return;

    RA2DPrimitive prim;
    prim.x1 = x;
    prim.y1 = y;
    prim.x2 = 0;
    prim.y2 = 0;
    prim.type = RA2D_PIXEL;
    prim.invert = invert;
    drawing_list.push_back(prim);
}

void RenderArea2D::DoDrawPixel(int x, int y, bool invert) {
    // Clip.  In theory this shouldn't be necessary, since all input is clipped before adding
    // to the drawing list, but it ensures that any errors in clipping lines etc will only 
    // spill over the clip area within the instrument, and still be clipped from straying
    // outside the instrument.
    if(x < _clipx1 || x > _clipx2 || y < _clipy1 || y > _clipy2) return;
    
    // Scale to position within background
    float fx1 = (float)x, fy1 = (float)y;
    float rx = (float)_sizex / (float)_logx;
    float ry = (float)_sizey / (float)_logy;
    fx1 *= rx;
    fy1 *= ry;
    float fx2 = fx1 + rx;
    float fy2 = fy1 + ry;
    
    // Translate to final position
    fx1 += (float)_posx;
    fx2 += (float)_posx;
    fy1 += (float)_posy;
    fy2 += (float)_posy;
    
    //cout << "DP: " << fx1 << ", " << fy1 << " ... " << fx2 << ", " << fy2 << '\n';
    
    SetRenderColor(invert ? _backgroundColor : _pixelColor);
    SGVec2f corners[4] = {
        SGVec2f(fx1, fy1),
        SGVec2f(fx2, fy1),
        SGVec2f(fx2, fy2),
        SGVec2f(fx1, fy2)
    };
    RenderQuad(corners);
}

void RenderArea2D::DrawLine(int x1, int y1, int x2, int y2) {
    // We need to clip the line to the current region before storing it in the drawing 
    // list, since when we come to actually draw it the clip region may have changed.

    // Liang-Barsky clipping algorithm
    int p[4], q[4];
    float u1 = 0.0f, u2 = 1.0f;
    p[0] = -(x2 - x1); q[0] = (x1 - _clipx1);
    p[1] =  (x2 - x1); q[1] = (_clipx2 - x1);
    p[2] = -(y2 - y1); q[2] = (y1 - _clipy1);
    p[3] =  (y2 - y1); q[3] = (_clipy2 - y1);
    
    for(int i=0; i<4; ++i) {
        if(p[i] == 0) {
            if(q[i] < 0) {
                // Then we have a trivial rejection of a line parallel to a clip plane
                // completely outside the clip region.
                return;
            }
        } else if(p[i] < 0) {
            float r = (float)q[i]/(float)p[i];
            u1 = (u1 > r ? u1 : r);
        } else {        // p[i] > 0
            float r = (float)q[i]/(float)p[i];
            u2 = (u2 < r ? u2 : r);
        }
        if(u1 > u2) {
            // Then the line is completely outside the clip area.
            return;
        }
    }
    
    float fx1 = x1 + u1 * (float)(x2 - x1);
    float fy1 = y1 + u1 * (float)(y2 - y1);
    float fx2 = x1 + u2 * (float)(x2 - x1);
    float fy2 = y1 + u2 * (float)(y2 - y1);
    x1 = (int)(fx1 + 0.5);
    y1 = (int)(fy1 + 0.5);
    x2 = (int)(fx2 + 0.5);
    y2 = (int)(fy2 + 0.5);
    
    RA2DPrimitive prim;
    prim.x1 = x1;
    prim.y1 = y1;
    prim.x2 = x2;
    prim.y2 = y2;
    prim.type = RA2D_LINE;
    prim.invert = false;
    drawing_list.push_back(prim);
}

void RenderArea2D::DoDrawLine(int x1, int y1, int x2, int y2) {
    // Crude implementation of Bresenham line drawing algorithm.
    
    // Our lines are non directional, so first order the points x-direction-wise to leave only 4 octants to consider.
    if(x2 < x1) {
        int tmp_x = x1;
        int tmp_y = y1;
        x1 = x2;
        y1 = y2;
        x2 = tmp_x;
        y2 = tmp_y;
    }
    
    bool flip_y = (y1 > y2 ? true : false);
    int dx = x2 - x1;
    int dy = (flip_y ? y1 - y2 : y2 - y1); 
    if(dx > dy) {
        // push the x dir
        int y = y1;
        int yn = dx/2;
        for(int x=x1; x<=x2; ++x) {
            DoDrawPixel(x, y);
            yn += dy;
            if(yn >= dx) {
                yn -= dx;
                y = (flip_y ? y - 1 : y + 1);
            }
        }
    } else {
        // push the y dir
        int x = x1;
        int xn = dy/2;
        // Must be a more elegant way to roll the next two cases into one!
        if(flip_y) {
            for(int y=y1; y>=y2; --y) {
                DoDrawPixel(x, y);
                xn += dx;
                if(xn >= dy) {
                    xn -= dy;
                    x++;
                }
            }
        } else {
            for(int y=y1; y<=y2; ++y) {
                DoDrawPixel(x, y);
                xn += dx;
                if(xn >= dy) {
                    xn -= dy;
                    x++;
                }
            }
        }
    }
}

void RenderArea2D::DrawQuad(int x1, int y1, int x2, int y2, bool invert) {
    // Force the input to be ordered with x1 < x2 and y1 < y2.
    if(x1 > x2) {
        int x = x2;
        x2 = x1;
        x1 = x;
    }
    if(y1 > y2) {
        int y = y2;
        y2 = y1;
        y1 = y;
    }
    
    // Clip the input to the current drawing region.
    x1 = x1 < _clipx1 ? _clipx1 : x1;
    if(x1 > _clipx2) { return; }
    x2 = x2 > _clipx2 ? _clipx2 : x2;
    if(x2 < _clipx1) { return; }
    y1 = y1 < _clipy1 ? _clipy1 : y1;
    if(y1 > _clipy2) { return; }
    y2 = y2 > _clipy2 ? _clipy2 : y2;
    if(y2 < _clipy1) { return; }
    
    RA2DPrimitive prim;
    prim.x1 = x1;
    prim.y1 = y1;
    prim.x2 = x2;
    prim.y2 = y2;
    prim.type = RA2D_QUAD;
    prim.invert = invert;
    if(_ra2d_debug) prim.debug = true;
    drawing_list.push_back(prim);
}

void RenderArea2D::DoDrawQuad(int x1, int y1, int x2, int y2, bool invert) {
    // Scale to position within background
    float fx1 = (float)x1, fy1 = (float)y1;
    float fx2 = (float)x2, fy2 = (float)y2;
    float rx = (float)_sizex / (float)_logx;
    float ry = (float)_sizey / (float)_logy;
    fx1 *= rx;
    fy1 *= ry;
    fx2 *= rx;
    fy2 *= ry;
    
    fx2 += rx;
    fy2 += ry;
    
    // Translate to final position
    fx1 += (float)_posx;
    fx2 += (float)_posx;
    fy1 += (float)_posy;
    fy2 += (float)_posy;
    
    //cout << "DP: " << fx1 << ", " << fy1 << " ... " << fx2 << ", " << fy2 << '\n';
    
    SetRenderColor(invert ? _backgroundColor : _pixelColor);
    SGVec2f corners[4] = {
        SGVec2f(fx1, fy1),
        SGVec2f(fx2, fy1),
        SGVec2f(fx2, fy2),
        SGVec2f(fx1, fy2)
    };
    RenderQuad(corners);
}

void RenderArea2D::DrawBackground() {
    // Currently a NO-OP
}

void RenderArea2D::DoDrawBackground() {
    SetRenderColor(_backgroundColor);
    SGVec2f corners[4] = {
        SGVec2f(_posx, _posy),
        SGVec2f(_posx + _sizex, _posy),
        SGVec2f(_posx + _sizex, _posy + _sizey),
        SGVec2f(_posx, _posy + _sizey)                                                                  
    };
  
    RenderQuad(corners);
}

// -----------------------------------------
//
// Actual drawing routines copied from Atlas
//
// -----------------------------------------

void RenderArea2D::SetRenderColor( const float *rgba ) {
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, rgba);
    glColor4fv( rgba );
}

void RenderArea2D::RenderQuad( const SGVec2f *p) {
    glBegin(GL_QUADS);
        glNormal3f(0.0f, 0.0f, 0.0f);
        glVertex2fv( p[0].data() );
        glVertex2fv( p[1].data() );
        glVertex2fv( p[2].data() );
        glVertex2fv( p[3].data() );
    glEnd();
}

void RenderArea2D::RenderQuad( const SGVec2f *p, const SGVec4f *color ) {
    glBegin(GL_QUADS);
        glNormal3f(0.0f, 0.0f, 0.0f);
        glColor4fv( color[0].data() ); glVertex2fv( p[0].data() );
        glColor4fv( color[1].data() ); glVertex2fv( p[1].data() );
        glColor4fv( color[2].data() ); glVertex2fv( p[2].data() );
        glColor4fv( color[3].data() ); glVertex2fv( p[3].data() );
    glEnd();
}
