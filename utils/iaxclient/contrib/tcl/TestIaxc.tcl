package require iaxclient

set user mats
set pass mats
set host jakinbidea.mine.nu
set number 004613136114
set line 0

proc NotifyRegister {args} {
    global regid
    puts "NotifyRegister: $args"
    set regid [lindex $args 0]
}
proc NotifyState {args} {
    puts "NotifyState $args"
}
iaxclient::notify <Registration> NotifyRegister
iaxclient::notify <State>        NotifyState

iaxclient::register $user $pass $host

iaxclient::changeline $line
iaxclient::dial ${user}:${pass}@${host}/${number} $line


iaxclient::hangup

iaxclient::unregister $regid



% iaxclient::register $user $pass $host
iaxclient::register $user $pass $host
1

% NotifyRegister: 1 ack -1

iaxclient::changeline $line
iaxclient::changeline $line
NotifyState 0 free 0 {} {} {} {}

% iaxclient::dial ${user}:${pass}@${host}/${number} $line
iaxclient::dial ${user}:${pass}@${host}/${number} $line
% NotifyState 0 {active outgoing} 0 004613136114 mats:mats@jakinbidea.mine.nu/004613136114 {Not Available} default

NotifyState 0 {active outgoing} 0 004613136114 mats:mats@jakinbidea.mine.nu/004613136114 {Not Available} default

NotifyState 0 free 0 004613136114 mats:mats@jakinbidea.mine.nu/004613136114 {Not Available} default

