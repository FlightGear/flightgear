#ifndef __MODEL_LOADER_HXX
#define __MODEL_LOADER_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/compiler.h>	// for SG_USING_STD

#include <map>
SG_USING_STD(map);

#include <string>
SG_USING_STD(string);

#include <plib/ssg.h>


/**
 * Base class for loading and managing SSG things.
 */
class FGSSGLoader
{
public:
    FGSSGLoader ();
    virtual ~FGSSGLoader ();
    virtual void flush ();
protected:
    std::map<string,ssgBase *> _table;
};


/**
 * Class for loading and managing models with XML wrappers.
 */
class FGModelLoader : public FGSSGLoader
{
public:
    FGModelLoader ();
    virtual ~FGModelLoader ();

    virtual ssgEntity * load_model( const string &fg_root,
                                    const string &path,
                                    SGPropertyNode *prop_root,
                                    double sim_time_sec );
};


#endif
