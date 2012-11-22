/*
 * FGCanvasSystemAdapter.hxx
 *
 *  Created on: 02.11.2012
 *      Author: tom
 */

#ifndef FG_CANVASSYSTEMADAPTER_HXX_
#define FG_CANVASSYSTEMADAPTER_HXX_

#include <simgear/canvas/CanvasSystemAdapter.hxx>

namespace canvas
{
  class FGCanvasSystemAdapter:
    public simgear::canvas::SystemAdapter
  {
    public:
      virtual simgear::canvas::FontPtr getFont(const std::string& name) const;
      virtual void addCamera(osg::Camera* camera) const;
      virtual void removeCamera(osg::Camera* camera) const;
      virtual osg::Image* getImage(const std::string& path) const;

      virtual int gcSave(naRef r);
      virtual void gcRelease(int key);
  };
}

#endif /* FG_CANVASSYSTEMADAPTER_HXX_ */
