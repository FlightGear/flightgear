#!/usr/bin/env python3

'''
Test script for record/replay. Only tested on Unix.

E.g.:

    ./flightgear/scripts/python/recordreplay.py -f run_fgfs.sh

Args:
    --all
        Run all tests (this is default).
    --continuous BOOLS
    --extra-properties BOOLS
    --it-max <it>
        Set min iteration (inclusive) to run; useful when fixing tests and
        retrying.
    --it-min <it>
        Set max iteration (exclusive) to run; useful when fixing tests and
        retrying.
    --main-view BOOLS
    --multiplayer
    -f <fgfs>
        The fgfs executable to use. Default assumes the Walk build system.
    --f-old <fgfs-old>
        A second fgfs executable. If specified we run all tests twice, first
        using <fgfs-old> to create the recording and <fgfs> to replay it,
        second the other way round.
    --test-motion
        Checks that speed of aircraft on replay is not affected by frame rate.

        We deliberately change frame rate while recording UFO moving at
        constant speed.
    
    --test-motion-mp
        Checks that speed of MP on replay is not affected by frame rate.

        We deliberately change frame rate while recording UFO moving at
        constant speed.
    
    BOOLS is comma-sparated list of 0 or 1, with 1 activating the particular
    feature. So for example '--continuous 0' tests normal recording/replay',
    '--continuous 1' tests continuous recording/replay, and continuous 0,1'
    tests both.

    We test all combinations of continuous, extra-properties, main-view and
    multiplayer recordings. For each test we check that we can create a
    recording, and replay it in a new fgfs instance. When replaying we check
    a small number of basic things such as the recording length, and whether
    extra-properties are replayed.
'''

import os
import resource
import signal
import subprocess
import sys
import time

import FlightGear

def log(text):
    print(text, file=sys.stderr)
    sys.stderr.flush()

g_cleanup = []
g_tapedir = './recordreplay.py.tapes'


def remove(path):
    '''
    Removes file, ignoring any error.
    '''
    log(f'Removing: {path}')
    try:
        os.remove(path)
    except Exception as e:
        log(f'Failed to remove {path}: {e}')


def readlink(path):
    '''
    Returns absolute path destination of link.
    '''
    ret = os.readlink(path)
    if not os.path.isabs(ret):
        ret = os.path.join(os.path.dirname(path), ret)
    return ret


