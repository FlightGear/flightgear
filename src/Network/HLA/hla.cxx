//
// Copyright (C) 2009 - 2012  Mathias Fröhlich <Mathias.Froehlich@web.de>
//
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
//

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "hla.hxx"

#include <simgear/compiler.h>

#include <algorithm>
#include <list>
#include <map>
#include <string>
#include <stack>

#include <simgear/debug/logstream.hxx>
#include <simgear/math/SGMath.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/props/props.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/xml/easyxml.hxx>

#include <simgear/hla/HLAFederate.hxx>
#include <simgear/hla/HLAArrayDataElement.hxx>
#include <simgear/hla/HLADataElement.hxx>
#include <simgear/hla/HLADataType.hxx>
#include <simgear/hla/HLALocation.hxx>
#include <simgear/hla/HLAObjectClass.hxx>
#include <simgear/hla/HLAObjectInstance.hxx>
#include <simgear/hla/HLAPropertyDataElement.hxx>
#include <simgear/hla/HLAVariantRecordDataElement.hxx>

#include <AIModel/AIMultiplayer.hxx>
#include <AIModel/AIManager.hxx>

#include <FDM/flightProperties.hxx>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>

// FIXME rename HLAMP
// FIXME make classes here private ...

namespace sg = simgear;

enum HLAVersion {
    RTI13,
    RTI1516,
    RTI1516E
};

class FGHLA::XMLConfigReader : public XMLVisitor {
public:
    XMLConfigReader() :
        _rtiVersion(RTI13)
    { }

    const std::string& getFederateObjectModel() const
    { return _federateObjectModel; }

    HLAVersion getRTIVersion() const
    { return _rtiVersion; }
    const std::list<std::string>& getRTIArguments() const
    { return _rtiArguments; }

    struct DataElement {
        std::string _type;
        std::string _name;
        std::string _inProperty;
        std::string _outProperty;
    };
    typedef std::list<DataElement> DataElementList;
    struct ModelConfig {
        std::string _type;
        DataElementList _dataElementList;
        std::list<std::pair<std::string, std::string> > _modelMap;
    };
    struct SimTimeConfig {
        std::string _type;
        DataElementList _dataElementList;
    };
    struct PositionConfig {
        std::string _type;
        DataElementList _dataElementList;
    };
    struct MPPropertiesConfig {
        std::string _name;
    };
    struct ObjectClassConfig {
        std::string _name;
        std::string _type;
        ModelConfig _modelConfig;
        SimTimeConfig _simTimeConfig;
        PositionConfig _positionConfig;
        MPPropertiesConfig _mpPropertiesConfig;
        DataElementList _dataElementList;
    };
    typedef std::list<ObjectClassConfig> ObjectClassConfigList;

    const ObjectClassConfigList& getObjectClassConfigList() const
    { return _objectClassConfigList; }

protected:
    virtual void startXML()
    {
        _modeStack.push(TopLevelMode);
    }
    virtual void endXML()
    {
        _modeStack.pop();
    }
    virtual void startElement(const char* name, const XMLAttributes& atts)
    {
        if (strcmp(name, "hlaConfiguration") == 0) {
            if (!isModeStackTop(TopLevelMode))
                throw sg_exception("hlaConfiguration tag not at top level");

            _modeStack.push(HLAConfigurationMode);
        }

        else if (strcmp(name, "rti") == 0) {
            if (!isModeStackTop(HLAConfigurationMode))
                throw sg_exception("rti tag not at top level");

            std::string rtiVersion = getAttribute("version", atts);
            if (rtiVersion == "RTI13")
                _rtiVersion = RTI13;
            else if (rtiVersion == "RTI1516")
                _rtiVersion = RTI1516;
            else if (rtiVersion == "RTI1516E")
                _rtiVersion = RTI1516E;
            else
                throw sg_exception("invalid version attribute in rti tag");

            _modeStack.push(RTIMode);
        }
        else if (strcmp(name, "argument") == 0) {
            if (!isModeStackTop(RTIMode))
                throw sg_exception("argument tag not inside an rti tag");

            _modeStack.push(RTIArgumentMode);
            _rtiArguments.push_back(std::string());
        }

        else if (strcmp(name, "objects") == 0) {
            if (!isModeStackTop(HLAConfigurationMode))
                throw sg_exception("objects tag not at top level");

            _modeStack.push(ObjectsMode);
        }
        else if (strcmp(name, "objectClass") == 0) {
            // Currently this is flat and only allows one class to be configured
            if (!isModeStackTop(ObjectsMode))
                throw sg_exception("objects tag not inside an objects tag");
            if (!_objectClassConfigList.empty())
                throw sg_exception("currently only one objectClass is allowed");
            // if (!isModeStackTop(ObjectsMode) && !isModeStackTop(ObjectClassMode))
            //     throw sg_exception("objects tag not inside an objects or objectClass tag");

            ObjectClassConfig objectClassConfig;
            objectClassConfig._type = getAttribute("type", atts);
            objectClassConfig._name = getAttribute("name", atts);
            _objectClassConfigList.push_back(objectClassConfig);

            _modeStack.push(ObjectClassMode);
        }
        else if (strcmp(name, "model") == 0) {
            if (!isModeStackTop(ObjectClassMode))
                throw sg_exception("position tag not inside an objectClass tag");

            _objectClassConfigList.back()._modelConfig._type = getAttribute("type", atts);

            _modeStack.push(ModelMode);
        }
        else if (strcmp(name, "key") == 0) {
            if (!isModeStackTop(ModelMode))
                throw sg_exception("key tag not inside an model tag");

            std::pair<std::string,std::string> p;
            p.first = getAttribute("external", atts);
            p.second = getAttribute("modelPath", atts);
            _objectClassConfigList.back()._modelConfig._modelMap.push_back(p);

            _modeStack.push(KeyMode);
        }
        else if (strcmp(name, "simTime") == 0) {
            if (!isModeStackTop(ObjectClassMode))
                throw sg_exception("simTime tag not inside an objectClass tag");

            _objectClassConfigList.back()._simTimeConfig._type = getAttribute("type", atts);

            _modeStack.push(SimTimeMode);
        }
        else if (strcmp(name, "position") == 0) {
            if (!isModeStackTop(ObjectClassMode))
                throw sg_exception("position tag not inside an objectClass tag");

            _objectClassConfigList.back()._positionConfig._type = getAttribute("type", atts);

            _modeStack.push(PositionMode);
        }
        else if (strcmp(name, "mpProperties") == 0) {
            if (!isModeStackTop(ObjectClassMode))
                throw sg_exception("mpProperties tag not inside an objectClass tag");

            _objectClassConfigList.back()._mpPropertiesConfig._name = getAttribute("name", atts);

            _modeStack.push(MPPropertiesMode);
        }
        else if (strcmp(name, "dataElement") == 0) {

            DataElement dataElement;
            dataElement._type = getAttribute("type", atts);
            dataElement._name = getAttribute("name", atts);
            if (isModeStackTop(ModelMode)) {
                _objectClassConfigList.back()._modelConfig._dataElementList.push_back(dataElement);
            } else if (isModeStackTop(SimTimeMode)) {
                _objectClassConfigList.back()._simTimeConfig._dataElementList.push_back(dataElement);
            } else if (isModeStackTop(PositionMode)) {
                _objectClassConfigList.back()._positionConfig._dataElementList.push_back(dataElement);
            } else if (isModeStackTop(ObjectClassMode)) {
                std::string io = getAttribute("io", atts);
                dataElement._inProperty = getAttribute("in", atts);
                if (dataElement._inProperty.empty())
                    dataElement._inProperty = io;
                dataElement._outProperty = getAttribute("out", atts);
                if (dataElement._outProperty.empty())
                    dataElement._outProperty = io;
                _objectClassConfigList.back()._dataElementList.push_back(dataElement);
            } else {
                throw sg_exception("dataElement tag not inside a position, model or objectClass tag");
            }

            _modeStack.push(DataElementMode);
        }

        else if (strcmp(name, "federateObjectModel") == 0) {
            if (!isModeStackTop(HLAConfigurationMode))
                throw sg_exception("federateObjectModel tag not at top level");

            _federateObjectModel = getAttribute("name", atts);
            _modeStack.push(FederateObjectModelMode);
        }

        else {
            throw sg_exception(std::string("Unknown tag ") + name);
        }
    }

