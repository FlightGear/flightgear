// splash.hxx -- draws the initial splash screen
//
// Written by Curtis Olson, started July 1998.  (With a little looking
// at Freidemann's panel code.) :-)
//
// Copyright (C) 1997  Michele F. America  - nomimarketing@mail.telepac.pt
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


#ifndef _SPLASH_HXX
#define _SPLASH_HXX

#include <osg/Group>
#include <osgText/Text>

#include <simgear/props/props.hxx>
#include <simgear/timing/timestamp.hxx>

#include <vector>

namespace osg
{
    class Texture2D;
    class Image;
    class Camera;
}

class SplashScreen : public osg::Group
{
public:
    SplashScreen();
    ~SplashScreen();

    void resize(int width, int height);
    
private:
    friend class SplashScreenUpdateCallback;

    void createNodes();
    void setupLogoImage();

    void doUpdate();
    void updateSplashSpinner();

    std::string selectSplashImage();

    void addText(osg::Geode* geode, const osg::Vec2& pos, double size, const std::string& text,
                 const osgText::Text::AlignmentType alignment,
                 SGPropertyNode* dynamicValue = nullptr,
                 double maxWidthFraction = -1.0,
                 const osg::Vec4& textColor = osg::Vec4(1, 1, 1, 1));

    osg::ref_ptr<osg::Camera> createFBOCamera();
    void manuallyResizeFBO(int width, int height);

    bool _legacySplashScreenMode = false;
    SGPropertyNode_ptr _splashAlphaNode;
    osg::ref_ptr<osg::Camera> _splashFBOCamera;
    double _splashImageAspectRatio; // stores width/height of the splash image we loaded
    osg::Image* _splashImage = nullptr;
    osg::Image* _logoImage = nullptr;
    osg::Vec3Array* _splashImageVertexArray = nullptr;
    osg::Vec3Array* _splashSpinnerVertexArray = nullptr;
    osg::Vec3Array* _aircraftLogoVertexArray = nullptr;
    
    int _width, _height;

    osg::Texture2D* _splashFBOTexture;
    osg::Vec4Array* _splashFSQuadColor;
    osg::ref_ptr<osg::Camera> _splashQuadCamera;

    struct TextItem
    {
        osg::ref_ptr<osgText::Text> textNode;
        SGPropertyNode_ptr dynamicContent;
        osg::Vec2 fractionalPosition; // position in the 0.0 .. 1.0 range
        double fractionalCharSize;
        double maxWidthFraction = -1.0;
        unsigned int maxLineCount = 0;

        void recomputeSize(int height) const;
        void reposition(int width, int height) const;
    };

    std::vector<TextItem> _items;
    SGTimeStamp _splashStartTime;
};

/** Set progress information.
 * "identifier" references an element of the language resource. */
void fgSplashProgress ( const char *identifier, unsigned int percent = 0 );

#endif // _SPLASH_HXX