class Fg:
    '''
    Runs flightgear, with support for setting/getting properties etc.

    self.fg is a FlightGear.FlightGear instance, which uses telnet to
    communicate with Flightgear.
    '''
    def __init__(self, aircraft, args, env=None, telnet_port=None, telnet_hz=None, out=None, screensaver_suspend=True):
        '''
        aircraft:
            Specified as: --aircraft={aircraft}. This is separate from <args>
            because we need to know the name of recording files.
        args:
            Miscellenous args either space-separated name=value strings or a
            dict.
        env:
            Environment to set. If DISPLAY is not in <env> we add 'DISPLAY=:0'.
        telnet_port:
        telnet_hz:
            .
        '''
        self.child = None
        self.aircraft = aircraft
        args += f' --aircraft={aircraft}'
        
        if telnet_port is None:
            telnet_port = 5500
        if telnet_hz is None:
            args += f' --telnet={telnet_port}'
        else:
            args += f' --telnet=_,_,{telnet_hz},_,{telnet_port},_'
        args += f' --prop:/sim/replay/tape-directory={g_tapedir}'
        args += f' --prop:bool:/sim/startup/screensaver-suspend={"true" if screensaver_suspend else "false"}'
        
        args2 = args.split()
        
        environ = os.environ.copy()
        if isinstance(env, str):
            for nv in env.split():
                n, v = nv.split('=', 1)
                environ[n] = v
        if 'DISPLAY' not in environ:
            environ['DISPLAY'] = ':0'
            
        # Run flightgear in new process, telling it to open telnet server.
        #
        # We run not in a shell, otherwise self.child.terminate() doesn't
        # work - it would kill the shell but leave fgfs running (there are
        # workarounds for this, such as prefixing the command with 'exec').
        #
        log(f'Command is: {args}')
        log(f'Running: {args2}')
        def preexec():
            try:
                resource.setrlimit(resource.RLIMIT_CORE, (resource.RLIM_INFINITY, resource.RLIM_INFINITY))
            except Exception as e:
                log(f'*** preexec failed with e={e}')
                raise
        if out:
            out = open(out, 'w')
        self.child = subprocess.Popen(
                args2,
                env=environ,
                preexec_fn=preexec,
                stdout=out,
                stderr=subprocess.STDOUT,
                )
        
        # Connect to flightgear's telnet server.
        timeout = 15
        t0 = time.time()
        while 1:
            time.sleep(1)
            dt = time.time() - t0
            if dt > timeout:
                text = f'Timeout trying to connect. timeout={timeout}'
                log(text)
                raise Exception(text)
            try:
                log('Connecting... ')
                self.fg = FlightGear.FlightGear('localhost', telnet_port)
                log(f'Connected. timeout={timeout} dt={dt:.1f}')
                return
            except Exception as e:
                log(f'Failed to connect timeout={timeout} dt={dt:.1f}: {e}')
    
    def waitfor(self, name, value, timeout=30):
        '''
        Waits for specified property to be <value>. Returns time taken.
        '''
        t0 = time.time()
        while 1:
            time.sleep(1)
            dt = time.time() - t0
            try:
                v = self.fg[name]
                log(f'Waiting for {name}={value} current value: {v}. timeout={timeout} dt={dt:.1f}')
            except Exception as e:
                log(f'Failed to get value of property {name}: {e}. timeout={timeout} dt={dt:.1f}')
                v = None
            if v == value:
                return dt
            if dt > timeout:
                raise Exception(f'Timeout waiting for {name}={value}; current value: {v}. timeout={timeout}')
    
    def run_command(self, command):
        self.fg.telnet._putcmd(command)
        ret = self.fg.telnet._getresp()
        log(f'command={command!r} ret: {ret}')
        return ret
    
    def close(self):
        assert self.child
        log(f'close(): stopping flightgear pid={self.child.pid}')
        if 1:
            # Kill any child processes so that things work if fgfs is being run
            # by download_and_compile.sh's run_fgfs.sh script.
            #
            # This is Unix-only.
            try:
                child_pids = subprocess.check_output(f'pgrep -P {self.child.pid}', shell=True)
            except Exception:
                # We get here if self.child has no child processes.
                child_pids = b''
            child_pids = child_pids.decode('utf-8')
            child_pids = child_pids.split()
            for child_pid in child_pids:
                #log(f'*** close() child_pid={child_pid}')
                child_pid = int(child_pid)
                #log(f'*** close() killing child_pid={child_pid}')
                os.kill(child_pid, signal.SIGTERM)
        self.child.terminate()
        self.child.wait()
        self.child = None
        #log(f'*** close() returning.')

    def __del__(self):
        if self.child:
            log('*** Fg.__del__() calling self.close()')
            self.close()

def make_recording(
        fg,
        continuous=0,   # 2 means continuous with compression.
        extra_properties=0,
        main_view=0,
        length=5,
        ):
    '''
    Makes a recording, and returns its path.
    
    We check that the recording file is newly created.
    '''
    t = time.time()
    fg.fg['/sim/replay/record-signals'] = True  # Just in case they are disabled by user.
    if continuous:
        assert not fg.fg['/sim/replay/record-continuous']
        if continuous == 2:
            fg.fg['/sim/replay/record-continuous-compression'] = 1
        fg.fg['/sim/replay/record-continuous'] = 1
        t0 = time.time()
        while 1:
            if time.time() > t0 + length:
                break
            time.sleep(1)
            fg.run_command('run view-step step=1')
        fg.fg['/sim/replay/record-continuous'] = 0
        path = f'{g_tapedir}/{fg.aircraft}-continuous.fgtape'
        time.sleep(1)
    else:
        # Normal recording will have effectively already started, so we sleep
        # for the remaining time. This is a little inaccurate though because it
        # looks like normal recording starts a little after t=0, e.g. at t=0.5.
        #
        # Also, it looks like /sim/time/elapsed-sec doesn't quite track real
        # time, so we sometimes need to sleep a little longer.
        #
        while 1:
            # Telnet interface seems very slow even if we set telnet_hz to
            # 100 (for example). We want to make recording have near to the
            # specified length, so we are cautious about overrunning.
            #
            #log(f'a: time.time()-t={time.time()-t}')
            t_record_begin = fg.fg['sim/replay/record-normal-begin']
            #log(f'b: time.time()-t={time.time()-t}')
            t_record_end = fg.fg['sim/replay/record-normal-end']
            #log(f'c: time.time()-t={time.time()-t}')
            t_delta = t_record_end - t_record_begin
            log(f't_record_begin={t_record_begin} t_record_end={t_record_end} t_delta={t_delta}')
            if t_delta >= length:
                break
            ts = max(length - t_delta - 1, 0.2)
            log(f'd: ts={ts}')
            time.sleep(ts)
        log(f'/sim/time/elapsed-sec={t}')
        log(f'/sim/replay/start-time={fg.fg["/sim/replay/start-time"]}')
        log(f'/sim/replay/end-time={fg.fg["/sim/replay/end-time"]}')
        fg.fg.telnet._putcmd('run save-tape tape-data/starttime= tape-data/stoptime=')
        response = fg.fg.telnet._getresp()
        log(f'response: {response!r}')
        path = f'{g_tapedir}/{fg.aircraft}.fgtape'
    
    # Check recording is new.
    os.system(f'ls -lL {path}')
    s = os.stat(path, follow_symlinks=True)
    assert s.st_mtime > t
    path2 = readlink(path)
    log(f'path={path} path2={path2}')
    return path
    

