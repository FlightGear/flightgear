// SceneryPager.hxx -- Interface to OSG database pager
//
// Copyright (C) 2007 Tim Moore timoore@redhat.com
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

#ifndef FLIGHTGEAR_SCENERYPAGERHXX
#include <string>
#include <vector>

#include <osg/FrameStamp>
#include <osg/Group>
#include <osgDB/DatabasePager>

namespace flightgear
{
class SceneryPager : public osgDB::DatabasePager
{
public:
    SceneryPager();
    SceneryPager(const SceneryPager& rhs);
    void queueRequest(const std::string& fileName, osg::Group* node,
                      float priority, osg::FrameStamp* frameStamp);
    // This is passed a ref_ptr so that it can "take ownership" of the
    // node to delete and decrement its refcount while holding the
    // lock on the delete list.
    void queueDeleteRequest(osg::ref_ptr<osg::Object>& objptr);
    virtual void signalEndFrame();
protected:
    // Queue up file requests until the end of the frame
    struct PagerRequest
    {
        PagerRequest() {}
        PagerRequest(const PagerRequest& rhs) :
            _fileName(rhs._fileName), _group(rhs._group),
            _priority(rhs._priority), _frameStamp(rhs._frameStamp) {}
        PagerRequest(const std::string& fileName, osg::Group* group,
                     float priority, osg::FrameStamp* frameStamp) :
            _fileName(fileName), _group(group), _priority(priority),
            _frameStamp(frameStamp) {}
        void doRequest(SceneryPager* pager)
        {
            pager->requestNodeFile(_fileName, _group.get(), _priority,
                                   _frameStamp.get());
        }
        std::string _fileName;
        osg::ref_ptr<osg::Group> _group;
        float _priority;
        osg::ref_ptr<osg::FrameStamp> _frameStamp;
    };
    typedef std::vector<PagerRequest> PagerRequestList;
    PagerRequestList _pagerRequests;
    typedef std::vector<osg::ref_ptr<osg::Object> > DeleteRequestList;
    DeleteRequestList _deleteRequests;
    virtual ~SceneryPager();
};
}
#define FLIGHTGEAR_SCENERYPAGERHXX 1
#endif