    virtual void data(const char * s, int length)
    {
        if (isModeStackTop(RTIArgumentMode)) {
            _rtiArguments.back().append(s, length);
        }
    }

    virtual void endElement(const char* name)
    {
        _modeStack.pop();
    }

    std::string getAttribute(const char* name, const XMLAttributes& atts)
    {
        int index = atts.findAttribute(name);
        if (index < 0 || atts.size() <= index)
            return std::string();
        return std::string(atts.getValue(index));
    }
    std::string getAttribute(const std::string& name, const XMLAttributes& atts)
    {
        int index = atts.findAttribute(name.c_str());
        if (index < 0 || atts.size() <= index)
            return std::string();
        return std::string(atts.getValue(index));
    }

private:
    // poor peoples xml validation ...
    enum Mode {
        TopLevelMode,
        HLAConfigurationMode,
        RTIMode,
        RTIArgumentMode,
        ObjectsMode,
        ObjectClassMode,
        ModelMode,
        KeyMode,
        SimTimeMode,
        PositionMode,
        MPPropertiesMode,
        DataElementMode,
        FederateObjectModelMode
    };
    bool isModeStackTop(Mode mode)
    {
        if (_modeStack.empty())
            return false;
        return _modeStack.top() == mode;
    }
    std::stack<Mode> _modeStack;

    std::string _federateObjectModel;
    HLAVersion _rtiVersion;
    std::list<std::string> _rtiArguments;

    ObjectClassConfigList _objectClassConfigList;
};

class PropertyReferenceSet : public SGReferenced {
public:
    void insert(const std::string& relativePath, const SGSharedPtr<sg::HLAPropertyDataElement>& dataElement)
    {
        if (_rootNode.valid())
            dataElement->setPropertyNode(_rootNode->getNode(relativePath, true));
        _pathDataElementPairList.push_back(PathDataElementPair(relativePath, dataElement));
    }

    void setRootNode(SGPropertyNode* rootNode)
    {
        _rootNode = rootNode;
        for (PathDataElementPairList::iterator i = _pathDataElementPairList.begin();
             i != _pathDataElementPairList.end(); ++i) {
            i->second->setPropertyNode(_rootNode->getNode(i->first, true));
        }
    }
    SGPropertyNode* getRootNode()
    { return _rootNode.get(); }

private:
    SGSharedPtr<SGPropertyNode> _rootNode;

    typedef std::pair<std::string, SGSharedPtr<sg::HLAPropertyDataElement> > PathDataElementPair;
    typedef std::list<PathDataElementPair> PathDataElementPairList;
    PathDataElementPairList _pathDataElementPairList;
};

class AbstractSimTime : public SGReferenced {
public:
    virtual ~AbstractSimTime() {}

    virtual double getTimeStamp() const = 0;
    virtual void setTimeStamp(double) = 0;
};

class AbstractModel : public SGReferenced {
public:
    virtual ~AbstractModel() {}

    virtual std::string getModelPath() const = 0;
    virtual void setModelPath(const std::string&) = 0;
};

// Factory class that is used to create an apternative data element for the multiplayer property attribute
class MPPropertyVariantRecordDataElementFactory :
    public sg::HLAVariantArrayDataElement::AlternativeDataElementFactory {
public:
    MPPropertyVariantRecordDataElementFactory(PropertyReferenceSet* propertyReferenceSet) :
        _propertyReferenceSet(propertyReferenceSet)
    { }

    virtual sg::HLADataElement* createElement(const sg::HLAVariantRecordDataElement& variantRecordDataElement, unsigned index)
    {
        const sg::HLAVariantRecordDataType* dataType = variantRecordDataElement.getDataType();
        if (!dataType)
            return 0;
        const sg::HLAEnumeratedDataType* enumDataType = dataType->getEnumeratedDataType();
        if (!enumDataType)
            return 0;
        const sg::HLADataType* alternativeDataType = dataType->getAlternativeDataType(index);
        if (!alternativeDataType)
            return 0;

        // The relative property path should be in the semantics field name
        std::string relativePath = dataType->getAlternativeSemantics(index);
        sg::HLAPropertyDataElement* dataElement = new sg::HLAPropertyDataElement(alternativeDataType, (SGPropertyNode*)0);
        _propertyReferenceSet->insert(relativePath, dataElement);
        return dataElement;
    }

private:
    SGSharedPtr<PropertyReferenceSet> _propertyReferenceSet;
};