def test_record_replay(
        fgfs_save,
        fgfs_load,
        multiplayer,
        continuous,
        extra_properties,
        main_view,
        length,
        ):
    if not fgfs_load:
        fgfs_load = fgfs_save
    log(f'=== save: {fgfs_save}')
    log(f'=== load: {fgfs_load}')
    log(f'=== --multiplayer {multiplayer} --continuous {continuous} --extra-properties {extra_properties} --main-view {main_view}')
    log(f'===')
    
    aircraft = 'harrier-gr3'
    args =  f'--state=vto --airport=egtk'
    args += f' --prop:bool:/sim/replay/record-extra-properties={extra_properties}'
    args += f' --prop:bool:/sim/replay/record-main-view={main_view}'
    args += f' --prop:bool:/sim/replay/record-main-window=0'
    #args += f' --prop:bool:/sim/time/simple-time/enabled=0'
    
    # Start Flightgear.
    fg = Fg(aircraft, f'{fgfs_save} {args}',
            #env='SG_LOG_DELTAS=flightgear/src/Network/props.cxx=4',
            telnet_hz=100,
            )
    fg.waitfor('/sim/fdm-initialized', 1, timeout=45)
    
    assert fg.fg['sim/replay/record-extra-properties'] == extra_properties
    assert fg.fg['sim/replay/record-main-view'] == main_view
    log(f'sim/replay/record-extra-properties={fg.fg["sim/replay/record-extra-properties"]}')
    
    # Save recording:
    path = make_recording(fg,
            continuous=continuous,
            extra_properties=extra_properties,
            main_view=main_view,
            length=length,
            )
    
    g_cleanup.append(lambda: remove(path))
    fg.close()
    
    # Load recording into new Flightgear.
    path = f'{g_tapedir}/{aircraft}-continuous.fgtape' if continuous else f'{g_tapedir}/{aircraft}.fgtape'
    fg = Fg(aircraft, f'{fgfs_load} {args} --load-tape={path}')
    fg.waitfor('/sim/fdm-initialized', 1, timeout=45)
    fg.waitfor('/sim/replay/replay-state', 1)
    
    t0 = time.time()
    
    # Check replay time is ok.
    rtime_begin = fg.fg['/sim/replay/start-time']
    rtime_end = fg.fg['/sim/replay/end-time']
    rtime = rtime_end - rtime_begin
    log(f'rtime={rtime_begin}..{rtime_end}, recording length: {rtime}, length={length}')
    assert rtime > length-1 and rtime <= length+2, \
            f'length={length} rtime_begin={rtime_begin} rtime_end={rtime_end} rtime={rtime}'
    
    num_frames_extra_properties = fg.fg['/sim/replay/continuous-stats-num-frames-extra-properties']
    log(f'num_frames_extra_properties={num_frames_extra_properties}')
    if continuous:
        if main_view:
            assert num_frames_extra_properties > 1, f'num_frames_extra_properties={num_frames_extra_properties}'
        else:
            assert num_frames_extra_properties == 0
    else:
        assert num_frames_extra_properties in (0, None), \
                f'num_frames_extra_properties={num_frames_extra_properties}'
    
    fg.run_command('run dialog-show dialog-name=replay')
    
    while 1:
        t = time.time()
        if t < t0 + length - 1:
            pass
            # Disabled because it seems that Flightgear starts replaying before
            # we see replay-state set to 1 because scenery loading blocks
            # things.
            #
            #assert not fg.fg['/sim/replay/replay-state-eof'], f'Replay has finished too early; lenth={length} t-t0={t-t0}'
        if t > t0 + length + 1:
            assert fg.fg['/sim/replay/replay-state-eof'], f'Replay has not finished on time; lenth={length} t-t0={t-t0}'
            break
        e = fg.fg['sim/replay/replay-error']
        assert not e, f'Replay failed: e={e}'
        time.sleep(1)
    
    fg.close()
    
    remove(path) 

    log('Test passed')


