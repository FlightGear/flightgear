// scriptmgr.cxx - run user scripts
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.


#include "scriptmgr.hxx"

#include <iostream>

#include <plib/psl.h>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>

SG_USING_STD(cout);



////////////////////////////////////////////////////////////////////////
// Extensions.
////////////////////////////////////////////////////////////////////////

static pslValue
do_print (int argc, pslValue * argv, pslProgram * p)
{
    for (int i = 0; i < argc; i++) {
        switch(argv[i].getType()) {
        case PSL_INT:
            cout << argv[i].getInt();
            break;
        case PSL_FLOAT:
            cout << argv[i].getFloat();
            break;
        case PSL_STRING:
            cout << argv[i].getString();
            break;
        case PSL_VOID:
            cout << "(void)";
            break;
        default:
            cout << "(**bad value**)";
            break;
        }
    }
}

static pslValue
do_get_property (int argc, pslValue * argv, pslProgram * p)
{
    pslValue result;
    SGPropertyNode * prop = fgGetNode(argv[0].getString());
    if (prop != 0) {
        switch (prop->getType()) {
        case SGPropertyNode::BOOL:
        case SGPropertyNode::INT:
        case SGPropertyNode::LONG:
            result.set(prop->getIntValue());
            break;
        case SGPropertyNode::FLOAT:
        case SGPropertyNode::DOUBLE:
            result.set(prop->getFloatValue());
            break;
        case SGPropertyNode::STRING:
        case SGPropertyNode::UNSPECIFIED:
            result.set(prop->getStringValue());
            break;
        default:
            // DO SOMETHING
            break;
        }
    } else {
        result.set();
    }

    return result;
}

static pslValue
do_set_property (int argc, pslValue * argv, pslProgram * p)
{
    pslValue result;
    SGPropertyNode * prop = fgGetNode(argv[0].getString(), true);
    switch (argv[1].getType()) {
    case PSL_INT:
        prop->setIntValue(argv[1].getInt());
        break;
    case PSL_FLOAT:
        prop->setFloatValue(argv[1].getFloat());
        break;
    case PSL_STRING:
        prop->setStringValue(argv[1].getString());
        break;
    case PSL_VOID:
        prop->setUnspecifiedValue("");
        break;
    default:
        // TODO: report an error.
        break;
    }

    result.set();
    return result;
}

static pslExtension
extensions[] = { {"print", -1, do_print},
                 {"get_property", 1, do_get_property},
                 {"set_property", 2, do_set_property},
                 {0, 0, 0} };



////////////////////////////////////////////////////////////////////////
// Implementation of FGScriptMgr.
////////////////////////////////////////////////////////////////////////

FGScriptMgr::FGScriptMgr ()
{
}

FGScriptMgr::~FGScriptMgr ()
{
}

void
FGScriptMgr::init ()
{
    pslInit();
}

void
FGScriptMgr::update (double delta_time_sec)
{
}

bool
FGScriptMgr::run (const char * script) const
{
#if defined(FG_PSL_STRING_COMPILE)
                                // FIXME: detect and report errors
    pslProgram program(extensions);
    if (program.compile(script, globals->get_fg_root().c_str()) > 0)
        return false;
    while (program.step() != PSL_PROGRAM_END)
        ;
    return true;
#else
    SG_LOG(SG_INPUT, SG_ALERT, "Input-binding scripts not supported");
    return false;
#endif
}

bool
FGScriptMgr::run_inline (const char * script) const
{
    string s = "int main () {\n";
    s += script;
    s += "\n  return 0;\n}\n";
    return run(s.c_str());
}


// end of scriptmgr.cxx
