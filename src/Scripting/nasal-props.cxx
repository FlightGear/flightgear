
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <simgear/nasal/nasal.h>
#include <simgear/props/props.hxx>

#include <Main/globals.hxx>

#include "NasalSys.hxx"

// Implementation of a Nasal wrapper for the SGPropertyNode class,
// using the Nasal "ghost" (er... Garbage collection Handle for
// OutSide Thingy) facility.
//
// Note that these functions appear in Nasal with prepended
// underscores.  They work on the low-level "ghost" objects and aren't
// intended for use from user code, but from Nasal code you will find
// in props.nas.  That is where the Nasal props.Node class is defined,
// which provides a saner interface along the lines of SGPropertyNode.

static void propNodeGhostDestroy(void* ghost)
{
    SGPropertyNode_ptr* prop = (SGPropertyNode_ptr*)ghost;
    delete prop;
}

naGhostType PropNodeGhostType = { propNodeGhostDestroy };

static naRef propNodeGhostCreate(naContext c, SGPropertyNode* n)
{
    if(!n) return naNil();
    SGPropertyNode_ptr* ghost = new SGPropertyNode_ptr(n);
    return naNewGhost(c, &PropNodeGhostType, ghost);
}

naRef FGNasalSys::propNodeGhost(SGPropertyNode* handle)
{
    return propNodeGhostCreate(_context, handle);
}

#define NASTR(s) s ? naStr_fromdata(naNewString(c),(char*)(s),strlen(s)) : naNil()

//
// Standard header for the extension functions.  It turns the "ghost"
// found in arg[0] into a SGPropertyNode_ptr*, and then "unwraps" the
// vector found in the second argument into a normal-looking args
// array.  This allows the Nasal handlers to do things like:
//   Node.getChild = func { _getChild(me.ghost, arg) }
//
#define NODEARG()                                                       \
    if(argc < 2 || !naIsGhost(args[0]) ||                               \
       naGhost_type(args[0]) != &PropNodeGhostType)                       \
        naRuntimeError(c, "bad argument to props function");            \
    SGPropertyNode_ptr* node = (SGPropertyNode_ptr*)naGhost_ptr(args[0]); \
    naRef argv = args[1]

static naRef f_getType(naContext c, naRef me, int argc, naRef* args)
{
    NODEARG();
    char* t = "unknown";
    switch((*node)->getType()) {
    case SGPropertyNode::NONE:   t = "NONE";   break;
    case SGPropertyNode::ALIAS:  t = "ALIAS";  break;
    case SGPropertyNode::BOOL:   t = "BOOL";   break;
    case SGPropertyNode::INT:    t = "INT";    break;
    case SGPropertyNode::LONG:   t = "LONG";   break;
    case SGPropertyNode::FLOAT:  t = "FLOAT";  break;
    case SGPropertyNode::DOUBLE: t = "DOUBLE"; break;
    case SGPropertyNode::STRING: t = "STRING"; break;
    case SGPropertyNode::UNSPECIFIED: t = "UNSPECIFIED"; break;
    }
    return NASTR(t);
}

static naRef f_getName(naContext c, naRef me, int argc, naRef* args)
{
    NODEARG();
    return NASTR((*node)->getName());
}

static naRef f_getIndex(naContext c, naRef me, int argc, naRef* args)
{
    NODEARG();
    return naNum((*node)->getIndex());
}

static naRef f_getValue(naContext c, naRef me, int argc, naRef* args)
{
    NODEARG();
    switch((*node)->getType()) {
    case SGPropertyNode::BOOL:   case SGPropertyNode::INT:
    case SGPropertyNode::LONG:   case SGPropertyNode::FLOAT:
    case SGPropertyNode::DOUBLE:
        return naNum((*node)->getDoubleValue());
    case SGPropertyNode::STRING:
    case SGPropertyNode::UNSPECIFIED:
        return NASTR((*node)->getStringValue());
    default:
        return naNil();
    }
}

static naRef f_setValue(naContext c, naRef me, int argc, naRef* args)
{
    NODEARG();
    naRef val = naVec_get(argv, 0);
    if(naIsString(val)) (*node)->setStringValue(naStr_data(val));
    else {
        naRef n = naNumValue(val);
        if(naIsNil(n))
            naRuntimeError(c, "props.setValue() with non-number");
        (*node)->setDoubleValue(naNumValue(val).num);
    }
    return naNil();
}

static naRef f_setIntValue(naContext c, naRef me, int argc, naRef* args)
{
    NODEARG();
    // Original code:
    //   int iv = (int)naNumValue(naVec_get(argv, 0)).num;

    // Junk to pacify the gcc-2.95.3 optimizer:
    naRef tmp0 = naVec_get(argv, 0);
    naRef tmp1 = naNumValue(tmp0);
    if(naIsNil(tmp1))
        naRuntimeError(c, "props.setIntValue() with non-number");
    double tmp2 = tmp1.num;
    int iv = (int)tmp2;

    (*node)->setIntValue(iv);
    return naNil();
}

static naRef f_setBoolValue(naContext c, naRef me, int argc, naRef* args)
{
    NODEARG();
    naRef val = naVec_get(argv, 0);
    (*node)->setBoolValue(naTrue(val) ? true : false);
    return naNil();
}

