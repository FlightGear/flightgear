#ifndef __FGAIMODELDATA_HXX
#define __FGAIMODELDATA_HXX

#include <osg/observer_ptr>
#include <Scripting/NasalSys.hxx>

class FGAIBase;

class FGAIModelData : public FGNasalModelData {
public:
    FGAIModelData(FGAIBase *b, SGPropertyNode *props = 0) : FGNasalModelData(props), _base(b) {}
    virtual void modelLoaded(const string& path, SGPropertyNode *prop, osg::Node *);

private:
    osg::observer_ptr<FGAIBase> _base;
};

#endif
