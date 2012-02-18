//
// Copyright (C) 2009 - 2010  Mathias Fröhlich <Mathias.Froehlich@web.de>
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

#include <string>

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

class SimTimeFactory : public SGReferenced {
public:
    virtual ~SimTimeFactory() {}
    virtual AbstractSimTime* createSimTime(sg::HLAAttributePathElementMap&) const = 0;
};

// A SimTime implementation that works with the simulation
// time in an attribute. Used when we cannot do real time management.
class AttributeSimTime : public AbstractSimTime {
public:

    virtual double getTimeStamp() const
    { return _simTime; }
    virtual void setTimeStamp(double simTime)
    { _simTime = simTime; }

    sg::HLADataElement* getDataElement()
    { return _simTime.getDataElement(); }

private:
    simgear::HLADoubleData _simTime;
};

class AttributeSimTimeFactory : public SimTimeFactory {
public:
    virtual AttributeSimTime* createSimTime(sg::HLAAttributePathElementMap& attributePathElementMap) const
    {
        AttributeSimTime* attributeSimTime = new AttributeSimTime;
        attributePathElementMap[_simTimeIndexPathPair.first][_simTimeIndexPathPair.second] = attributeSimTime->getDataElement();
        return attributeSimTime;
    }

    void setSimTimeIndexPathPair(const sg::HLADataElement::IndexPathPair& indexPathPair)
    { _simTimeIndexPathPair = indexPathPair; }

private:
    sg::HLADataElement::IndexPathPair _simTimeIndexPathPair;
};

// A SimTime implementation that works with the simulation
// time in two attributes like the avation sim net fom works
class MSecAttributeSimTime : public AbstractSimTime {
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
    simgear::HLADoubleData _secSimTime;
    simgear::HLADoubleData _msecSimTime;
};

class MSecAttributeSimTimeFactory : public SimTimeFactory {
public:
    virtual MSecAttributeSimTime* createSimTime(sg::HLAAttributePathElementMap& attributePathElementMap) const
    {
        MSecAttributeSimTime* attributeSimTime = new MSecAttributeSimTime;
        attributePathElementMap[_secIndexPathPair.first][_secIndexPathPair.second] = attributeSimTime->getSecDataElement();
        attributePathElementMap[_msecIndexPathPair.first][_msecIndexPathPair.second] = attributeSimTime->getMSecDataElement();
        return attributeSimTime;
    }

    void setSecIndexPathPair(const sg::HLADataElement::IndexPathPair& indexPathPair)
    { _secIndexPathPair = indexPathPair; }
    void setMSecIndexPathPair(const sg::HLADataElement::IndexPathPair& indexPathPair)
    { _msecIndexPathPair = indexPathPair; }

private:
    sg::HLADataElement::IndexPathPair _secIndexPathPair;
    sg::HLADataElement::IndexPathPair _msecIndexPathPair;
};

class AbstractModel : public SGReferenced {
public:
    virtual ~AbstractModel() {}

    virtual std::string getModelPath() const = 0;
    virtual void setModelPath(const std::string&) = 0;
};

class ModelFactory : public SGReferenced {
public:
    virtual ~ModelFactory() {}
    virtual AbstractModel* createModel(sg::HLAAttributePathElementMap&, bool) const = 0;
};

class AttributeModel : public AbstractModel {
public:
    virtual std::string getModelPath() const
    { return _modelPath; }
    virtual void setModelPath(const std::string& modelPath)
    { _modelPath = modelPath; }

    sg::HLADataElement* getDataElement()
    { return _modelPath.getDataElement(); }

private:
    simgear::HLAStringData _modelPath;
};

class AttributeModelFactory : public ModelFactory {
public:
    virtual AttributeModel* createModel(sg::HLAAttributePathElementMap& attributePathElementMap, bool outgoing) const
    {
        AttributeModel* attributeModel = new AttributeModel;
        attributePathElementMap[_modelIndexPathPair.first][_modelIndexPathPair.second] = attributeModel->getDataElement();
        if (outgoing)
            attributeModel->setModelPath(fgGetString("/sim/model/path", "default"));
        return attributeModel;
    }