class FGHLA::MultiplayerObjectInstance : public sg::HLAObjectInstance {
public:
    MultiplayerObjectInstance(sg::HLAObjectClass* objectClass) :
        sg::HLAObjectInstance(objectClass),
        _propertyReferenceSet(new PropertyReferenceSet),
        _mpProperties(new sg::HLAVariantArrayDataElement)
    {
        _mpProperties->setAlternativeDataElementFactory(new MPPropertyVariantRecordDataElementFactory(_propertyReferenceSet.get()));
    }
    virtual ~MultiplayerObjectInstance()
    { }

    SGSharedPtr<PropertyReferenceSet> _propertyReferenceSet;
    SGSharedPtr<sg::HLAAbstractLocation> _location;
    SGSharedPtr<AbstractSimTime> _simTime;
    SGSharedPtr<AbstractModel> _model;
    SGSharedPtr<sg::HLAVariantArrayDataElement> _mpProperties;
};

class FGHLA::MPUpdateCallback : public sg::HLAObjectInstance::UpdateCallback {
public:
    virtual void updateAttributeValues(sg::HLAObjectInstance& objectInstance, const sg::RTIData& tag)
    {
        updateAttributeValues(static_cast<MultiplayerObjectInstance&>(objectInstance));
        objectInstance.encodeAttributeValues();
        objectInstance.sendAttributeValues(tag);
    }

    virtual void updateAttributeValues(sg::HLAObjectInstance& objectInstance, const SGTimeStamp& timeStamp, const sg::RTIData& tag)
    {
        updateAttributeValues(static_cast<MultiplayerObjectInstance&>(objectInstance));
        objectInstance.encodeAttributeValues();
        objectInstance.sendAttributeValues(timeStamp, tag);
    }

    void updateAttributeValues(MultiplayerObjectInstance& objectInstance)
    {
        objectInstance._simTime->setTimeStamp(globals->get_sim_time_sec());

        SGGeod position = _ifce.getPosition();
        // The quaternion rotating from the earth centered frame to the
        // horizontal local frame
        SGQuatd qEc2Hl = SGQuatd::fromLonLat(position);
        SGQuatd hlOr = SGQuatd::fromYawPitchRoll(_ifce.get_Psi(), _ifce.get_Theta(), _ifce.get_Phi());
        objectInstance._location->setCartPosition(SGVec3d::fromGeod(position));
        objectInstance._location->setCartOrientation(qEc2Hl*hlOr);
        // The angular velocitied in the body frame
        double p = _ifce.get_P_body();
        double q = _ifce.get_Q_body();
        double r = _ifce.get_R_body();
        objectInstance._location->setAngularBodyVelocity(SGVec3d(p, q, r));
        // The body uvw velocities in the interface are wrt the wind instead
        // of wrt the ec frame
        double n = _ifce.get_V_north()*SG_FEET_TO_METER;
        double e = _ifce.get_V_east()*SG_FEET_TO_METER;
        double d = _ifce.get_V_down()*SG_FEET_TO_METER;
        objectInstance._location->setLinearBodyVelocity(hlOr.transform(SGVec3d(n, e, d)));

        if (objectInstance._mpProperties.valid() && objectInstance._mpProperties->getNumElements() == 0) {
            if (objectInstance._propertyReferenceSet.valid() && objectInstance._propertyReferenceSet->getRootNode()) {
                const sg::HLADataType* elementDataType = objectInstance._mpProperties->getElementDataType();
                const sg::HLAVariantRecordDataType* variantRecordDataType = elementDataType->toVariantRecordDataType();
                for (unsigned i = 0, count = 0; i < variantRecordDataType->getNumAlternatives(); ++i) {
                    std::string name = variantRecordDataType->getAlternativeSemantics(i);
                    SGPropertyNode* node = objectInstance._propertyReferenceSet->getRootNode()->getNode(name);
                    if (!node)
                        continue;
                    objectInstance._mpProperties->getOrCreateElement(count++)->setAlternativeIndex(i);
                }
            }
        }
    }

    FlightProperties _ifce;
};

class FGHLA::MPReflectCallback : public sg::HLAObjectInstance::ReflectCallback {
public:
    virtual void reflectAttributeValues(sg::HLAObjectInstance& objectInstance,
                                        const sg::HLAIndexList& indexList, const sg::RTIData& tag)
    {
        objectInstance.reflectAttributeValues(indexList, tag);
        reflectAttributeValues(static_cast<MultiplayerObjectInstance&>(objectInstance));
    }
    virtual void reflectAttributeValues(sg::HLAObjectInstance& objectInstance, const sg::HLAIndexList& indexList,
                                        const SGTimeStamp& timeStamp, const sg::RTIData& tag)
    {
        objectInstance.reflectAttributeValues(indexList, timeStamp, tag);
        reflectAttributeValues(static_cast<MultiplayerObjectInstance&>(objectInstance));
    }

    void reflectAttributeValues(MultiplayerObjectInstance& objectInstance)
    {
        // Puh, damn ordering problems with properties startup and so on
        if (_aiMultiplayer.valid()) {
            FGExternalMotionData motionInfo;
            motionInfo.time = objectInstance._simTime->getTimeStamp();
            motionInfo.lag = 0;

            motionInfo.position = objectInstance._location->getCartPosition();
            motionInfo.orientation = toQuatf(objectInstance._location->getCartOrientation());
            motionInfo.linearVel = toVec3f(objectInstance._location->getLinearBodyVelocity());
            motionInfo.angularVel = toVec3f(objectInstance._location->getAngularBodyVelocity());
            motionInfo.linearAccel = SGVec3f::zeros();
            motionInfo.angularAccel = SGVec3f::zeros();

            _aiMultiplayer->addMotionInfo(motionInfo, SGTimeStamp::now().getSeconds());

        } else {
            std::string modelPath = objectInstance._model->getModelPath();
            if (modelPath.empty())
                return;
            FGAIManager *aiMgr;
            aiMgr = static_cast<FGAIManager*>(globals->get_subsystem("ai-model"));
            if (!aiMgr)
                return;

            _aiMultiplayer = new FGAIMultiplayer;
            _aiMultiplayer->setPath(modelPath.c_str());
            aiMgr->attach(_aiMultiplayer.get());

            objectInstance._propertyReferenceSet->setRootNode(_aiMultiplayer->getPropertyRoot());
        }
    }