def test_motion(fgfs, multiplayer=False):
    '''
    Records UFO moving with constant velocity with varying framerates, then
    replays with varying framerates and checks that replayed UFO moves with
    expected constant speed.

    If <multiplayer> is true we also record MP UFO running in second Flightgear
    instance and check that it too moves at constant speed when replaying.
    '''
    log('')
    log('='*80)
    log('== Record')
    
    aircraft = 'ufo'
    fgfs += ' --prop:bool:/sim/time/simple-time/enabled=true'
    if multiplayer:
        fg = Fg( aircraft, f'{fgfs} --prop:/sim/replay/log-raw-speed-multiplayer=cgdae-t')
    else:
        fg = Fg( aircraft, f'{fgfs}')
    path = f'{g_tapedir}/{fg.aircraft}-continuous.fgtape'
    
    fg.waitfor('/sim/fdm-initialized', 1, timeout=45)
    
    fg.fg['/controls/engines/engine[0]/throttle'] = 0
    
    # Throttle/speed for ufo is set in fgdata/Aircraft/ufo/ufo.nas.
    #
    speed_max = 2000    # default for ufo; current=7.
    fixed_speed = 100
    throttle = fixed_speed / speed_max
    
    if multiplayer:
        fg.fg['/sim/replay/record-multiplayer'] = True
        fg2 = Fg( aircraft, f'{fgfs} --callsign=cgdae-t --multiplay=in,4,,5033 --read-only', telnet_port=5501)
        fg2.waitfor('/sim/fdm-initialized', 1, timeout=45)
        fg.fg['/controls/engines/engine[0]/throttle'] = throttle
        fg2.fg['/controls/engines/engine[0]/throttle'] = throttle
        time.sleep(1)
        fgt = fg.fg['/controls/engines/engine[0]/throttle']
        fg2t = fg2.fg['/controls/engines/engine[0]/throttle']
        log(f'fgt={fgt} fg2t={fg2t}')
    else:
        fg.fg['/controls/engines/engine[0]/throttle'] = throttle
    
    # Run UFO with constant speed, varying the framerate so we check whether
    # recorded speeds are affected.
    #
    fg.fg['/sim/frame-rate-throttle-hz'] = 5
    if multiplayer:
        fg2.fg['/sim/frame-rate-throttle-hz'] = 5
    
    # Delay to let frame rate settle.
    time.sleep(10)
    
    # Start recording.
    fg.fg['/sim/replay/record-continuous'] = 1
    time.sleep(5)
    
    # Change frame rate.
    fg.fg['/sim/frame-rate-throttle-hz'] = 2
    time.sleep(5)
    
    # Change frame rate.
    fg.fg['/sim/frame-rate-throttle-hz'] = 5
    if multiplayer:
        fg2.fg['/sim/frame-rate-throttle-hz'] = 2
    time.sleep(5)
    
    # Stop recording.
    fg.fg['/sim/replay/record-continuous'] = 0
    
    fg.close()
    if multiplayer:
        fg2.close()
    time.sleep(2)
    
    path2 = readlink( path)
    log(f'*** path={path} path2={path2}')
    g_cleanup.append(lambda: remove(path2))
    
    log('')
    log('='*80)
    log('== Replay')
    
    if multiplayer:
        fg = Fg( aircraft, f'{fgfs} --load-tape={path}'
                f' --prop:/sim/replay/log-raw-speed-multiplayer=cgdae-t'
                f' --prop:/sim/replay/log-raw-speed=true'
                )
    else:
        fg = Fg( aircraft,
                f'{fgfs} --load-tape={path} --prop:/sim/replay/log-raw-speed=true',
                #env='SG_LOG_DELTAS=flightgear/src/Aircraft/flightrecorder.cxx:replay=3',
                )
    fg.waitfor('/sim/fdm-initialized', 1, timeout=45)
    fg.fg['/sim/frame-rate-throttle-hz'] = 10
    fg.waitfor('/sim/replay/replay-state', 1)
    
    time.sleep(3)
    fg.fg['/sim/frame-rate-throttle-hz'] = 2
    time.sleep(5)
    fg.fg['/sim/frame-rate-throttle-hz'] = 5
    time.sleep(3)
    fg.fg['/sim/frame-rate-throttle-hz'] = 7
    
    fg.waitfor('/sim/replay/replay-state-eof', 1)
    
    errors = []
    def examine_values(infix=''):
        '''
        Looks at /sim/replay/log-raw-speed{infix}-values/value[], which will
        contain measured speed of user/MP UFO. We check that the values are all
        as expected - constant speed.
        '''
        log(f'== Looking at /sim/replay/log-raw-speed{infix}-values/value[]')
        items0 = fg.fg.ls( f'/sim/replay/log-raw-speed{infix}-values')
        log(f'{infix} len(items0)={len(items0)}')
        assert items0, f'Failed to read items in /sim/replay/log-raw-speed{infix}-values/'
        items = []
        descriptions = []
        for item in items0:
            if item.name == 'value':
                #log(f'have read item: {item}')
                items.append(item)
            elif item.name == 'description':
                descriptions.append(item)
        num_errors = 0
        for i in range(len(items)-1):   # Ignore last item because replay at end interpolates.
            item = items[i]
            description = ''
            if i < len(descriptions):
                description = descriptions[i].value
            speed = float(item.value)
            prefix = ' '
            if abs(speed - fixed_speed) > 0.1:
                num_errors += 1
                prefix = '*'
            log( f'    {infix} {prefix} speed={speed:12.4} details: {item}: {description}')
        if num_errors != 0:
            log( f'*** Replay showed uneven speed')
            errors.append('1')
    
    def show_values(paths):
        if isinstance(paths, str):
            paths = paths,
        log(f'Values in {paths}:')
        line2values = dict()
        for i, path in enumerate(paths):
            line = 0
            for item in fg.fg.ls(path):
                if item.name == 'value':
                    line2values.setdefault(line, []).append(item.value)
                    line += 1
        for line in sorted(line2values.keys()):
            t = ''
            for value in line2values[line]:
                t += f' {value}'
            log(f'    {t}')
    
    if multiplayer:
        examine_values()
        examine_values('-multiplayer')
        examine_values('-multiplayer-post')
        
        if 0:
            show_values('/sim/replay/log-raw-speed-multiplayer-post-relative-distance')
            show_values('/sim/replay/log-raw-speed-multiplayer-post-relative-bearing')
            show_values('/sim/replay/log-raw-speed-multiplayer-post-absolute-distance')
            show_values('/sim/replay/log-raw-speed-multiplayer-post-user-absolute-distance')
        
        def get_values(path):
            '''
            Returns <path>/value[] as a list.
            '''
            ret = []
            for item in fg.fg.ls(path):
                if item.name == 'value':
                    ret.append(item.value)
            return ret
        
        # Check that distance between user and mp is constant.
        #
        # The two paths below contain values[] that are the distances of the
        # mp and user aircraft from their starting points. Both are moving at
        # the same speed in the same direction, so the differences between each
        # pair of values should be constant.
        #
        distances_mp = get_values('/sim/replay/log-raw-speed-multiplayer-post-absolute-distance')
        distances_user = get_values('/sim/replay/log-raw-speed-multiplayer-post-user-absolute-distance')
        log(f'len(distances_user)={len(distances_user)} len(distances_mp)={len(distances_mp)}')
        assert len(distances_user) == len(distances_mp)
        assert len(distances_user) > 20
        for i in range(len(distances_user)):
            distance_mp = distances_mp[i]
            distance_user = distances_user[i]
            delta = distance_mp - distance_user
            if i == 0:
                delta_original = delta
            prefix = ' '
            if abs(delta - delta_original) > 0.01:
                #log('replay shows varying differences between user and mp aircraft')
                errors.append('1')
                prefix = '*'
            log(f'    {prefix} user={distance_user} mp={distance_mp} delta={delta}')
    else:
        examine_values()
    
    fg.close()
    if errors:
        raise Exception('Failure')

    log('test_motion() passed')