    void setModelIndexPathPair(const sg::HLADataElement::IndexPathPair& indexPathPair)
    { _modelIndexPathPair = indexPathPair; }

private:
    sg::HLADataElement::IndexPathPair _modelIndexPathPair;
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
        simgear::HLAStringData _modelPath;
        SGSharedPtr<const AttributeMapModelFactory> _mapModelFactory;
    };

    virtual AbstractModel* createModel(sg::HLAAttributePathElementMap& attributePathElementMap, bool outgoing) const
    {
        Model* attributeModel = new Model(this);
        attributePathElementMap[_modelIndexPathPair.first][_modelIndexPathPair.second] = attributeModel->getDataElement();
        if (outgoing)
            attributeModel->setModelPath(fgGetString("/sim/model/path", "default"));
        return attributeModel;
    }

    void setModelIndexPathPair(const sg::HLADataElement::IndexPathPair& indexPathPair)
    { _modelIndexPathPair = indexPathPair; }

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
    sg::HLADataElement::IndexPathPair _modelIndexPathPair;
    std::map<std::string,std::string> _externalToModelPathMap;
    std::map<std::string,std::string> _modelPathToExternalMap;
    std::string _externalDefault;
};

// Factory class that is used to create an apternative data element for the multiplayer property attribute
class MPPropertyVariantRecordDataElementFactory : public sg::HLAVariantArrayDataElement::AlternativeDataElementFactory {
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

class MPAttributeCallback : public sg::HLAObjectInstance::AttributeCallback {
public:
    MPAttributeCallback() :
        _propertyReferenceSet(new PropertyReferenceSet),
        _mpProperties(new sg::HLAVariantArrayDataElement)
    {
        _mpProperties->setAlternativeDataElementFactory(new MPPropertyVariantRecordDataElementFactory(_propertyReferenceSet.get()));
    }
    virtual ~MPAttributeCallback()
    { }

    void setLocation(sg::HLAAbstractLocation* location)
    { _location = location; }
    sg::HLAAbstractLocation* getLocation()
    { return _location.get(); }
    const sg::HLAAbstractLocation* getLocation() const
    { return _location.get(); }

    void setModel(AbstractModel* model)
    { _model = model; }
    AbstractModel* getModel()
    { return _model.get(); }
    const AbstractModel* getModel() const
    { return _model.get(); }

    void setSimTime(AbstractSimTime* simTime)
    { _simTime = simTime; }
    AbstractSimTime* getSimTime()
    { return _simTime.get(); }
    const AbstractSimTime* getSimTime() const
    { return _simTime.get(); }

    sg::HLAVariantArrayDataElement* getMPProperties() const
    { return _mpProperties.get(); }

