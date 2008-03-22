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

#include <simgear/scene/model/SGPagedLOD.hxx>
#include "SceneryPager.hxx"
#include <algorithm>
#include <functional>

using namespace flightgear;
using osg::ref_ptr;
using osg::Node;

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

void SceneryPager::requestNodeFile(const std::string& fileName,osg::Group* group,
                                    float priority, const osg::FrameStamp* framestamp)
{
    simgear::SGPagedLOD *sgplod = dynamic_cast<simgear::SGPagedLOD*>(group);
    if(sgplod)
        DatabasePager::requestNodeFile(fileName,group,priority,framestamp,sgplod->getReaderWriterOptions());
    else
        DatabasePager::requestNodeFile(fileName,group,priority,framestamp);
}

void SceneryPager::queueRequest(const std::string& fileName, osg::Group* group,
                                float priority, osg::FrameStamp* frameStamp)
{
    _pagerRequests.push_back(PagerRequest(fileName, group, priority,
                                          frameStamp));
}

void SceneryPager::queueDeleteRequest(osg::ref_ptr<osg::Object>& objptr)
{
    _deleteRequests.push_back(objptr);
    objptr = 0;
}
void SceneryPager::signalEndFrame()
{
    using namespace std;
    bool areDeleteRequests = false;
    bool arePagerRequests = false;
    if (!_deleteRequests.empty()) {
        areDeleteRequests = true;
        OpenThreads::ScopedLock<OpenThreads::Mutex>
            lock(_childrenToDeleteListMutex);
        _childrenToDeleteList.insert(_childrenToDeleteList.end(),
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
    if (areDeleteRequests && !arePagerRequests)
        updateDatabasePagerThreadBlock();
    DatabasePager::signalEndFrame();
}

