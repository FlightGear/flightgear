#include "fgfs.hxx"

#include <simgear/debug/logstream.hxx>

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

// end of fgfs.cxx