    SGSharedPtr<PropertyReferenceSet> _propertyReferenceSet;

protected:
    SGSharedPtr<sg::HLAAbstractLocation> _location;
    SGSharedPtr<AbstractSimTime> _simTime;
    SGSharedPtr<AbstractModel> _model;
    SGSharedPtr<sg::HLAVariantArrayDataElement> _mpProperties;
};

class MPOutAttributeCallback : public MPAttributeCallback {
public:
    virtual void updateAttributeValues(sg::HLAObjectInstance&, const sg::RTIData&)
    {
        _simTime->setTimeStamp(globals->get_sim_time_sec());

        SGGeod position = ifce.getPosition();
        // The quaternion rotating from the earth centered frame to the
        // horizontal local frame
        SGQuatd qEc2Hl = SGQuatd::fromLonLat(position);
        SGQuatd hlOr = SGQuatd::fromYawPitchRoll(ifce.get_Psi(), ifce.get_Theta(), ifce.get_Phi());
        _location->setCartPosition(SGVec3d::fromGeod(position));
        _location->setCartOrientation(qEc2Hl*hlOr);
        // The angular velocitied in the body frame
        double p = ifce.get_P_body();
        double q = ifce.get_Q_body();
        double r = ifce.get_R_body();
        _location->setAngularBodyVelocity(SGVec3d(p, q, r));
        // The body uvw velocities in the interface are wrt the wind instead
        // of wrt the ec frame
        double n = ifce.get_V_north();
        double e = ifce.get_V_east();
        double d = ifce.get_V_down();
        _location->setLinearBodyVelocity(hlOr.transform(SGVec3d(n, e, d)));

        if (_mpProperties.valid() && _mpProperties->getNumElements() == 0) {
            if (_propertyReferenceSet.valid() && _propertyReferenceSet->getRootNode()) {
                const sg::HLADataType* elementDataType = _mpProperties->getElementDataType();
                const sg::HLAVariantRecordDataType* variantRecordDataType = elementDataType->toVariantRecordDataType();
                for (unsigned i = 0, count = 0; i < variantRecordDataType->getNumAlternatives(); ++i) {
                    std::string name = variantRecordDataType->getAlternativeSemantics(i);
                    SGPropertyNode* node = _propertyReferenceSet->getRootNode()->getNode(name);
                    if (!node)
                        continue;
                    _mpProperties->getOrCreateElement(count++)->setAlternativeIndex(i);
                }
            }
        }
    }

private:
    FlightProperties ifce;
};

class MPInAttributeCallback : public MPAttributeCallback {
public:
    virtual void reflectAttributeValues(sg::HLAObjectInstance& objectInstance,
                                        const sg::RTIIndexDataPairList&, const sg::RTIData&)
    {
        // Puh, damn ordering problems with properties startup and so on
        if (_aiMultiplayer.valid()) {
            FGExternalMotionData motionInfo;
            motionInfo.time = _simTime->getTimeStamp();
            motionInfo.lag = 0;

            motionInfo.position = _location->getCartPosition();
            motionInfo.orientation = toQuatf(_location->getCartOrientation());
            motionInfo.linearVel = toVec3f(_location->getLinearBodyVelocity());
            motionInfo.angularVel = toVec3f(_location->getAngularBodyVelocity());
            motionInfo.linearAccel = SGVec3f::zeros();
            motionInfo.angularAccel = SGVec3f::zeros();

            _aiMultiplayer->addMotionInfo(motionInfo, SGTimeStamp::now().getSeconds());

        } else {
            std::string modelPath = _model->getModelPath();
            if (modelPath.empty())
                return;
            FGAIManager *aiMgr;
            aiMgr = static_cast<FGAIManager*>(globals->get_subsystem("ai-model"));
            if (!aiMgr)
                return;

            _aiMultiplayer = new FGAIMultiplayer;
            _aiMultiplayer->setPath(modelPath.c_str());
            aiMgr->attach(_aiMultiplayer.get());

            _propertyReferenceSet->setRootNode(_aiMultiplayer->getPropertyRoot());
        }
    }

    void setDie()
    {
        if (!_aiMultiplayer.valid())
            return;
        _aiMultiplayer->setDie(true);
        _aiMultiplayer = 0;
    }

private:
    SGSharedPtr<FGAIMultiplayer> _aiMultiplayer;
};


class MpClassCallback : public sg::HLAObjectClass::InstanceCallback {
public:
    MpClassCallback()
    { }
    virtual ~MpClassCallback()
    { }

    virtual void discoverInstance(const sg::HLAObjectClass& objectClass, sg::HLAObjectInstance& objectInstance, const sg::RTIData& tag)
    {
        MPInAttributeCallback* attributeCallback = new MPInAttributeCallback;
        objectInstance.setAttributeCallback(attributeCallback);
        attachDataElements(objectInstance, *attributeCallback, false);
    }

