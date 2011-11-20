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
#define FLIGHTGEAR_SCENERYPAGERHXX 1
#include <string>
#include <vector>

#include <osg/FrameStamp>
#include <osg/Group>
#include <osgDB/DatabasePager>

#include <simgear/structure/OSGVersion.hxx>

namespace flightgear
{
class SceneryPager : public osgDB::DatabasePager
{
public:
    SceneryPager();
    SceneryPager(const SceneryPager& rhs);
    // Unhide DatabasePager::requestNodeFile
    using osgDB::DatabasePager::requestNodeFile;
    void queueRequest(const std::string& fileName, osg::Group* node,
                      float priority, osg::FrameStamp* frameStamp,
                      osg::ref_ptr<osg::Referenced>& databaseRequest,
                      osgDB::ReaderWriter::Options* options);
    // This is passed a ref_ptr so that it can "take ownership" of the
    // node to delete and decrement its refcount while holding the
    // lock on the delete list.
    void queueDeleteRequest(osg::ref_ptr<osg::Object>& objptr);
    virtual void signalEndFrame();
protected:
    // Queue up file requests until the end of the frame
    struct PagerRequest
    {
        PagerRequest() : _priority(0.0f), _databaseRequest(0) {}
        PagerRequest(const PagerRequest& rhs) :
            _fileName(rhs._fileName), _group(rhs._group),
            _priority(rhs._priority), _frameStamp(rhs._frameStamp),
            _options(rhs._options), _databaseRequest(rhs._databaseRequest) {}

        PagerRequest(const std::string& fileName, osg::Group* group,
                     float priority, osg::FrameStamp* frameStamp,
                     osg::ref_ptr<Referenced>& databaseRequest,
                     osgDB::ReaderWriter::Options* options):
            _fileName(fileName), _group(group), _priority(priority),
            _frameStamp(frameStamp), _options(options),
            _databaseRequest(&databaseRequest)
        {}

        void doRequest(SceneryPager* pager);
        std::string _fileName;
        osg::ref_ptr<osg::Group> _group;
        float _priority;
        osg::ref_ptr<osg::FrameStamp> _frameStamp;
        osg::ref_ptr<osgDB::ReaderWriter::Options> _options;
        osg::ref_ptr<osg::Referenced>* _databaseRequest;
    };
    typedef std::vector<PagerRequest> PagerRequestList;
    PagerRequestList _pagerRequests;
    typedef std::vector<osg::ref_ptr<osg::Object> > DeleteRequestList;
    DeleteRequestList _deleteRequests;
    virtual ~SceneryPager();
};
}
#endif
