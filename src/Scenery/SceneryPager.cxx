// SceneryPager.cxx -- Interface to OSG database pager
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "SceneryPager.hxx"
#include <algorithm>
#include <functional>

using namespace osg;
using namespace flightgear;

SceneryPager::SceneryPager()
{
    _pagerRequests.reserve(48);
    _deleteRequests.reserve(16);
}

SceneryPager::SceneryPager(const SceneryPager& rhs) :
    DatabasePager(rhs)
{
}

SceneryPager::~SceneryPager()
{
}

void SceneryPager::queueRequest(const std::string& fileName, Group* group,
                                float priority, FrameStamp* frameStamp,
                                ref_ptr<Referenced>& databaseRequest,
                                osgDB::ReaderWriter::Options* options)
{
    _pagerRequests.push_back(PagerRequest(fileName, group, priority,
                                          frameStamp,
                                          databaseRequest,
                                          options));
}

void SceneryPager::queueDeleteRequest(osg::ref_ptr<osg::Object>& objptr)
{
    _deleteRequests.push_back(objptr);
    objptr = 0;
}

// Work around interface change in
// osgDB::DatabasePager::requestNodeFile
namespace
{
struct NodePathProxy
{
    NodePathProxy(NodePath& nodePath)
        : _nodePath(nodePath)
    {
    }
    operator Group* () { return static_cast<Group*>(_nodePath.back()); }
    operator NodePath& () { return _nodePath; }
    NodePath& _nodePath;
};
}

void SceneryPager::PagerRequest::doRequest(SceneryPager* pager)
{
    if (_group->getNumChildren() == 0) {
        NodePath path;
        path.push_back(_group.get());
        pager->requestNodeFile(_fileName, NodePathProxy(path), _priority,
                               _frameStamp.get(),
                               *_databaseRequest,
                               _options.get());
    }
}

void SceneryPager::signalEndFrame()
{
    using namespace std;
    bool areDeleteRequests = false;
    bool arePagerRequests = false;
    if (!_deleteRequests.empty()) {
        areDeleteRequests = true;
        OpenThreads::ScopedLock<OpenThreads::Mutex>
            lock(_fileRequestQueue->_childrenToDeleteListMutex);
        ObjectList& deleteList = _fileRequestQueue->_childrenToDeleteList;
        deleteList.insert(deleteList.end(),
                          _deleteRequests.begin(),
                          _deleteRequests.end());
        _deleteRequests.clear();
    }
    if (!_pagerRequests.empty()) {
        arePagerRequests = true;
        for_each(_pagerRequests.begin(), _pagerRequests.end(),
                 bind2nd(mem_fun_ref(&PagerRequest::doRequest), this));
        _pagerRequests.clear();
    }
    if (areDeleteRequests && !arePagerRequests) {
        _fileRequestQueue->updateBlock();
    }
    DatabasePager::signalEndFrame();
}