    void setDie()
    {
        if (!_aiMultiplayer.valid())
            return;
        _aiMultiplayer->setDie(true);
        _aiMultiplayer = 0;
    }

    SGSharedPtr<FGAIMultiplayer> _aiMultiplayer;
};

class SimTimeFactory : public SGReferenced {
public:
    virtual ~SimTimeFactory() {}
    virtual AbstractSimTime* createSimTime(sg::HLAObjectInstance&) const = 0;
};

// A SimTime implementation that works with the simulation
// time in an attribute. Used when we cannot do real time management.
class AttributeSimTimeFactory : public SimTimeFactory {
public:
    class SimTime : public AbstractSimTime {
    public:
        virtual double getTimeStamp() const
        { return _simTime; }
        virtual void setTimeStamp(double simTime)
        { _simTime = simTime; }
        
        sg::HLADataElement* getDataElement()
        { return _simTime.getDataElement(); }
        
    private:
        sg::HLADoubleData _simTime;
    };

    virtual AbstractSimTime* createSimTime(sg::HLAObjectInstance& objectInstance) const
    {
        SimTime* simTime = new SimTime;
        objectInstance.setAttributeDataElement(_simTimeIndex, simTime->getDataElement());
        return simTime;
    }
    void setSimTimeIndex(const sg::HLADataElementIndex& simTimeIndex)
    { _simTimeIndex = simTimeIndex; }

private:
    sg::HLADataElementIndex _simTimeIndex;
};

// A SimTime implementation that works with the simulation
// time in two attributes like the avation sim net fom works
class MSecAttributeSimTimeFactory : public SimTimeFactory {
public:
    class SimTime : public AbstractSimTime {
    public:
        virtual double getTimeStamp() const
        {
            return _secSimTime + 1e-3*_msecSimTime;
        }
        virtual void setTimeStamp(double simTime)
        {
            double sec = floor(simTime);
            _secSimTime = sec;
            _msecSimTime = 1e3*(simTime - sec);
        }
        
        sg::HLADataElement* getSecDataElement()
        { return _secSimTime.getDataElement(); }
        sg::HLADataElement* getMSecDataElement()
        { return _msecSimTime.getDataElement(); }
        
    private:
        sg::HLADoubleData _secSimTime;
        sg::HLADoubleData _msecSimTime;
    };
    
    virtual AbstractSimTime* createSimTime(sg::HLAObjectInstance& objectInstance) const
    {
        SimTime* simTime = new SimTime;
        objectInstance.setAttributeDataElement(_secIndex, simTime->getSecDataElement());
        objectInstance.setAttributeDataElement(_msecIndex, simTime->getMSecDataElement());
        return simTime;
    }
    void setSecIndex(const sg::HLADataElementIndex& secIndex)
    { _secIndex = secIndex; }
    void setMSecIndex(const sg::HLADataElementIndex& msecIndex)
    { _msecIndex = msecIndex; }

private:
    sg::HLADataElementIndex _secIndex;
    sg::HLADataElementIndex _msecIndex;
};

class ModelFactory : public SGReferenced {
public:
    virtual ~ModelFactory() {}
    virtual AbstractModel* createModel(sg::HLAObjectInstance& objectInstance) const = 0;
};

class AttributeModelFactory : public ModelFactory {
public:
    class Model : public AbstractModel {
    public:
        virtual std::string getModelPath() const
        { return _modelPath; }
        virtual void setModelPath(const std::string& modelPath)
        { _modelPath = modelPath; }
        
        sg::HLADataElement* getDataElement()
        { return _modelPath.getDataElement(); }
        
    private:
        sg::HLAStringData _modelPath;
    };

    virtual AbstractModel* createModel(sg::HLAObjectInstance& objectInstance) const
    {
        Model* model = new Model;
        objectInstance.setAttributeDataElement(_modelIndex, model->getDataElement());
        return model;
    }
    void setModelIndex(const sg::HLADataElementIndex& modelIndex)
    { _modelIndex = modelIndex; }

private:
    sg::HLADataElementIndex _modelIndex;
};

class AttributeMapModelFactory : public ModelFactory {
public:
    class Model : public AbstractModel {
    public:
        Model(const AttributeMapModelFactory* mapModelFactory) :
            _mapModelFactory(mapModelFactory)
        { }

        virtual std::string getModelPath() const
        {
            if (_modelPath.getValue().empty())
                return _modelPath.getValue();
            return _mapModelFactory->mapToFlightgear(_modelPath);
        }
        virtual void setModelPath(const std::string& modelPath)
        { _modelPath = _mapModelFactory->mapToExternal(modelPath); }

        sg::HLADataElement* getDataElement()
        { return _modelPath.getDataElement(); }

    private:
        sg::HLAStringData _modelPath;
        SGSharedPtr<const AttributeMapModelFactory> _mapModelFactory;
    };

    virtual AbstractModel* createModel(sg::HLAObjectInstance& objectInstance) const
    {
        Model* model = new Model(this);
        objectInstance.setAttributeDataElement(_modelIndex, model->getDataElement());
        return model;
    }
    void setModelIndex(const sg::HLADataElementIndex& modelIndex)
    { _modelIndex = modelIndex; }

    std::string mapToFlightgear(const std::string& externalModel) const
    {
        std::map<std::string,std::string>::const_iterator i;
        i = _externalToModelPathMap.find(externalModel);
        if (i != _externalToModelPathMap.end())
            return i->second;
        return "default";
    }
    std::string mapToExternal(const std::string& modelPath) const
    {
        std::map<std::string,std::string>::const_iterator i;
        i = _modelPathToExternalMap.find(modelPath);
        if (i != _modelPathToExternalMap.end())
            return i->second;
        return _externalDefault;
    }

    void setExternalDefault(const std::string& externalDefault)
    { _externalDefault = externalDefault; }
    void addExternalModelPathPair(const std::string& external, const std::string& modelPath)
    {
        _externalToModelPathMap[external] = modelPath;
        _modelPathToExternalMap[modelPath] = external;
    }

private:
    sg::HLADataElementIndex _modelIndex;

    std::map<std::string,std::string> _externalToModelPathMap;
    std::map<std::string,std::string> _modelPathToExternalMap;
    std::string _externalDefault;
};

class FGHLA::MultiplayerObjectClass : public sg::HLAObjectClass {
public:
    MultiplayerObjectClass(const std::string& name, sg::HLAFederate* federate) :
        HLAObjectClass(name, federate)
    { }
    virtual ~MultiplayerObjectClass()
    { }

