// Copyright (C) 2009 - 2012  Mathias Froehlich
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

#ifndef HLAObjectReferenceData_hxx
#define HLAObjectReferenceData_hxx

#include <simgear/hla/HLAArrayDataElement.hxx>
#include <simgear/hla/HLADataElement.hxx>
#include <simgear/hla/HLAFederate.hxx>

#include "HLAProxyDataElement.hxx"

namespace simgear {

/// Data element that references an other object instance by its name, or object instance handle?
/// Abstract variant that is object type independent and could be registered at the federate
/// if the object is not found immediately and might arrive later.
class HLAAbstractObjectReferenceDataElement : public HLAProxyDataElement {
public:
    // FIXME drop the federate once we have a decode/encode visitor?!
    HLAAbstractObjectReferenceDataElement(const SGWeakPtr<HLAFederate>& federate) :
        _federate(federate)
    { }

    virtual bool decode(HLADecodeStream& stream)
    {
        if (!HLAProxyDataElement::decode(stream))
            return false;

        // No change?
        if (!_name.getDataElement()->getDirty())
            return true;
        _name.getDataElement()->setDirty(false);

        return recheckObject();
    }

    /// Returns true if the object is correctly set
    bool getComplete() const
    {
        if (HLAObjectInstance* objectInstance = _getObjectInstance()) {
            return _name.getValue() == objectInstance->getName();
        } else {
            return _name.getValue().empty();
        }
    }

    bool recheckObject()
    {
        HLAObjectInstance* objectInstance = _getObjectInstance();
        if (objectInstance && _name.getValue() == objectInstance->getName())
            return true;

        if (_name.getValue().empty()) {
            return _setObjectInstance(0);
        } else {
            // Get the object by its name from the federate
            SGSharedPtr<HLAFederate> federate = _federate.lock();
            if (!federate.valid())
                return false;
            SGSharedPtr<HLAObjectInstance> objectInstance;
            objectInstance = federate->getObjectInstance(_name.getValue());
            return _setObjectInstance(objectInstance.get());
        }
    }

protected:
    virtual HLAStringDataElement* _getDataElement()
    { return _name.getDataElement(); }
    virtual const HLAStringDataElement* _getDataElement() const
    { return _name.getDataElement(); }

    virtual HLAObjectInstance* _getObjectInstance() const = 0;
    virtual bool _setObjectInstance(HLAObjectInstance*) = 0;

    void _setName(const HLAObjectInstance* objectInstance)
    {
        if (objectInstance)
            _name.setValue(objectInstance->getName());
        else
            _name.setValue(std::string());
        _name.getDataElement()->setDirty(true);
    }

private:
    HLAStringData _name;
    SGWeakPtr<HLAFederate> _federate;
};


template<typename T>
class HLAObjectReferenceDataElement : public HLAAbstractObjectReferenceDataElement {
public:
    // FIXME drop the federate once we have a decode/encode visitor?!
    HLAObjectReferenceDataElement(const SGWeakPtr<HLAFederate>& federate) :
        HLAAbstractObjectReferenceDataElement(federate)
    { }

    const SGSharedPtr<T>& getObject() const
    { return _object; }
    void setObject(const SGSharedPtr<T>& object)
    {
        if (_object == object)
            return;
        _setName(object.get());
        _object = object;
    }

protected:
    virtual HLAObjectInstance* _getObjectInstance() const
    {
        return _object.get();
    }
    virtual bool _setObjectInstance(HLAObjectInstance* objectInstance)
    {
        if (!objectInstance) {
            _object.clear();
            return true;
        } else {
            if (!dynamic_cast<T*>(objectInstance))
                return false;
            _object = static_cast<T*>(objectInstance);
            return true;
        }
    }

private:
    SGSharedPtr<T> _object;
};

template<typename T>
class HLAObjectReferenceData {
public:
    typedef HLAObjectReferenceDataElement<T> DataElement;

    HLAObjectReferenceData(const SGWeakPtr<HLAFederate>& federate) :
        _dataElement(new DataElement(federate))
    { }

    const HLADataElement* getDataElement() const
    { return _dataElement.get(); }
    HLADataElement* getDataElement()
    { return _dataElement.get(); }

    const SGSharedPtr<T>& getObject() const
    { return _dataElement->getObject(); }
    void setObject(const SGSharedPtr<T>& object)
    { _dataElement->setObject(object); }

    bool getComplete() const
    { return _dataElement->getComplete(); }
    /// FIXME this must happen in the federate by registering object references
    /// that still want to be resolved
    bool recheckObject() const
    { return _dataElement->recheckObject(); }

private:
    SGSharedPtr<DataElement> _dataElement;
};

}

#endif
