
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <cstring>

#include <simgear/nasal/nasal.h>
#include <simgear/props/props.hxx>
#include <simgear/props/vectorPropTemplates.hxx>

#include <Main/globals.hxx>

#include "NasalSys.hxx"

using namespace std;

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

naGhostType PropNodeGhostType = { propNodeGhostDestroy, "prop" };

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
    using namespace simgear;
    NODEARG();
    const char* t = "unknown";
    switch((*node)->getType()) {
    case props::NONE:   t = "NONE";   break;
    case props::ALIAS:  t = "ALIAS";  break;
    case props::BOOL:   t = "BOOL";   break;
    case props::INT:    t = "INT";    break;
    case props::LONG:   t = "LONG";   break;
    case props::FLOAT:  t = "FLOAT";  break;
    case props::DOUBLE: t = "DOUBLE"; break;
    case props::STRING: t = "STRING"; break;
    case props::UNSPECIFIED: t = "UNSPECIFIED"; break;
    case props::VEC3D:  t = "VEC3D";  break;
    case props::VEC4D:  t = "VEC4D";  break;
    case props::EXTENDED: t = "EXTENDED";  break; // shouldn't happen
    }
    return NASTR(t);
}

static naRef f_getAttribute(naContext c, naRef me, int argc, naRef* args)
{
    NODEARG();
    if(naVec_size(argv) == 0) return naNum(unsigned((*node)->getAttributes()));
    naRef val = naVec_get(argv, 0);
    const char *a = naStr_data(val);
    SGPropertyNode::Attribute attr;
    if(!a) a = "";
    if(!strcmp(a, "last")) return naNum(SGPropertyNode::LAST_USED_ATTRIBUTE);
    else if(!strcmp(a, "children"))    return naNum((*node)->nChildren());
    else if(!strcmp(a, "listeners"))   return naNum((*node)->nListeners());
    else if(!strcmp(a, "references"))  return naNum(node->getNumRefs());
    else if(!strcmp(a, "tied"))        return naNum((*node)->isTied());
    else if(!strcmp(a, "alias"))       return naNum((*node)->isAlias());
    else if(!strcmp(a, "readable"))    attr = SGPropertyNode::READ;
    else if(!strcmp(a, "writable"))    attr = SGPropertyNode::WRITE;
    else if(!strcmp(a, "archive"))     attr = SGPropertyNode::ARCHIVE;
    else if(!strcmp(a, "trace-read"))  attr = SGPropertyNode::TRACE_READ;
    else if(!strcmp(a, "trace-write")) attr = SGPropertyNode::TRACE_WRITE;
    else if(!strcmp(a, "userarchive")) attr = SGPropertyNode::USERARCHIVE;
    else if(!strcmp(a, "preserve"))    attr = SGPropertyNode::PRESERVE;
    else {
        naRuntimeError(c, "props.getAttribute() with invalid attribute");
        return naNil();
    }
    return naNum((*node)->getAttribute(attr));
}

