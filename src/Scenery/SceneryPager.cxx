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