    virtual MultiplayerObjectInstance* createObjectInstance(const std::string& name)
    { return new MultiplayerObjectInstance(this); }

    virtual void discoverInstance(sg::HLAObjectInstance& objectInstance, const sg::RTIData& tag)
    {
        HLAObjectClass::discoverInstance(objectInstance, tag);
        MPReflectCallback* reflectCallback = new MPReflectCallback;
        objectInstance.setReflectCallback(reflectCallback);
        attachDataElements(static_cast<MultiplayerObjectInstance&>(objectInstance), false);
    }
    virtual void removeInstance(sg::HLAObjectInstance& objectInstance, const sg::RTIData& tag)
    {
        HLAObjectClass::removeInstance(objectInstance, tag);
        MPReflectCallback* reflectCallback;
        reflectCallback = dynamic_cast<MPReflectCallback*>(objectInstance.getReflectCallback().get());
        if (!reflectCallback) {
            SG_LOG(SG_IO, SG_WARN, "HLA: expected to have a different attribute callback in remove instance.");
            return;
        }
        reflectCallback->setDie();
    }
    virtual void registerInstance(sg::HLAObjectInstance& objectInstance)
    {
        HLAObjectClass::registerInstance(objectInstance);
        MPUpdateCallback* updateCallback = new MPUpdateCallback;
        objectInstance.setUpdateCallback(updateCallback);
        MultiplayerObjectInstance& mpObjectInstance = static_cast<MultiplayerObjectInstance&>(objectInstance);
        attachDataElements(mpObjectInstance, true);
        mpObjectInstance._model->setModelPath(fgGetString("/sim/model/path", "default"));
        mpObjectInstance._propertyReferenceSet->setRootNode(fgGetNode("/", true));
    }
    virtual void deleteInstance(sg::HLAObjectInstance& objectInstance)
    {
        HLAObjectClass::deleteInstance(objectInstance);
    }

    virtual void createAttributeDataElements(sg::HLAObjectInstance& objectInstance)
    {
        sg::HLAObjectClass::createAttributeDataElements(objectInstance);
    }

    void setLocationFactory(sg::HLALocationFactory* locationFactory)
    { _locationFactory = locationFactory; }
    sg::HLALocationFactory* getLocationFactory()
    { return _locationFactory.get(); }

    void setSimTimeFactory(SimTimeFactory* simTimeFactory)
    { _simTimeFactory = simTimeFactory; }
    SimTimeFactory* getSimTimeFactory()
    { return _simTimeFactory.get(); }

    void setModelFactory(ModelFactory* modelFactory)
    { _modelFactory = modelFactory; }
    ModelFactory* getModelFactory()
    { return _modelFactory.get(); }

    void setMPPropertiesIndex(const sg::HLADataElementIndex& index)
    { _mpPropertiesIndex = index; }
    const sg::HLADataElementIndex& getMPPropertiesIndex() const
    { return _mpPropertiesIndex; }

    void setInputProperty(const sg::HLADataElementIndex& index, const std::string& property)
    {
        _inputProperties[index] = property;
    }
    void setOutputProperty(const sg::HLADataElementIndex& index, const std::string& property)
    {
        _outputProperties[index] = property;
    }

private:
    void attachDataElements(MultiplayerObjectInstance& objectInstance, bool outgoing)
    {
        objectInstance.setAttributeDataElement(_mpPropertiesIndex, objectInstance._mpProperties.get());

        if (_locationFactory.valid())
            objectInstance._location = _locationFactory->createLocation(objectInstance);
        if (_modelFactory.valid())
            objectInstance._model = _modelFactory->createModel(objectInstance);
        if (_simTimeFactory.valid())
            objectInstance._simTime = _simTimeFactory->createSimTime(objectInstance);

        if (outgoing)
            attachPropertyDataElements(*objectInstance._propertyReferenceSet,
                                       objectInstance, _outputProperties);
        else
            attachPropertyDataElements(*objectInstance._propertyReferenceSet,
                                       objectInstance, _inputProperties);
    }
    typedef std::map<sg::HLADataElementIndex, std::string> IndexPropertyMap;

    void attachPropertyDataElements(PropertyReferenceSet& propertyReferenceSet,
                                    sg::HLAObjectInstance& objectInstance,
                                    const IndexPropertyMap& attributePathPropertyMap)
    {
        for (IndexPropertyMap::const_iterator i = attributePathPropertyMap.begin();
             i != attributePathPropertyMap.end(); ++i) {
            sg::HLAPropertyDataElement* dataElement = new sg::HLAPropertyDataElement;
            propertyReferenceSet.insert(i->second, dataElement);
            objectInstance.setAttributeDataElement(i->first, dataElement);
        }
    }

    IndexPropertyMap _inputProperties;
    IndexPropertyMap _outputProperties;

    SGSharedPtr<sg::HLALocationFactory> _locationFactory;
    SGSharedPtr<SimTimeFactory> _simTimeFactory;
    SGSharedPtr<ModelFactory> _modelFactory;

    sg::HLADataElementIndex _mpPropertiesIndex;
};

class FGHLA::Federate : public sg::HLAFederate {
public:
    virtual ~Federate()
    { }
    virtual bool readObjectModel()
    { return readRTI1516ObjectModelTemplate(getFederationObjectModel()); }

    virtual sg::HLAObjectClass* createObjectClass(const std::string& name)
    {
        if (std::find(_multiplayerObjectClassNames.begin(), _multiplayerObjectClassNames.end(), name)
            != _multiplayerObjectClassNames.end()) {
            if (_localAircraftClass.valid()) {
                return new MultiplayerObjectClass(name, this);
            } else {
                _localAircraftClass = new MultiplayerObjectClass(name, this);
                return _localAircraftClass.get();
            }
        } else {
            return 0;
        }
    }

    void updateLocalAircraftInstance()
    {
        // First push our own data so that others can recieve ...
        if (!_localAircraftClass.valid())
            return;
        if (!_localAircraftInstance.valid()) {
            _localAircraftInstance = new MultiplayerObjectInstance(_localAircraftClass.get());
            _localAircraftInstance->registerInstance();
        }
        _localAircraftInstance->updateAttributeValues(sg::RTIData("MPAircraft"));
    }