static naRef f_setAttribute(naContext c, naRef me, int argc, naRef* args)
{
    NODEARG();
    naRef val = naVec_get(argv, 0);
    if(naVec_size(argv) == 1 && naIsNum(val)) {
        naRef ret = naNum((*node)->getAttributes());
        (*node)->setAttributes((int)val.num);
        return ret;
    }
    SGPropertyNode::Attribute attr;
    const char *a = naStr_data(val);
    if(!a) a = "";
    if(!strcmp(a, "readable"))         attr = SGPropertyNode::READ;
    else if(!strcmp(a, "writable"))    attr = SGPropertyNode::WRITE;
    else if(!strcmp(a, "archive"))     attr = SGPropertyNode::ARCHIVE;
    else if(!strcmp(a, "trace-read"))  attr = SGPropertyNode::TRACE_READ;
    else if(!strcmp(a, "trace-write")) attr = SGPropertyNode::TRACE_WRITE;
    else if(!strcmp(a, "userarchive")) attr = SGPropertyNode::USERARCHIVE;
    else if(!strcmp(a, "preserve"))    attr = SGPropertyNode::PRESERVE;
    else {
        naRuntimeError(c, "props.setAttribute() with invalid attribute");
        return naNil();
    }
    naRef ret = naNum((*node)->getAttribute(attr));
    (*node)->setAttribute(attr, naTrue(naVec_get(argv, 1)) ? true : false);
    return ret;
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

template<typename T>
naRef makeVectorFromVec(naContext c, const T& vec)
{
    const int num_components
        = sizeof(vec.data()) / sizeof(typename T::value_type);
    naRef vector = naNewVector(c);
    naVec_setsize(c, vector, num_components);
    for (int i = 0; i < num_components; ++i)
        naVec_set(vector, i, naNum(vec[i]));
    return vector;
}

static naRef f_getValue(naContext c, naRef me, int argc, naRef* args)
{
    using namespace simgear;
    NODEARG();
    switch((*node)->getType()) {
    case props::BOOL:   case props::INT:
    case props::LONG:   case props::FLOAT:
    case props::DOUBLE:
    {
        double dv = (*node)->getDoubleValue();
        if (osg::isNaN(dv)) {
          SG_LOG(SG_NASAL, SG_ALERT, "Nasal getValue: property " << (*node)->getPath() << " is NaN");
          return naNil();
        }
        
        return naNum(dv);
    }
    
    case props::STRING:
    case props::UNSPECIFIED:
        return NASTR((*node)->getStringValue());
    case props::VEC3D:
        return makeVectorFromVec(c, (*node)->getValue<SGVec3d>());
    case props::VEC4D:
        return makeVectorFromVec(c, (*node)->getValue<SGVec4d>());
    default:
        return naNil();
    }
}

template<typename T>
T makeVecFromVector(naRef vector)
{
    T vec;
    const int num_components
        = sizeof(vec.data()) / sizeof(typename T::value_type);
    int size = naVec_size(vector);

    for (int i = 0; i < num_components && i < size; ++i) {
        naRef element = naVec_get(vector, i);
        naRef n = naNumValue(element);
        if (!naIsNil(n))
            vec[i] = n.num;
    }
    return vec;
}

static naRef f_setValue(naContext c, naRef me, int argc, naRef* args)
{
    NODEARG();
    naRef val = naVec_get(argv, 0);
    bool result = false;
    if(naIsString(val)) {
        result = (*node)->setStringValue(naStr_data(val));
    } else if(naIsVector(val)) {
        if(naVec_size(val) == 3)
            result = (*node)->setValue(makeVecFromVector<SGVec3d>(val));
        else if(naVec_size(val) == 4)
            result = (*node)->setValue(makeVecFromVector<SGVec4d>(val));
        else
            naRuntimeError(c, "props.setValue() vector value has wrong size");
    } else {
        naRef n = naNumValue(val);
        if(naIsNil(n))
            naRuntimeError(c, "props.setValue() with non-number");
            
        double d = naNumValue(val).num;
        if (osg::isNaN(d)) {
          naRuntimeError(c, "props.setValue() passed a NaN");
        }
        
        result = (*node)->setDoubleValue(d);
    }
    return naNum(result);
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

    return naNum((*node)->setIntValue(iv));
}

static naRef f_setBoolValue(naContext c, naRef me, int argc, naRef* args)
{
    NODEARG();
    naRef val = naVec_get(argv, 0);
    return naNum((*node)->setBoolValue(naTrue(val) ? true : false));
}

static naRef f_setDoubleValue(naContext c, naRef me, int argc, naRef* args)
{
    NODEARG();
    naRef r = naNumValue(naVec_get(argv, 0));
    if (naIsNil(r))
        naRuntimeError(c, "props.setDoubleValue() with non-number");
        
    if (osg::isNaN(r.num)) {
      naRuntimeError(c, "props.setDoubleValue() passed a NaN");
    }
        
    return naNum((*node)->setDoubleValue(r.num));
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
    SGPropertyNode_ptr n = 0;
    try {
        n = (*node)->removeChild(naStr_data(child), (int)index.num, false);
    } catch (const string& err) {
        naRuntimeError(c, (char *)err.c_str());
    }
    return propNodeGhostCreate(c, n);
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

static naRef f_alias(naContext c, naRef me, int argc, naRef* args)
{
    NODEARG();
    SGPropertyNode* al;
    naRef prop = naVec_get(argv, 0);
    try {
        if(naIsString(prop)) al = globals->get_props()->getNode(naStr_data(prop), true);
        else if(naIsGhost(prop)) al = *(SGPropertyNode_ptr*)naGhost_ptr(prop);
        else throw string("props.alias() with bad argument");
    } catch (const string& err) {
        naRuntimeError(c, (char *)err.c_str());
        return naNil();
    }
    return naNum((*node)->alias(al));
}

static naRef f_unalias(naContext c, naRef me, int argc, naRef* args)
{
    NODEARG();
    return naNum((*node)->unalias());
}

static naRef f_getAliasTarget(naContext c, naRef me, int argc, naRef* args)
{
    NODEARG();
    return propNodeGhostCreate(c, (*node)->getAliasTarget());
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
    const char* name;
} propfuncs[] = {
    { f_getType, "_getType" },
    { f_getAttribute, "_getAttribute" },
    { f_setAttribute, "_setAttribute" },
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
    { f_alias, "_alias" },
    { f_unalias, "_unalias" },
    { f_getAliasTarget, "_getAliasTarget" },
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