static naRef f_setDoubleValue(naContext c, naRef me, int argc, naRef* args)
{
    NODEARG();
    naRef r = naNumValue(naVec_get(argv, 0));
    if(naIsNil(r))
        naRuntimeError(c, "props.setDoubleValue() with non-number");
    (*node)->setDoubleValue(r.num);
    return naNil();
}

static naRef f_getParent(naContext c, naRef me, int argc, naRef* args)
{
    NODEARG();
    SGPropertyNode* n = (*node)->getParent();
    if(!n) return naNil();
    return propNodeGhostCreate(c, n);
}

static naRef f_getChild(naContext c, naRef me, int argc, naRef* args)
{
    NODEARG();
    naRef child = naVec_get(argv, 0);
    if(!naIsString(child)) return naNil();
    naRef idx = naNumValue(naVec_get(argv, 1));
    bool create = naTrue(naVec_get(argv, 2));
    SGPropertyNode* n;
    try {
        if(naIsNil(idx) || !naIsNum(idx)) {
            n = (*node)->getChild(naStr_data(child), create);
        } else {
            n = (*node)->getChild(naStr_data(child), (int)idx.num, create);
        }
    } catch (const string& err) {
        naRuntimeError(c, (char *)err.c_str());
        return naNil();
    }
    if(!n) return naNil();
    return propNodeGhostCreate(c, n);
}

static naRef f_getChildren(naContext c, naRef me, int argc, naRef* args)
{
    NODEARG();
    naRef result = naNewVector(c);
    if(naIsNil(argv) || naVec_size(argv) == 0) {
        // Get all children
        for(int i=0; i<(*node)->nChildren(); i++)
            naVec_append(result, propNodeGhostCreate(c, (*node)->getChild(i)));
    } else {
        // Get all children of a specified name
        naRef name = naVec_get(argv, 0);
        if(!naIsString(name)) return naNil();
        try {
            vector<SGPropertyNode_ptr> children
                = (*node)->getChildren(naStr_data(name));
            for(unsigned int i=0; i<children.size(); i++)
                naVec_append(result, propNodeGhostCreate(c, children[i]));
        } catch (const string& err) {
            naRuntimeError(c, (char *)err.c_str());
            return naNil();
        }
    }
    return result;
}

static naRef f_removeChild(naContext c, naRef me, int argc, naRef* args)
{
    NODEARG();
    naRef child = naVec_get(argv, 0);
    naRef index = naVec_get(argv, 1);
    if(!naIsString(child) || !naIsNum(index)) return naNil();
    try {
        (*node)->removeChild(naStr_data(child), (int)index.num, false);
    } catch (const string& err) {
        naRuntimeError(c, (char *)err.c_str());
    }
    return naNil();
}

static naRef f_removeChildren(naContext c, naRef me, int argc, naRef* args)
{
    NODEARG();
    naRef result = naNewVector(c);
    if(naIsNil(argv) || naVec_size(argv) == 0) {
        // Remove all children
        for(int i = (*node)->nChildren() - 1; i >=0; i--)
            naVec_append(result, propNodeGhostCreate(c, (*node)->removeChild(i)));
    } else {
        // Remove all children of a specified name
        naRef name = naVec_get(argv, 0);
        if(!naIsString(name)) return naNil();
        try {
            vector<SGPropertyNode_ptr> children
                = (*node)->removeChildren(naStr_data(name), false);
            for(unsigned int i=0; i<children.size(); i++)
                naVec_append(result, propNodeGhostCreate(c, children[i]));
        } catch (const string& err) {
            naRuntimeError(c, (char *)err.c_str());
            return naNil();
        }
    }
    return result;
}

static naRef f_getNode(naContext c, naRef me, int argc, naRef* args)
{
    NODEARG();
    naRef path = naVec_get(argv, 0);
    bool create = naTrue(naVec_get(argv, 1));
    if(!naIsString(path)) return naNil();
    SGPropertyNode* n;
    try {
        n = (*node)->getNode(naStr_data(path), create);
    } catch (const string& err) {
        naRuntimeError(c, (char *)err.c_str());
        return naNil();
    }
    return propNodeGhostCreate(c, n);
}

static naRef f_new(naContext c, naRef me, int argc, naRef* args)
{
    return propNodeGhostCreate(c, new SGPropertyNode());
}

static naRef f_globals(naContext c, naRef me, int argc, naRef* args)
{
    return propNodeGhostCreate(c, globals->get_props());
}

static struct {
    naCFunction func;
    char* name;
} propfuncs[] = {
    { f_getType, "_getType" },
    { f_getName, "_getName" },
    { f_getIndex, "_getIndex" },
    { f_getValue, "_getValue" },
    { f_setValue, "_setValue" },
    { f_setIntValue, "_setIntValue" },
    { f_setBoolValue, "_setBoolValue" },
    { f_setDoubleValue, "_setDoubleValue" },
    { f_getParent, "_getParent" },
    { f_getChild, "_getChild" },
    { f_getChildren, "_getChildren" },
    { f_removeChild, "_removeChild" },
    { f_removeChildren, "_removeChildren" },
    { f_getNode, "_getNode" },
    { f_new, "_new" },
    { f_globals, "_globals" },
    { 0, 0 }
};

naRef FGNasalSys::genPropsModule()
{
    naRef namespc = naNewHash(_context);
    for(int i=0; propfuncs[i].name; i++)
        hashset(namespc, propfuncs[i].name,
                naNewFunc(_context, naNewCCode(_context, propfuncs[i].func)));
    return namespc;
}
