# Simple iax phone.

package require iaxclient
package require tile

set user mats
set pass mats
set host jakinbidea.mine.nu
set number 004613136114
#set number 004690510
set line 0

proc NotifyRegister {id reply msgcount} {
    puts "NotifyRegister: $id $reply $msgcount"
    set ::cb(reg) "Reg: $reply"
}
proc NotifyState {callNo state args} {
    puts "NotifyState $args"
    set ::cb(state) "State: $state"
}
proc NotifyNetStats {args} {
    puts "NotifyNetStats: $args"
}
proc NotifyLevels {args} {
    puts "NotifyLevels: $args"
}
iaxclient::notify <Registration> NotifyRegister
iaxclient::notify <State>        NotifyState
iaxclient::notify <NetStats>     NotifyNetStats
#iaxclient::notify <Levels>       NotifyLevels
iaxclient::changeline $line

proc Register {} {
    set ::session [iaxclient::register $::user $::pass $::host]
}
proc UnRegister {} {
    iaxclient::unregister $::session
}
proc Dial {} {
    iaxclient::dial ${::user}:${::pass}@${::host}/${::number} $::line   
}
proc HangUp {} {
    iaxclient::hangup   
}
proc MicCmd {level} {
    iaxclient::level input [expr $level/100.0]
}
proc SpeakerCmd {level} {
    iaxclient::level output [expr $level/100.0]    
}
set cb(reg) "Reg:"
set cb(state) "State:"
set vol(mic) 100
set vol(spk) 100
set vol(mic) [expr 100*[iaxclient::level input]]
set vol(spk) [expr 100*[iaxclient::level output]]

# UI:
toplevel .phone
wm title .phone "IAX Phone"
wm resizable .phone 0 0
set w .phone.pad.top
pack [ttk::frame .phone.pad -padding {16 10}]
pack [ttk::frame $w] -side top
ttk::label $w.tit -text "Simple IAX Phone" -padding {0 4 0 8}
ttk::entry $w.num -textvariable number
ttk::button $w.dial -text Dial -command Dial
ttk::button $w.hang -text HangUp -command HangUp
ttk::button $w.reg  -text Register -command Register
ttk::button $w.ureg -text UnRegister -command UnRegister
ttk::button $w.acc  -text "Account Info" -command SetAccount
ttk::label $w.lreg  -textvariable cb(reg)
ttk::label $w.lstat -textvariable cb(state)

grid  $w.tit    -
grid  $w.num    -        -sticky ew -pady 2
grid  $w.dial   $w.hang  -sticky ew -padx 4 -pady 2
grid  $w.reg    $w.ureg  -sticky ew -padx 4 -pady 2
grid  $w.acc    -        -sticky ew -pady 2
grid  $w.lreg   -        -sticky w  -pady 2
grid  $w.lstat  -        -sticky w  -pady 2
grid columnconfigure $w 0 -uniform u
grid columnconfigure $w 1 -uniform u

set w .phone.pad.vol
pack [ttk::frame $w] -side top
ttk::label $w.lmic -text Microphone:
ttk::label $w.lspk -text Speakers:
ttk::scale $w.smic -orient horizontal -from 0 -to 100 \
  -variable vol(mic) -command MicCmd
ttk::scale $w.sspk -orient horizontal -from 0 -to 100 \
  -variable vol(spk) -command SpeakerCmd
ttk::progressbar $w.pmic -orient horizontal -variable lev(mic)
ttk::progressbar $w.pspk -orient horizontal -variable lev(spk)

grid  $w.lmic  $w.smic  -sticky e -padx 4 -pady 2
grid  x        $w.pmic  -sticky ew
grid  $w.lspk  $w.sspk  -sticky e -padx 4 -pady 2
grid  x        $w.pspk  -sticky ew
grid columnconfigure $w 1 -weight 1

proc SetAccount {} {
    set w .phone_account
    toplevel $w
    wm title $w "Account Info"
    wm resizable $w 0 0
    set box $w.pad.top
    pack [ttk::frame $w.pad -padding {16 10}]
    pack [ttk::frame $box] -side top
    
    ttk::label $box.host -text "Host:"
    ttk::label $box.user -text "Username:"
    ttk::label $box.pass -text "Password:"
    ttk::entry $box.ehost -textvariable ::host
    ttk::entry $box.euser -textvariable ::user
    ttk::entry $box.epass -textvariable ::pass
    
    grid  $box.host  $box.ehost  -pady 2
    grid  $box.user  $box.euser  -pady 2
    grid  $box.pass  $box.epass  -pady 2
    grid $box.host  $box.user  $box.pass  -sticky e
    grid $box.ehost $box.euser $box.epass -sticky ew

    set bot $w.pad.bot
    pack [ttk::frame $bot -padding {4 6}] -fill x
    ttk::button $bot.set -text Set -command [list destroy $w]
    pack $bot.set -side right
}