def test_carrier(fgfs):
    '''
    Checks that mp carrier motion is even.
    '''
    # We require simple-time. Can probably also work by setting the default
    # timing system's lag parameters but haven't figured this out yet.
    #
    simple_time = 'true'
    fg = Fg( 'harrier-gr3',
            f'{fgfs} --prop:int:/sim/mp-carriers/latch-always=1 --prop:bool:/sim/time/simple-time/enabled={simple_time} --callsign=cgdae3 --airport=ksfo',
            telnet_port=5500,
            telnet_hz=100,
            #out='out-rr-carrier-1',
            )
    fg.waitfor('/sim/fdm-initialized', 1, timeout=45)
    
    fg_carrier = Fg('Nimitz',
            f'{fgfs} --prop:int:/sim/mp-carriers/latch-always=1 --prop:bool:/sim/time/simple-time/enabled={simple_time} --callsign=cgdae4 --multiplay=in,1,,5033 --read-only',
            telnet_port=5501,
            #out='out-rr-carrier-2',
            )
    fg_carrier.waitfor('/sim/fdm-initialized', 1, timeout=45)
    
    fg.fg['/sim/replay/log-raw-speed-multiplayer'] = 'cgdae4'
    fg.fg['/sim/log-multiplayer-callsign'] = 'cgdae4'
    
    def get_items(path, leafname, out=None):
        '''
        Finds list of tuples from properties <path>/<leafname>[]/*. Appends new
        items to <out> and returns new items.
        
        Runs rather slowly because telnet commands appear to be throttled.
        '''
        if out is None:
            out = []
        out_len_original = len(out)
        items = fg.fg.ls(path)
        i = 0
        for item_i, item in enumerate(items):
            if item.name == leafname:
                if i == len(out):
                    #print(f'len(items)={len(items)} item_i={item_i}: looking at {path}/{leafname}[{item.index}]')
                    class Item:
                        pass
                    item2 = Item()
                    item2.i = i
                    for j in fg.fg.ls(f'{path}/{leafname}[{item.index}]'):
                        setattr( item2, j.name, j)
                    out.append(item2)
                i += 1
        return out[out_len_original:]
    
    t0 = time.time()
    mps = []
    mppackets = []
    while 1:
        time.sleep(1)
        t = time.time() - t0
        log(f'test_carrier(): t={t:.1f}')
        if t > 60:
            print(f'finished, t={t}')
            break
        mps_new         = get_items( '/sim/log-multiplayer', 'mp', mps)
        mppackets_new   = get_items( '/sim/log-multiplayer', 'mppacket', mppackets)
        for mp in mps_new:
            log(f'test_carrier():     mp: i={mp.i}:'
                    f' speed={mp.speed.value:20}'
                    f' distance={mp.distance.value:20}'
                    f' t={mp.t.value:20}'
                    f' dt={mp.dt.value:20}={mp.dt.value*120:20}/120'
                    f' ubody={mp.ubody.value:20}'
                    f' vbody={mp.vbody.value:20}'
                    f' wbody={mp.wbody.value:20}'
                    )
        for mppacket in mppackets_new:
            log(f'test_carrier():     mppacket: i={mppacket.i}:'
                    f' speed={mppacket.speed.value:20}'
                    f' distance={mppacket.distance.value:20}'
                    f' t={mppacket.t.value:20}'
                    f' linear_vel={mppacket.linear_vel.value:20}'
                    f' dt={mppacket.dt.value:20}={mppacket.dt.value*120:20}/120'
                    )
    
    # Check speed of multiplayer carrier is constant:
    knots2si = 1852.0/3600
    speed_expected = 10 * knots2si
    num_incorrect = 0
    for mp in mps[2:]:  # First two items have bogus values.
        delta = mp.speed.value - speed_expected
        if abs(delta) > 0.001:
            num_incorrect += 1
            print(f'    * speed={mp.speed.value:20}')
    assert num_incorrect == 0, f'num_incorrect={num_incorrect}'
    fg.close()
    fg_carrier.close()