    virtual void removeInstance(const sg::HLAObjectClass& objectClass, sg::HLAObjectInstance& objectInstance, const sg::RTIData& tag)
    {
        MPInAttributeCallback* attributeCallback;
        attributeCallback = dynamic_cast<MPInAttributeCallback*>(objectInstance.getAttributeCallback().get());
        if (!attributeCallback) {
            SG_LOG(SG_IO, SG_WARN, "HLA: expected to have a different attribute callback in remove instance.");
            return;
        }
        attributeCallback->setDie();
    }

    virtual void registerInstance(const sg::HLAObjectClass& objectClass, sg::HLAObjectInstance& objectInstance)
    {
        MPOutAttributeCallback* attributeCallback = new MPOutAttributeCallback;
        objectInstance.setAttributeCallback(attributeCallback);
        attachDataElements(objectInstance, *attributeCallback, true);
        attributeCallback->_propertyReferenceSet->setRootNode(fgGetNode("/", true));
    }

    virtual void deleteInstance(const sg::HLAObjectClass& objectClass, sg::HLAObjectInstance& objectInstance)
    {
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

    void setMPPropertiesIndexPathPair(const sg::HLADataElement::IndexPathPair& indexPathPair)
    { _mpPropertiesIndexPathPair = indexPathPair; }
    const sg::HLADataElement::IndexPathPair& getMPPropertiesIndexPathPair() const
    { return _mpPropertiesIndexPathPair; }

    typedef std::map<sg::HLADataElement::Path, std::string> PathPropertyMap;
    typedef std::map<unsigned, PathPropertyMap> AttributePathPropertyMap;

    void setInputProperty(const sg::HLADataElement::IndexPathPair& indexPathPair, const std::string& property)
    {
        _inputProperties[indexPathPair.first][indexPathPair.second] = property;
    }
    void setOutputProperty(const sg::HLADataElement::IndexPathPair& indexPathPair, const std::string& property)
    {
        _outputProperties[indexPathPair.first][indexPathPair.second] = property;
    }

private:
    void attachDataElements(sg::HLAObjectInstance& objectInstance, MPAttributeCallback& attributeCallback, bool outgoing)
    {
        sg::HLAAttributePathElementMap attributePathElementMap;

        if (_locationFactory.valid())
            attributeCallback.setLocation(_locationFactory->createLocation(attributePathElementMap));
        if (_modelFactory.valid())
            attributeCallback.setModel(_modelFactory->createModel(attributePathElementMap, outgoing));
        if (_simTimeFactory.valid())
            attributeCallback.setSimTime(_simTimeFactory->createSimTime(attributePathElementMap));

        attributePathElementMap[_mpPropertiesIndexPathPair.first][_mpPropertiesIndexPathPair.second] = attributeCallback.getMPProperties();

        if (outgoing)
            attachPropertyDataElements(*attributeCallback._propertyReferenceSet,
                                       attributePathElementMap, _outputProperties);
        else
            attachPropertyDataElements(*attributeCallback._propertyReferenceSet,
                                       attributePathElementMap, _inputProperties);

        objectInstance.setAttributes(attributePathElementMap);
    }
    void attachPropertyDataElements(PropertyReferenceSet& propertyReferenceSet,
                                    sg::HLAAttributePathElementMap& attributePathElementMap,
                                    const AttributePathPropertyMap& attributePathPropertyMap)
    {
        for (AttributePathPropertyMap::const_iterator i = attributePathPropertyMap.begin();
             i != attributePathPropertyMap.end(); ++i) {
            for (PathPropertyMap::const_iterator j = i->second.begin();
                 j != i->second.end(); ++j) {
                sg::HLAPropertyDataElement* dataElement = new sg::HLAPropertyDataElement;
                propertyReferenceSet.insert(j->second, dataElement);
                attributePathElementMap[i->first][j->first] = dataElement;
            }
        }
    }

    AttributePathPropertyMap _inputProperties;
    AttributePathPropertyMap _outputProperties;

    SGSharedPtr<sg::HLALocationFactory> _locationFactory;
    SGSharedPtr<SimTimeFactory> _simTimeFactory;
    SGSharedPtr<ModelFactory> _modelFactory;

    sg::HLADataElement::IndexPathPair _mpPropertiesIndexPathPair;
};

class FGHLA::Federate : public sg::HLAFederate {
public:
    virtual ~Federate()
    { }
    virtual bool readObjectModel()
    { return readRTI1516ObjectModelTemplate(getFederationObjectModel()); }
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

