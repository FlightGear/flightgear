#include "fgfs.hxx"

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/exception.hxx>

#include "globals.hxx"
#include "fg_props.hxx"



////////////////////////////////////////////////////////////////////////
// Implementation of FGSubsystem
////////////////////////////////////////////////////////////////////////


FGSubsystem::FGSubsystem ()
  : _suspended(false)
{
}

FGSubsystem::~FGSubsystem ()
{
}

void
FGSubsystem::init ()
{
}

void
FGSubsystem::reinit ()
{
}

void
FGSubsystem::bind ()
{
}

void
FGSubsystem::unbind ()
{
}

void
FGSubsystem::suspend ()
{
  _suspended = true;
}

void
FGSubsystem::suspend (bool suspended)
{
  _suspended = suspended;
}

void
FGSubsystem::resume ()
{
  _suspended = false;
}

bool
FGSubsystem::is_suspended () const
{
  if (!_freeze_master_node.valid())
    _freeze_master_node = fgGetNode("/sim/freeze/master", true);
  return _suspended || _freeze_master_node->getBoolValue();
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGSubsystemGroup.
////////////////////////////////////////////////////////////////////////

FGSubsystemGroup::FGSubsystemGroup ()
{
}

FGSubsystemGroup::~FGSubsystemGroup ()
{
    for (int i = 0; i < _members.size(); i++)
        delete _members[i];
}

void
FGSubsystemGroup::init ()
{
    for (int i = 0; i < _members.size(); i++)
        _members[i]->subsystem->init();
}

void
FGSubsystemGroup::reinit ()
{
    for (int i = 0; i < _members.size(); i++)
        _members[i]->subsystem->reinit();
}

void
FGSubsystemGroup::bind ()
{
    for (int i = 0; i < _members.size(); i++)
        _members[i]->subsystem->bind();
}

void
FGSubsystemGroup::unbind ()
{
    for (int i = 0; i < _members.size(); i++)
        _members[i]->subsystem->unbind();
}

void
FGSubsystemGroup::update (double delta_time_sec)
{
    for (int i = 0; i < _members.size(); i++)
        _members[i]->update(delta_time_sec); // indirect call
}

void
FGSubsystemGroup::suspend ()
{
    for (int i = 0; i < _members.size(); i++)
        _members[i]->subsystem->suspend();
}

void
FGSubsystemGroup::resume ()
{
    for (int i = 0; i < _members.size(); i++)
        _members[i]->subsystem->resume();
}

bool
FGSubsystemGroup::is_suspended () const
{
    return false;
}

void
FGSubsystemGroup::set_subsystem (const string &name, FGSubsystem * subsystem,
                                 double min_step_sec)
{
    Member * member = get_member(name, true);
    if (member->subsystem != 0)
        delete member->subsystem;
    member->name = name;
    member->subsystem = subsystem;
    member->min_step_sec = min_step_sec;
}

FGSubsystem *
FGSubsystemGroup::get_subsystem (const string &name)
{
    Member * member = get_member(name);
    if (member != 0)
        return member->subsystem;
    else
        return 0;
}

void
FGSubsystemGroup::remove_subsystem (const string &name)
{
    for (int i = 0; i < _members.size(); i++) {
        if (name == _members[i]->name) {
            _members.erase(_members.begin() + i);
            return;
        }
    }
}

bool
FGSubsystemGroup::has_subsystem (const string &name) const
{
    return (((FGSubsystemGroup *)this)->get_member(name) != 0);
}

FGSubsystemGroup::Member *
FGSubsystemGroup::get_member (const string &name, bool create)
{
    for (int i = 0; i < _members.size(); i++) {
        if (_members[i]->name == name)
            return _members[i];
    }
    if (create) {
        Member * member = new Member;
        _members.push_back(member);
        return member;
    } else {
        return 0;
    }
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGSubsystemGroup::Member
////////////////////////////////////////////////////////////////////////


FGSubsystemGroup::Member::Member ()
    : name(""),
      subsystem(0),
      min_step_sec(0),
      elapsed_sec(0)
{
}

FGSubsystemGroup::Member::Member (const Member &)
{
    Member();
}

FGSubsystemGroup::Member::~Member ()
{
                                // FIXME: causes a crash
//     delete subsystem;
}

void
FGSubsystemGroup::Member::update (double delta_time_sec)
{
    elapsed_sec += delta_time_sec;
    if (elapsed_sec >= min_step_sec) {
        if (!subsystem->is_suspended()) {
            subsystem->update(elapsed_sec);
            elapsed_sec = 0;
        }
    }
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGSubsystemMgr.
////////////////////////////////////////////////////////////////////////


FGSubsystemMgr::FGSubsystemMgr ()
{
}

FGSubsystemMgr::~FGSubsystemMgr ()
{
}

void
FGSubsystemMgr::init ()
{
    for (int i = 0; i < MAX_GROUPS; i++)
            _groups[i].init();
}

void
FGSubsystemMgr::reinit ()
{
    for (int i = 0; i < MAX_GROUPS; i++)
            _groups[i].reinit();
}

void
FGSubsystemMgr::bind ()
{
    for (int i = 0; i < MAX_GROUPS; i++)
        _groups[i].bind();
}

void
FGSubsystemMgr::unbind ()
{
    for (int i = 0; i < MAX_GROUPS; i++)
        _groups[i].unbind();
}

void
FGSubsystemMgr::update (double delta_time_sec)
{
    for (int i = 0; i < MAX_GROUPS; i++) {
        _groups[i].update(delta_time_sec);
    }
}

void
FGSubsystemMgr::suspend ()
{
    for (int i = 0; i < MAX_GROUPS; i++)
        _groups[i].suspend();
}

void
FGSubsystemMgr::resume ()
{
    for (int i = 0; i < MAX_GROUPS; i++)
        _groups[i].resume();
}

bool
FGSubsystemMgr::is_suspended () const
{
    return false;
}

void
FGSubsystemMgr::add (const char * name, FGSubsystem * subsystem,
                     GroupType group, double min_time_sec)
{
    SG_LOG(SG_GENERAL, SG_INFO, "Adding subsystem " << name);
    get_group(group)->set_subsystem(name, subsystem, min_time_sec);

    if (_subsystem_map.find(name) != _subsystem_map.end()) {
        SG_LOG(SG_GENERAL, SG_ALERT, "Adding duplicate subsystem " << name);
        throw sg_exception("duplicate subsystem");
    }
    _subsystem_map[name] = subsystem;
}

FGSubsystemGroup *
FGSubsystemMgr::get_group (GroupType group)
{
    return &(_groups[group]);
}

FGSubsystem *
FGSubsystemMgr::get_subsystem (const string &name)
{
    map<string,FGSubsystem *>::iterator s =_subsystem_map.find(name);

    if (s == _subsystem_map.end())
        return 0;
    else
        return s->second;
}

// end of fgfs.cxx
