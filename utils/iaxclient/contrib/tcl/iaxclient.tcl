#  iaxclient.tcl --
#  
#       Script support for the iaxclient. Part of the iaxclient for Tcl.
#       Note that these scripts hijack the <State> notifier.
#      
#  Copyright (c) 2006 Mats Bengtsson
#  Copyright (c) 2006 Antonio Cano damas
#
# $Id$

namespace eval iaxclient {

    variable priv
    array set priv {
	state       "free"
	oldstate    "free"
	timeout     4000
	pending     0
	binds       {}
    }

    variable cand    
    array set cand {
	candidates  {}
	cmd         {}
    }
}

# iaxclient::actionbind --
# 
#       It replaces the <State> notifier interface with 'iaxclient::statebind'.
#       This makes a modified state notifier using 'actions' instead to
#       properly detect timeouts.
#       The action is identical to the state change except for timeouts.
#       The timeout always results in the 'free' state, without any extra
#       notification.
#       An action is:
#       
#         state --action--> new state
#         
# Arguments:
#       None
#	
# Results:
#       None

proc iaxclient::actionbind {} {
    variable priv
    
    notify <State> [namespace code State]
    set priv(state)    [state]
    set priv(oldstate) [state]
    return
}

# iaxclient::statebind --
# 
#       This is an interface for <State> events which is more flexible than
#       the 'iaxclinet::notify <State>' interface.
#
# Arguments:
#       sub args    add tclProc
#                   delete tclProc
#                   info
#	
# Results:
#       None

proc iaxclient::statebind {sub args} {
    variable priv
    
    switch -- $sub {
	add {
	    if {[llength $args] == 1} {
		if {[lsearch $priv(binds) $args] < 0} {
		    lappend priv(binds) $args
		}
	    } else {
		return -code error "usage: iaxclient::bind add tclProc"
	    }
	}
	delete {
	    if {[llength $args] == 1} {
		set idx [lsearch $priv(binds) $args]
		if {$idx >= 0} {
		    set priv(binds) [lreplace $priv(binds) $idx $idx]
		}
	    } else {
		return -code error "usage: iaxclient::bind delete tclProc"
	    }
	}
	info {
	   return $priv(binds) 
	}
	default {
	    return -code error "sub command must be one of add, delete, or info"
	}
    }
}

# iaxclient::timeout --
# 
#       Sets or gets the timeout before we receive any response at all
#       from an 'active outgoing' state change.

proc iaxclient::timeout {{ms ""}} {
    variable priv

    if {$ms eq ""} {
	return $priv(timeout)
    } else {
	set priv(timeout) $ms
    }
}

# iaxclient::reset --
# 
#       Resets all internal stuff for 'actionbind' and 'dial_candidates'.
#       You need to call iaxclient::hangup yourself.

proc iaxclient::reset {} {
    variable priv
    
    if {[::info exists priv(id)]} {
	after cancel $priv(id)
    }
}

proc iaxclient::State {callNo state args} {    
    variable priv

    # Do this to be able to do string comparisons on lists.
    set state [lsort $state]

    # Push the state change on our internal cache.
    set priv(oldstate) $priv(state)
    set priv(state)    $state

    # Skip non changes which we get from changeline actions etc.
    if {$state eq $priv(oldstate)} {
	return
    }

    set action $state
    set cancel 1
    if {$state eq [list active outgoing]} {
	set priv(id) [after $priv(timeout) [namespace code Timeout]]
	set cancel 0
    } elseif {($state eq "free") && $priv(pending)} {
	set action "timeout"
    }
    if {$cancel && [::info exists priv(id)]} {
	after cancel $priv(id)
	unset priv(id)
    }
    set priv(pending) 0
    
    # Make the callback. Be sure to supply all info we've got.
    foreach cmd $priv(binds) {
	uplevel #0 $cmd [list $action $callNo $state] $args
    }
}

proc iaxclient::Timeout {} {
    variable priv

    unset -nocomplain priv(id)
    set priv(pending) 1
    hangup
}

# iaxclient::dial_candidates --
# 
#       Gives a high level interface to dial a number of candidates.
#       We get all the usual iaxclient <State> events but prepended with
#       '{host port} action'. The action is any of the iaxclient states,
#       'timeout' and 'error'. 
#       We get callbacks until the call terminates for one reason or another.
#       
# Arguments:
#	candidates  a list {{host port} {host port} ...} of calls to try
#	            in that order
#	cmd         tclProc that gets called with arguments:
#	              {{host port} action notifyArgs...}
#	
# Results:
#       None

proc iaxclient::dial_candidates {candidates cmd} {
    variable cand
    
    set cand(candidates) $candidates
    set cand(cmd) $cmd
    
    actionbind
    statebind add [namespace code CEvent]
    if {[llength $candidates]} {
	set host [lindex $candidates 0 0]
	set port [lindex $candidates 0 1]
	dial ${host}:${port}/
    } else {
	return -code error "No candidates provided"
    }
    return
}

proc iaxclient::CEvent {action args} {
    variable cand

    # The actual {host port} candidate that causes this event.
    set candidate [lindex $cand(candidates) 0]
    set cmd $cand(cmd)
    set type $action
    
    uplevel #0 $cmd [list $candidate $action] $args
    if {$action eq "timeout"} {
	
	# Pop failing candidate and dial the next one.
	set cand(candidates) [lrange $cand(candidates) 1 end]
	if {[llength $cand(candidates)]} {
	    set host [lindex $cand(candidates) 0 0]
	    set port [lindex $cand(candidates) 0 1]
	    
	    # IMPORTANT: this is a workaround for bug
	    #     1587718 EXC_BAD_ACCESS in iax_hangup 
	    after 100 [namespace code [list dial ${host}:${port}/]]
	} else {
	    set type "error"
	    uplevel #0 $cmd [list $candidate $type] $args
	}
    }    
    
    # Identify our endpoint when we either fail, or either part close the call.
    # This is our destructor.
    if {($type eq "error") || ($type eq "free")} {
	statebind delete [namespace code CEvent]
	array set cand {
	    candidates  {}
	    cmd         {}
	}
    }
}

if {0} {
    proc cmd {args} {log "+++> $args"}
    set candidates {{home.se 8899} {192.168.0.114 4569}}
    iaxclient::actionbind
    iaxclient::dial_candidates $candidates cmd    
}