    virtual bool shutdown()
    {
        if (_localAircraftInstance.valid()) {
            // Remove the local object from the rti
            _localAircraftInstance->deleteInstance(simgear::RTIData("gone"));
            _localAircraftInstance = 0;
        }
        return HLAFederate::shutdown();
    }

    std::list<std::string> _multiplayerObjectClassNames;
    /// This class is used to register the local instance and to subscribe for others
    SGSharedPtr<MultiplayerObjectClass> _localAircraftClass;
    /// The local aircraft instance
    SGSharedPtr<MultiplayerObjectInstance> _localAircraftInstance;
};

FGHLA::FGHLA(const std::vector<std::string>& tokens) :
    _hlaFederate(new Federate)
{
    if (1 < tokens.size() && !tokens[1].empty())
        set_direction(tokens[1]);
    else
        set_direction("bi");

    int rate = 60;
    if (2 < tokens.size() && !tokens[2].empty()) {
        std::stringstream ss(tokens[2]);
        ss >> rate;
    }
    set_hz(rate);

    if (3 < tokens.size() && !tokens[3].empty())
        _federate = tokens[3];
    else
        _federate = fgGetString("/sim/user/callsign");

    if (4 < tokens.size() && !tokens[4].empty())
        _federation = tokens[4];
    else
        _federation = "FlightGear";

    if (5 < tokens.size() && !tokens[5].empty())
        _objectModelConfig = tokens[5];
    else
        _objectModelConfig = "mp-aircraft.xml";
}

FGHLA::~FGHLA()
{
}