    // Now that it is paramtrized, connect/join
    if (!_hlaFederate->init()) {
        SG_LOG(SG_IO, SG_ALERT, "Could not init the hla/rti connect.");
        return false;
    }

    // bool publish = get_direction() & SG_IO_OUT;
    // bool subscribe = get_direction() & SG_IO_IN;

    // This should be configured form a file
    XMLConfigReader::ObjectClassConfigList::const_iterator i;
    for (i = configReader.getObjectClassConfigList().begin();
         i != configReader.getObjectClassConfigList().end(); ++i) {

        if (i->_type != "Multiplayer") {
            SG_LOG(SG_IO, SG_ALERT, "Ignoring unknown object class type \"" << i->_type << "\"!");
            continue;
        }

        /// The object class for HLA aircraft
        SGSharedPtr<simgear::HLAObjectClass> objectClass;

        // Register the object class that we need for this simple hla implementation
        std::string aircraftClassName = i->_name;
        objectClass = _hlaFederate->getObjectClass(aircraftClassName);
        if (!objectClass.valid()) {
            SG_LOG(SG_IO, SG_ALERT, "Could not find " << aircraftClassName << " object class!");
            continue;
        }

        SGSharedPtr<MpClassCallback> mpClassCallback = new MpClassCallback;

        if (i->_positionConfig._type == "cartesian") {
            SGSharedPtr<sg::HLACartesianLocationFactory> locationFactory;
            locationFactory = new sg::HLACartesianLocationFactory;
            XMLConfigReader::DataElementList::const_iterator j;
            for (j = i->_positionConfig._dataElementList.begin();
                 j != i->_positionConfig._dataElementList.end(); ++j) {

                if (j->_type == "position-x")
                    locationFactory->setPositionIndexPathPair(0, objectClass->getIndexPathPair(j->_name));
                else if (j->_type == "position-y")
                    locationFactory->setPositionIndexPathPair(1, objectClass->getIndexPathPair(j->_name));
                else if (j->_type == "position-z")
                    locationFactory->setPositionIndexPathPair(2, objectClass->getIndexPathPair(j->_name));

                else if (j->_type == "orientation-sin-angle-axis-x")
                    locationFactory->setOrientationIndexPathPair(0, objectClass->getIndexPathPair(j->_name));
                else if (j->_type == "orientation-sin-angle-axis-y")
                    locationFactory->setOrientationIndexPathPair(1, objectClass->getIndexPathPair(j->_name));
                else if (j->_type == "orientation-sin-angle-axis-z")
                    locationFactory->setOrientationIndexPathPair(2, objectClass->getIndexPathPair(j->_name));

                else if (j->_type == "angular-velocity-x")
                    locationFactory->setAngularVelocityIndexPathPair(0, objectClass->getIndexPathPair(j->_name));
                else if (j->_type == "angular-velocity-y")
                    locationFactory->setAngularVelocityIndexPathPair(1, objectClass->getIndexPathPair(j->_name));
                else if (j->_type == "angular-velocity-z")
                    locationFactory->setAngularVelocityIndexPathPair(2, objectClass->getIndexPathPair(j->_name));

                else if (j->_type == "linear-velocity-x")
                    locationFactory->setLinearVelocityIndexPathPair(0, objectClass->getIndexPathPair(j->_name));
                else if (j->_type == "linear-velocity-y")
                    locationFactory->setLinearVelocityIndexPathPair(1, objectClass->getIndexPathPair(j->_name));
                else if (j->_type == "linear-velocity-z")
                    locationFactory->setLinearVelocityIndexPathPair(2, objectClass->getIndexPathPair(j->_name));

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
                    locationFactory->setIndexPathPair(sg::HLAGeodeticLocationFactory::LatitudeDeg,
                                                      objectClass->getIndexPathPair(j->_name));
                else if (j->_type == "latitude-rad")
                    locationFactory->setIndexPathPair(sg::HLAGeodeticLocationFactory::LatitudeRad,
                                                      objectClass->getIndexPathPair(j->_name));
                else if (j->_type == "longitude-deg")
                    locationFactory->setIndexPathPair(sg::HLAGeodeticLocationFactory::LongitudeDeg,
                                                      objectClass->getIndexPathPair(j->_name));
                else if (j->_type == "longitude-rad")
                    locationFactory->setIndexPathPair(sg::HLAGeodeticLocationFactory::LongitudeRad,
                                                      objectClass->getIndexPathPair(j->_name));
                else if (j->_type == "elevation-m")
                    locationFactory->setIndexPathPair(sg::HLAGeodeticLocationFactory::ElevationM,
                                                      objectClass->getIndexPathPair(j->_name));
                else if (j->_type == "elevation-m")
                    locationFactory->setIndexPathPair(sg::HLAGeodeticLocationFactory::ElevationFt,
                                                      objectClass->getIndexPathPair(j->_name));
                else if (j->_type == "heading-deg")
                    locationFactory->setIndexPathPair(sg::HLAGeodeticLocationFactory::HeadingDeg,
                                                      objectClass->getIndexPathPair(j->_name));
                else if (j->_type == "heading-rad")
                    locationFactory->setIndexPathPair(sg::HLAGeodeticLocationFactory::HeadingRad,
                                                      objectClass->getIndexPathPair(j->_name));
                else if (j->_type == "pitch-deg")
                    locationFactory->setIndexPathPair(sg::HLAGeodeticLocationFactory::PitchDeg,
                                                      objectClass->getIndexPathPair(j->_name));
                else if (j->_type == "pitch-rad")
                    locationFactory->setIndexPathPair(sg::HLAGeodeticLocationFactory::PitchRad,
                                                      objectClass->getIndexPathPair(j->_name));
                else if (j->_type == "roll-deg")
                    locationFactory->setIndexPathPair(sg::HLAGeodeticLocationFactory::RollDeg,
                                                      objectClass->getIndexPathPair(j->_name));
                else if (j->_type == "roll-rad")
                    locationFactory->setIndexPathPair(sg::HLAGeodeticLocationFactory::RollRad,
                                                      objectClass->getIndexPathPair(j->_name));
                else if (j->_type == "ground-track-deg")
                    locationFactory->setIndexPathPair(sg::HLAGeodeticLocationFactory::GroundTrackDeg,
                                                      objectClass->getIndexPathPair(j->_name));
                else if (j->_type == "ground-track-rad")
                    locationFactory->setIndexPathPair(sg::HLAGeodeticLocationFactory::GroundTrackRad,
                                                      objectClass->getIndexPathPair(j->_name));
                else if (j->_type == "ground-speed-kt")
                    locationFactory->setIndexPathPair(sg::HLAGeodeticLocationFactory::GroundSpeedKnots,
                                                      objectClass->getIndexPathPair(j->_name));
                else if (j->_type == "ground-speed-ft-per-sec")
                    locationFactory->setIndexPathPair(sg::HLAGeodeticLocationFactory::GroundSpeedFtPerSec,
                                                      objectClass->getIndexPathPair(j->_name));
                else if (j->_type == "ground-speed-m-per-sec")
                    locationFactory->setIndexPathPair(sg::HLAGeodeticLocationFactory::GroundSpeedMPerSec,
                                                      objectClass->getIndexPathPair(j->_name));
                else if (j->_type == "vertical-speed-ft-per-sec")
                    locationFactory->setIndexPathPair(sg::HLAGeodeticLocationFactory::VerticalSpeedFtPerSec,
                                                      objectClass->getIndexPathPair(j->_name));
                else if (j->_type == "vertical-speed-ft-per-min")
                    locationFactory->setIndexPathPair(sg::HLAGeodeticLocationFactory::VerticalSpeedFtPerMin,
                                                      objectClass->getIndexPathPair(j->_name));
                else if (j->_type == "vertical-speed-m-per-sec")
                    locationFactory->setIndexPathPair(sg::HLAGeodeticLocationFactory::VerticalSpeedMPerSec,
                                                      objectClass->getIndexPathPair(j->_name));
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
                    attributeModelFactory->setModelIndexPathPair(objectClass->getIndexPathPair(j->_name));

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
                    attributeModelFactory->setModelIndexPathPair(objectClass->getIndexPathPair(j->_name));

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
                    attributeSimTimeFactory->setSimTimeIndexPathPair(objectClass->getIndexPathPair(j->_name));
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
                    attributeSimTimeFactory->setSecIndexPathPair(objectClass->getIndexPathPair(j->_name));
                else if (j->_type == "local-simtime-msec")
                    attributeSimTimeFactory->setMSecIndexPathPair(objectClass->getIndexPathPair(j->_name));
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
            mpClassCallback->setMPPropertiesIndexPathPair(objectClass->getIndexPathPair(i->_mpPropertiesConfig._name));
        }

        // The free configurabel property - dataElement mapping
        XMLConfigReader::DataElementList::const_iterator j;
        for (j = i->_dataElementList.begin();
             j != i->_dataElementList.end(); ++j) {

            if (j->_type == "property") {
                if (!j->_inProperty.empty())
                    mpClassCallback->setInputProperty(objectClass->getIndexPathPair(j->_name), j->_inProperty);
                if (!j->_outProperty.empty())
                    mpClassCallback->setOutputProperty(objectClass->getIndexPathPair(j->_name), j->_outProperty);
            } else {
                SG_LOG(SG_IO, SG_ALERT, "HLA: Unknown dataElement configuration type \""
                       << j->_type << "\" for object class \"" << aircraftClassName << "\". Ignoring!");
            }
        }

        objectClass->setInstanceCallback(mpClassCallback);

        if (i->_type == "Multiplayer")
            _localAircraftClass = objectClass;
    }

    set_enabled(true);
    return is_enabled();
}

bool
FGHLA::process()
{
    if (!is_enabled())
        return false;

    // First push our own data so that others can recieve ...
    if (get_direction() & SG_IO_OUT) {
        if (fgGetBool("/sim/fdm-initialized", false) && _localAircraftClass.valid()) {
            if (!_localAircraftInstance.valid()) {
                _localAircraftInstance = new sg::HLAObjectInstance(_localAircraftClass.get());
                _localAircraftInstance->registerInstance();
            }
            _localAircraftInstance->updateAttributeValues(sg::RTIData("tag"));
        }
    }

    // Then get news from others and process possible update requests
    if (get_direction() & (SG_IO_IN|SG_IO_OUT)) {
        _hlaFederate->processMessages();
    }

    return true;
}

bool
FGHLA::close()
{
    if (!is_enabled())
        return false;

    if (get_direction() & SG_IO_OUT) {
        // Remove the local object from the rti
        _localAircraftInstance->deleteInstance(simgear::RTIData("gone"));
        _localAircraftInstance = 0;
    }

    // Leave the federation and try to destroy the federation execution.
    _hlaFederate->shutdown();

    set_enabled(false);

    return true;
}

