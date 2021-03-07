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
import signal
import subprocess
import sys
import time

import FlightGear

def log(text):
    print(text, file=sys.stderr)
    sys.stderr.flush()

g_cleanup = []

class Fg:
    '''
    Runs flightgear. self.fg is a FlightGear.FlightGear instance, which uses
    telnet to communicate with Flightgear.
    '''
    def __init__(self, aircraft, args, env=None):
        '''
        aircraft:
            Specified as: --aircraft={aircraft}. This is separate from <args>
            because we need to know the name of recording files.
        args:
            Miscellenous args either space-separated name=value strings or a
            dict.
        env:
            Environment to set. If DISPLAY is not in <env> we add 'DISPLAY=:0'.
        '''
        self.child = None
        self.aircraft = aircraft
        args += f' --aircraft={aircraft}'
        
        port = 5500
        args += f' --telnet={port}'
        args2 = args.split()
        
        if isinstance(env, str):
            env2 = dict()
            for nv in env.split():
                n, v = nv.split('=', 1)
                env2[n] = v
            env = env2
        environ = os.environ.copy()
        if env:
            environ.update(env)
        if 'DISPLAY' not in environ:
            environ['DISPLAY'] = ':0'
            
        # Run flightgear in new process, telling it to open telnet server.
        self.child = subprocess.Popen(args2)
        
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
                self.fg = FlightGear.FlightGear('localhost', port)
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
        self.child.terminate()
        self.child.wait()
        self.child = None

    def __del__(self):
        if self.child:
            log('*** Fg.__del__() calling self.close()')
            self.close()

def make_recording(
        fg,
        continuous=0,
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
        fg.fg['/sim/replay/record-continuous'] = 1
        t0 = time.time()
        while 1:
            if time.time() > t0 + length:
                break
            fg.run_command('run view-step step=1')
            time.sleep(1)
        fg.fg['/sim/replay/record-continuous'] = 0
        path = f'{fg.aircraft}-continuous.fgtape'
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
            t = fg.fg['/sim/time/elapsed-sec']
            log(f'/sim/time/elapsed-sec={t}')
            if t > length:
                break
            time.sleep(length - t + 0.5)
        log(f'/sim/time/elapsed-sec={t}')
        fg.fg.telnet._putcmd('run save-tape tape-data/starttime= tape-data/stoptime=')
        response = fg.fg.telnet._getresp()
        log(f'response: {response!r}')
        path = f'{fg.aircraft}.fgtape'
    
    # Check recording is new.
    os.system(f'ls -lL {path}')
    s = os.stat(path, follow_symlinks=True)
    assert s.st_mtime > t
    path2 = os.readlink(path)
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
    
    # Start Flightgear.
    fg = Fg(aircraft, f'{fgfs_save} {args}',
            #env='SG_LOG_DELTAS=flightgear/src/Network/props.cxx=4',
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
    
    g_cleanup.append(lambda: os.remove(path))
    fg.close()
    
    # Load recording into new Flightgear.
    path = f'{aircraft}-continuous.fgtape' if continuous else f'{aircraft}.fgtape'
    fg = Fg(aircraft, f'{fgfs_load} {args} --load-tape={path}')
    fg.waitfor('/sim/fdm-initialized', 1, timeout=45)
    fg.waitfor('/sim/replay/replay-state', 1)
    
    # Check replay time is ok.
    rtime_begin = fg.fg['/sim/replay/start-time']
    rtime_end = fg.fg['/sim/replay/end-time']
    rtime = rtime_end - rtime_begin
    log(f'rtime={rtime_begin}..{rtime_end}, recording length: {rtime}, length={length}')
    assert rtime > length-1 and rtime < length+2, \
            f'length={length} rtime_begin={rtime_begin} rtime_end={rtime_end} rtime={rtime}'
    
    num_frames_extra_properties = fg.fg['/sim/replay/continuous-stats-num-frames-extra-properties']
    log(f'num_frames_extra_properties={num_frames_extra_properties}')
    if continuous:
        if main_view:
            assert num_frames_extra_properties > 1
        else:
            assert num_frames_extra_properties == 0
    else:
        assert num_frames_extra_properties in (0, None), \
                f'num_frames_extra_properties={num_frames_extra_properties}'
    
    time.sleep(length)
    
    fg.close()
    
    os.remove(path) 

    log('Test passed')


if __name__ == '__main__':

    fgfs = f'./build-walk/fgfs.exe-run.sh'
    fgfs_old = None
    
    continuous_s = [0, 1]
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
        elif arg == '--continuous':
            continuous_s = map(int, next(args).split(','))
        elif arg == '--extra-properties':
            extra_properties_s = map(int, next(args).split(','))
        elif arg == '--it-max':
            it_max = int(next(args))
        elif arg == '--it-min':
            it_min = int(next(args))
        elif arg == '--main-view':
            main_view_s = map(int, next(args).split(','))
        elif arg == '--multiplayer':
            multiplayer_s = map(int, next(args).split(','))
        elif arg == '-f':
            fgfs = next(args)
        elif arg == '--f-old':
            fgfs_old = next(args)
            fgfs_reverse = [0, 1]
        else:
            raise Exception(f'Unrecognised arg: {arg!r}')
    
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
    
    # If everything passed, cleanup. Otherwise leave recordings in place, as
    # they can be useful for debugging.
    #
    for f in g_cleanup:
        try:
            f()
        except:
            pass
    
    if 0:
        # This path can be used to check we cleanup properly after an error.
        fg = Fg('./build-walk/fgfs.exe-run.sh --aircraft=harrier-gr3 --airport=egtk')
        time.sleep(5)
        assert 0