if __name__ == '__main__':

    fgfs = f'./build-walk/fgfs.exe-run.sh'
    fgfs_old = None
    
    do_test = 'all'
    continuous_s = [0, 1, 2]    # 2 is continuous with compression.
    extra_properties_s = [0, 1]
    main_view_s = [0, 1]
    multiplayer_s = [0, 1]
    fgfs_reverse_s = [0]
    it_min = None
    it_max = None
    
    if len(sys.argv) == 1:
        do_all = True
    
    args = iter(sys.argv[1:])
    while 1:
        try:
            arg = next(args)
        except StopIteration:
            break
        if arg == '--all':
            do_all = True
        elif arg == '--carrier':
            do_test = 'carrier'
        elif arg == '--continuous':
            continuous_s = [int(x) for x in  next(args).split(',')]
            log(f'continuous_s={continuous_s}')
        elif arg == '--tape-dir':
            g_tapedir = next(args)
        elif arg == '--extra-properties':
            extra_properties_s = [int(x) for x in  next(args).split(',')]
        elif arg == '--it-max':
            it_max = int(next(args))
        elif arg == '--it-min':
            it_min = int(next(args))
        elif arg == '--main-view':
            main_view_s = [int(x) for x in  next(args).split(',')]
        elif arg == '--multiplayer':
            multiplayer_s = [int(x) for x in  next(args).split(',')]
        elif arg == '-f':
            fgfs = next(args)
        elif arg == '--f-old':
            fgfs_old = next(args)
            fgfs_reverse = [0, 1]
        elif arg == '--test-motion':
            do_test = 'motion'
        elif arg == '--test-motion-mp':
            do_test = 'motion-mp'
        else:
            raise Exception(f'Unrecognised arg: {arg!r}')
    
    g_tapedir = os.path.abspath(g_tapedir)
    os.makedirs( g_tapedir, exist_ok=True)
    if 0:
        pass
    elif do_test == 'carrier':
        test_carrier(fgfs)
    elif do_test == 'motion':
        test_motion( fgfs)
    elif do_test == 'motion-mp':
        test_motion( fgfs, True)
    elif do_test == 'all':
        try:
            if fgfs_old:
                for fgfs1, fgfs2 in [(fgfs, fgfs_old), (fgfs_old, fgfs)]:
                    for multiplayer in 0, 1:
                        test_record_replay(
                                fgfs1,
                                fgfs2,
                                multiplayer,
                                continuous=0,
                                extra_properties=0,
                                main_view=0,
                                length=10,
                                )
            else:
                log(f'continuous_s={continuous_s}')
                its_max = len(multiplayer_s) * len(continuous_s) * len(extra_properties_s) * len(main_view_s) * len(fgfs_reverse_s)
                it = 0
                for multiplayer in multiplayer_s:
                    for continuous in continuous_s:
                        for extra_properties in extra_properties_s:
                            for main_view in main_view_s:
                                for fgfs_reverse in fgfs_reverse_s:
                                    if fgfs_reverse:
                                        fgfs_save = fgfs_old
                                        fgfs_load = fgfs
                                    else:
                                        fgfs_save = fgfs
                                        fgfs_load = fgfs_old

                                    ok = True
                                    if it_min is not None:
                                        if it < it_min:
                                            ok = False
                                    if it_max is not None:
                                        if it >= it_max:
                                            ok = False
                                    log('')
                                    log(f'===')
                                    log(f'=== {it}/{its_max}')
                                    if ok:
                                        test_record_replay(
                                            fgfs_save,
                                            fgfs_load,
                                            multiplayer=multiplayer,
                                            continuous=continuous,
                                            extra_properties=extra_properties,
                                            main_view=main_view,
                                            length=10
                                            )
                                    it += 1
        finally:
            pass
    else:
        assert 0, f'do_test={do_test}'
    
    # If everything passed, cleanup. Otherwise leave recordings in place, as
    # they can be useful for debugging.
    #
    for f in g_cleanup:
        try:
            f()
        except Exception:
            pass

    log(f'{__file__}: Returning 0')
