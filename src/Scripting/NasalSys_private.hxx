// NasalSys.hxx -
// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: Copyright (C) 2013  James Turner

#pragma once

#include <simgear/debug/BufferedLogCallback.hxx>
#include <simgear/nasal/nasal.h>
#include <simgear/props/props.hxx>
#include <simgear/threads/SGQueue.hxx>
#include <simgear/xml/easyxml.hxx>

#include "NasalModelData.hxx"

// forward decls
class FGNasalSys;
class TimerObj; ///< persistent timer created by maketimer
class FGNasalModuleListener;
class NasalCommand;

/**
  @breif wrapper for naEqual which recursively checks vec/hash equality
    Probably not very performant.
 */
int nasalStructEqual(naContext ctx, naRef a, naRef b);

class FGNasalListener : public SGPropertyChangeListener {
public:
    FGNasalListener(SGPropertyNode* node, naRef code, FGNasalSys* nasal,
                    int key, int id, int init, int type);
    
    virtual ~FGNasalListener();
    virtual void valueChanged(SGPropertyNode* node);
    virtual void childAdded(SGPropertyNode* parent, SGPropertyNode* child);
    virtual void childRemoved(SGPropertyNode* parent, SGPropertyNode* child);
    
private:
    bool changed(SGPropertyNode* node);
    void call(SGPropertyNode* which, naRef mode);
    
    friend class FGNasalSys;
    SGPropertyNode_ptr _node;
    naRef _code;
    int _gcKey;
    int _id;
    FGNasalSys* _nas;
    int _init;
    int _type;
    unsigned int _active;
    bool _dead;
    long _last_int;
    double _last_float;
    std::string _last_string;
};


class NasalXMLVisitor : public XMLVisitor {
public:
    NasalXMLVisitor(naContext c, int argc, naRef* args);
    virtual ~NasalXMLVisitor() { naFreeContext(_c); }
    
    virtual void startElement(const char* tag, const XMLAttributes& a);
    virtual void endElement(const char* tag);
    virtual void data(const char* str, int len);
    virtual void pi(const char* target, const char* data);
    
private:
    void call(naRef func, int num, naRef a = naNil(), naRef b = naNil());
    naRef make_string(const char* s, int n = -1);
    
    naContext _c;
    naRef _start_element, _end_element, _data, _pi;
};

//
// See the implementation of the settimer() extension function for
// more notes.
//
struct NasalTimer
{
    NasalTimer(naRef handler, FGNasalSys* sys);
    
    void timerExpired();
    ~NasalTimer();
    
    naRef handler;
    int gcKey = 0;
    FGNasalSys* nasal = nullptr;
};

// declare the interface to the unit-testing module
naRef initNasalUnitTestCppUnit(naRef globals, naContext c);
naRef initNasalUnitTestInSim(naRef globals, naContext c);


class NasalSysPrivate
{
public:
    ~NasalSysPrivate();

    //friend class FGNasalScript;
    // friend class FGNasalListener;
    // friend class FGNasalModuleListener;

    SGLockedQueue<SGSharedPtr<FGNasalModelData>> _loadList;
    SGLockedQueue<SGSharedPtr<FGNasalModelData>> _unloadList;
    // Delay removing items of the _loadList to ensure the are already attached
    // to the scene graph (eg. enables to retrieve world position in load
    // callback).
    bool _delay_load;

    // Listener
    std::map<int, FGNasalListener*> _listener;
    std::vector<FGNasalListener*> _dead_listener;

    std::vector<FGNasalModuleListener*> _moduleListeners;

    static int _listenerId;

    bool _inited = false;
    naContext _context = nullptr;
    naRef _globals,
        _string;

    SGPropertyNode_ptr _cmdArg;

    std::unique_ptr<simgear::BufferedLogCallback> _log;

    typedef std::map<std::string, NasalCommand*> NasalCommandDict;
    NasalCommandDict _commands;

    naRef _wrappedNodeFunc;

    // track NasalTimer instances (created via settimer() call) -
    // this allows us to clean these up on shutdown
    std::vector<NasalTimer*> _nasalTimers;


    // track persistent timers. These are owned from the Nasal side, so we
    // only track a non-owning reference here.
    std::vector<TimerObj*> _persistentTimers;
};