bool
FGHLA::open()
{
    if (is_enabled()) {
	SG_LOG(SG_IO, SG_ALERT, "This shouldn't happen, but the channel "
		"is already in use, ignoring");
	return false;
    }

    if (_federation.empty()) {
	SG_LOG(SG_IO, SG_ALERT, "No federation to join given to "
               "the HLA module");
	return false;
    }

    if (_federate.empty()) {
	SG_LOG(SG_IO, SG_ALERT, "No federate name given to the HLA module");
	return false;
    }

    if (!SGPath(_objectModelConfig).exists()) {
        SGPath path(globals->get_fg_root());
        path.append("HLA");
        path.append(_objectModelConfig);
        if (path.exists())
            _objectModelConfig = path.str();
        else {
            SG_LOG(SG_IO, SG_ALERT, "Federate object model configuration \""
                   << _objectModelConfig << "\" does not exist.");
            return false;
        }
    }

    XMLConfigReader configReader;
    try {
        readXML(_objectModelConfig, configReader);
    } catch (const sg_throwable& e) {
	SG_LOG(SG_IO, SG_ALERT, "Could not open HLA configuration file: "
               << e.getMessage());
	return false;
    } catch (...) {
	SG_LOG(SG_IO, SG_ALERT, "Could not open HLA configuration file");
	return false;
    }

    // Flightgears hla configuration
    std::string objectModel = configReader.getFederateObjectModel();
    if (objectModel.empty()) {
	SG_LOG(SG_IO, SG_ALERT, "No federate object model given to "
               "the HLA module");
	return false;
    }
    if (!SGPath(objectModel).exists()) {
        SGPath path(SGPath(_objectModelConfig).dir());
        path.append(objectModel);
        if (path.exists())
            objectModel = path.str();
        else {
            SG_LOG(SG_IO, SG_ALERT, "Federate object model file \""
                   << objectModel << "\" does not exist.");
            return false;
        }
    }

    // We need that to communicate to the rti
    switch (configReader.getRTIVersion()) {
    case RTI13:
        _hlaFederate->setVersion(simgear::HLAFederate::RTI13);
        break;
    case RTI1516:
        _hlaFederate->setVersion(simgear::HLAFederate::RTI1516);
        break;
    case RTI1516E:
        _hlaFederate->setVersion(simgear::HLAFederate::RTI1516E);
        break;
    }
    _hlaFederate->setConnectArguments(configReader.getRTIArguments());
    _hlaFederate->setFederationExecutionName(_federation);
    _hlaFederate->setFederationObjectModel(objectModel);
    _hlaFederate->setFederateType(_federate);

    // Store the multiplayer class name in the federate
    XMLConfigReader::ObjectClassConfigList::const_iterator i;
    for (i = configReader.getObjectClassConfigList().begin();
         i != configReader.getObjectClassConfigList().end(); ++i) {

        if (i->_type != "Multiplayer") {
            SG_LOG(SG_IO, SG_ALERT, "Ignoring unknown object class type \"" << i->_type << "\"!");
            continue;
        }

        // Register the object class that we need for this simple hla implementation
        _hlaFederate->_multiplayerObjectClassNames.push_back(i->_name);
    }


    // Now that it is paramtrized, connect/join
    if (!_hlaFederate->init()) {
        SG_LOG(SG_IO, SG_ALERT, "Could not init the hla/rti connect.");
        return false;
    }

    // bool publish = get_direction() & SG_IO_OUT;
    // bool subscribe = get_direction() & SG_IO_IN;

    // Interpret the configuration file
    for (i = configReader.getObjectClassConfigList().begin();
         i != configReader.getObjectClassConfigList().end(); ++i) {

        /// already warned about this above
        if (i->_type != "Multiplayer")
            continue;

        /// The object class for HLA aircraft
        SGSharedPtr<MultiplayerObjectClass> objectClass;

        // Register the object class that we need for this simple hla implementation
        std::string aircraftClassName = i->_name;
        objectClass = dynamic_cast<MultiplayerObjectClass*>(_hlaFederate->getObjectClass(aircraftClassName));
        if (!objectClass.valid()) {
            SG_LOG(SG_IO, SG_ALERT, "Could not find " << aircraftClassName << " object class!");
            continue;
        }

        SGSharedPtr<MultiplayerObjectClass> mpClassCallback = objectClass;

        if (i->_positionConfig._type == "cartesian") {
            SGSharedPtr<sg::HLACartesianLocationFactory> locationFactory;
            locationFactory = new sg::HLACartesianLocationFactory;
            XMLConfigReader::DataElementList::const_iterator j;
            for (j = i->_positionConfig._dataElementList.begin();
                 j != i->_positionConfig._dataElementList.end(); ++j) {

                if (j->_type == "position-x")
                    locationFactory->setPositionIndex(0, objectClass->getDataElementIndex(j->_name));
                else if (j->_type == "position-y")
                    locationFactory->setPositionIndex(1, objectClass->getDataElementIndex(j->_name));
                else if (j->_type == "position-z")
                    locationFactory->setPositionIndex(2, objectClass->getDataElementIndex(j->_name));

                else if (j->_type == "orientation-sin-angle-axis-x")
                    locationFactory->setOrientationIndex(0, objectClass->getDataElementIndex(j->_name));
                else if (j->_type == "orientation-sin-angle-axis-y")
                    locationFactory->setOrientationIndex(1, objectClass->getDataElementIndex(j->_name));
                else if (j->_type == "orientation-sin-angle-axis-z")
                    locationFactory->setOrientationIndex(2, objectClass->getDataElementIndex(j->_name));

                else if (j->_type == "angular-velocity-x")
                    locationFactory->setAngularVelocityIndex(0, objectClass->getDataElementIndex(j->_name));
                else if (j->_type == "angular-velocity-y")
                    locationFactory->setAngularVelocityIndex(1, objectClass->getDataElementIndex(j->_name));
                else if (j->_type == "angular-velocity-z")
                    locationFactory->setAngularVelocityIndex(2, objectClass->getDataElementIndex(j->_name));

                else if (j->_type == "linear-velocity-x")
                    locationFactory->setLinearVelocityIndex(0, objectClass->getDataElementIndex(j->_name));
                else if (j->_type == "linear-velocity-y")
                    locationFactory->setLinearVelocityIndex(1, objectClass->getDataElementIndex(j->_name));
                else if (j->_type == "linear-velocity-z")
                    locationFactory->setLinearVelocityIndex(2, objectClass->getDataElementIndex(j->_name));

                else {
                    SG_LOG(SG_IO, SG_ALERT, "HLA: Unknown position configuration type \""
                           << j->_type << "\"for object class \"" << aircraftClassName << "\". Ignoring!");
                }
            }

            mpClassCallback->setLocationFactory(locationFactory.get());
        } else if (i->_positionConfig._type == "geodetic") {
            SGSharedPtr<sg::HLAGeodeticLocationFactory> locationFactory;
            locationFactory = new sg::HLAGeodeticLocationFactory;

            XMLConfigReader::DataElementList::const_iterator j;
            for (j = i->_positionConfig._dataElementList.begin();
                 j != i->_positionConfig._dataElementList.end(); ++j) {

                if (j->_type == "latitude-deg")
                    locationFactory->setIndex(sg::HLAGeodeticLocationFactory::LatitudeDeg,
                                              objectClass->getDataElementIndex(j->_name));
                else if (j->_type == "latitude-rad")
                    locationFactory->setIndex(sg::HLAGeodeticLocationFactory::LatitudeRad,
                                              objectClass->getDataElementIndex(j->_name));
                else if (j->_type == "longitude-deg")
                    locationFactory->setIndex(sg::HLAGeodeticLocationFactory::LongitudeDeg,
                                              objectClass->getDataElementIndex(j->_name));
                else if (j->_type == "longitude-rad")
                    locationFactory->setIndex(sg::HLAGeodeticLocationFactory::LongitudeRad,
                                              objectClass->getDataElementIndex(j->_name));
                else if (j->_type == "elevation-m")
                    locationFactory->setIndex(sg::HLAGeodeticLocationFactory::ElevationM,
                                              objectClass->getDataElementIndex(j->_name));
                else if (j->_type == "elevation-m")
                    locationFactory->setIndex(sg::HLAGeodeticLocationFactory::ElevationFt,
                                              objectClass->getDataElementIndex(j->_name));
                else if (j->_type == "heading-deg")
                    locationFactory->setIndex(sg::HLAGeodeticLocationFactory::HeadingDeg,
                                              objectClass->getDataElementIndex(j->_name));
                else if (j->_type == "heading-rad")
                    locationFactory->setIndex(sg::HLAGeodeticLocationFactory::HeadingRad,
                                              objectClass->getDataElementIndex(j->_name));
                else if (j->_type == "pitch-deg")
                    locationFactory->setIndex(sg::HLAGeodeticLocationFactory::PitchDeg,
                                              objectClass->getDataElementIndex(j->_name));
                else if (j->_type == "pitch-rad")
                    locationFactory->setIndex(sg::HLAGeodeticLocationFactory::PitchRad,
                                              objectClass->getDataElementIndex(j->_name));
                else if (j->_type == "roll-deg")
                    locationFactory->setIndex(sg::HLAGeodeticLocationFactory::RollDeg,
                                              objectClass->getDataElementIndex(j->_name));
                else if (j->_type == "roll-rad")
                    locationFactory->setIndex(sg::HLAGeodeticLocationFactory::RollRad,
                                              objectClass->getDataElementIndex(j->_name));
                else if (j->_type == "ground-track-deg")
                    locationFactory->setIndex(sg::HLAGeodeticLocationFactory::GroundTrackDeg,
                                              objectClass->getDataElementIndex(j->_name));
                else if (j->_type == "ground-track-rad")
                    locationFactory->setIndex(sg::HLAGeodeticLocationFactory::GroundTrackRad,
                                              objectClass->getDataElementIndex(j->_name));
                else if (j->_type == "ground-speed-kt")
                    locationFactory->setIndex(sg::HLAGeodeticLocationFactory::GroundSpeedKnots,
                                              objectClass->getDataElementIndex(j->_name));
                else if (j->_type == "ground-speed-ft-per-sec")
                    locationFactory->setIndex(sg::HLAGeodeticLocationFactory::GroundSpeedFtPerSec,
                                              objectClass->getDataElementIndex(j->_name));
                else if (j->_type == "ground-speed-m-per-sec")
                    locationFactory->setIndex(sg::HLAGeodeticLocationFactory::GroundSpeedMPerSec,
                                              objectClass->getDataElementIndex(j->_name));
                else if (j->_type == "vertical-speed-ft-per-sec")
                    locationFactory->setIndex(sg::HLAGeodeticLocationFactory::VerticalSpeedFtPerSec,
                                              objectClass->getDataElementIndex(j->_name));
                else if (j->_type == "vertical-speed-ft-per-min")
                    locationFactory->setIndex(sg::HLAGeodeticLocationFactory::VerticalSpeedFtPerMin,
                                              objectClass->getDataElementIndex(j->_name));
                else if (j->_type == "vertical-speed-m-per-sec")
                    locationFactory->setIndex(sg::HLAGeodeticLocationFactory::VerticalSpeedMPerSec,
                                              objectClass->getDataElementIndex(j->_name));
                else {
                    SG_LOG(SG_IO, SG_ALERT, "HLA: Unknown position configuration type \""
                           << j->_type << "\" for object class \"" << aircraftClassName << "\". Ignoring!");
                }
            }

            mpClassCallback->setLocationFactory(locationFactory.get());
        }

        if (i->_modelConfig._type == "native") {
            SGSharedPtr<AttributeModelFactory> attributeModelFactory;
            attributeModelFactory = new AttributeModelFactory;

            XMLConfigReader::DataElementList::const_iterator j;
            for (j = i->_modelConfig._dataElementList.begin();
                 j != i->_modelConfig._dataElementList.end(); ++j) {

                if (j->_type == "model-path")
                    attributeModelFactory->setModelIndex(objectClass->getDataElementIndex(j->_name));

                else {
                    SG_LOG(SG_IO, SG_ALERT, "HLA: Unknown model configuration type \""
                           << j->_type << "\" for object class \"" << aircraftClassName << "\". Ignoring!");
                }
            }

            mpClassCallback->setModelFactory(attributeModelFactory.get());
        } else if (i->_modelConfig._type == "map") {

            SGSharedPtr<AttributeMapModelFactory> attributeModelFactory;
            attributeModelFactory = new AttributeMapModelFactory;

            XMLConfigReader::DataElementList::const_iterator j;
            for (j = i->_modelConfig._dataElementList.begin();
                 j != i->_modelConfig._dataElementList.end(); ++j) {

                if (j->_type == "external")
                    attributeModelFactory->setModelIndex(objectClass->getDataElementIndex(j->_name));

                else {
                    SG_LOG(SG_IO, SG_ALERT, "HLA: Unknown model configuration type \""
                           << j->_type << "\" for object class \"" << aircraftClassName << "\". Ignoring!");
                }
            }

            std::list<std::pair<std::string, std::string> >::const_iterator k;
            for (k = i->_modelConfig._modelMap.begin();
                 k != i->_modelConfig._modelMap.end(); ++k) {

                if (k->second.empty())
                    attributeModelFactory->setExternalDefault(k->first);
                else
                    attributeModelFactory->addExternalModelPathPair(k->first, k->second);
            }

            mpClassCallback->setModelFactory(attributeModelFactory.get());

        } else {
            SG_LOG(SG_IO, SG_ALERT, "HLA: Unknown model configuration type \""
                   << i->_modelConfig._type << "\" for object class \"" << aircraftClassName << "\". Ignoring!");
        }

        if (i->_simTimeConfig._type == "attribute") {
            SGSharedPtr<AttributeSimTimeFactory> attributeSimTimeFactory;
            attributeSimTimeFactory = new AttributeSimTimeFactory;

            XMLConfigReader::DataElementList::const_iterator j;
            for (j = i->_simTimeConfig._dataElementList.begin();
                 j != i->_simTimeConfig._dataElementList.end(); ++j) {

                if (j->_type == "local-simtime")
                    attributeSimTimeFactory->setSimTimeIndex(objectClass->getDataElementIndex(j->_name));
                else {
                    SG_LOG(SG_IO, SG_ALERT, "HLA: Unknown simtime configuration type \""
                           << j->_type << "\" for object class \"" << aircraftClassName << "\". Ignoring!");
                }
            }

            mpClassCallback->setSimTimeFactory(attributeSimTimeFactory.get());
        } else if (i->_simTimeConfig._type == "attribute-sec-msec") {
            SGSharedPtr<MSecAttributeSimTimeFactory> attributeSimTimeFactory;
            attributeSimTimeFactory = new MSecAttributeSimTimeFactory;

            XMLConfigReader::DataElementList::const_iterator j;
            for (j = i->_simTimeConfig._dataElementList.begin();
                 j != i->_simTimeConfig._dataElementList.end(); ++j) {

                if (j->_type == "local-simtime-sec")
                    attributeSimTimeFactory->setSecIndex(objectClass->getDataElementIndex(j->_name));
                else if (j->_type == "local-simtime-msec")
                    attributeSimTimeFactory->setMSecIndex(objectClass->getDataElementIndex(j->_name));
                else {
                    SG_LOG(SG_IO, SG_ALERT, "HLA: Unknown simtime configuration type \""
                           << j->_type << "\" for object class \"" << aircraftClassName << "\". Ignoring!");
                }
            }

            mpClassCallback->setSimTimeFactory(attributeSimTimeFactory.get());
        } else {
            SG_LOG(SG_IO, SG_ALERT, "HLA: Unknown simtime configuration type \""
                   << i->_simTimeConfig._type << "\" for object class \"" << aircraftClassName << "\". Ignoring!");
        }

        if (!i->_mpPropertiesConfig._name.empty()) {
            mpClassCallback->setMPPropertiesIndex(objectClass->getDataElementIndex(i->_mpPropertiesConfig._name));
        }

        // The free configurabel property - dataElement mapping
        XMLConfigReader::DataElementList::const_iterator j;
        for (j = i->_dataElementList.begin();
             j != i->_dataElementList.end(); ++j) {

            if (j->_type == "property") {
                if (!j->_inProperty.empty())
                    mpClassCallback->setInputProperty(objectClass->getDataElementIndex(j->_name), j->_inProperty);
                if (!j->_outProperty.empty())
                    mpClassCallback->setOutputProperty(objectClass->getDataElementIndex(j->_name), j->_outProperty);
            } else {
                SG_LOG(SG_IO, SG_ALERT, "HLA: Unknown dataElement configuration type \""
                       << j->_type << "\" for object class \"" << aircraftClassName << "\". Ignoring!");
            }
        }
    }

    set_enabled(true);
    return is_enabled();
}

bool
FGHLA::process()
{
    if (!is_enabled())
        return false;

    if (!_hlaFederate.valid())
        return false;

    // First push our own data so that others can recieve ...
    if (get_direction() & SG_IO_OUT)
        _hlaFederate->updateLocalAircraftInstance();

    // Then get news from others and process possible update requests
    _hlaFederate->update();

    return true;
}

bool
FGHLA::close()
{
    if (!is_enabled())
        return false;

    if (!_hlaFederate.valid())
        return false;

    // Leave the federation and try to destroy the federation execution.
    _hlaFederate->shutdown();
    _hlaFederate = 0;

    set_enabled(false);

    return true;
}

