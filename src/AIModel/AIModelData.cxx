#include "AIBase.hxx"
#include "AIModelData.hxx"

void FGAIModelData::modelLoaded(const string& path, SGPropertyNode *prop,
                                   osg::Node *n)
{
    FGNasalModelData::modelLoaded(path, prop, n);
    // SG_LOG(SG_NASAL, SG_ALERT, "FGAIModelData::modelLoaded(" << path << ")");
    if(_base.valid())
        _base->initModel(n);
}
