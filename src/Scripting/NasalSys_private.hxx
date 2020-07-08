// Copyright (C) 2013  James Turner//
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

#ifndef __NASALSYS_PRIVATE_HXX
#define __NASALSYS_PRIVATE_HXX

#include <simgear/props/props.hxx>
#include <simgear/nasal/nasal.h>
#include <simgear/xml/easyxml.hxx>

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

#endif // of __NASALSYS_PRIVATE_HXX
