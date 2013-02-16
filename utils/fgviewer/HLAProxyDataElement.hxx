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

#ifndef HLAProxyDataElement_hxx
#define HLAProxyDataElement_hxx

#include <simgear/hla/HLADataElement.hxx>

namespace simgear {

class HLAProxyDataElement : public HLADataElement {
public:
    virtual ~HLAProxyDataElement()
    { }

    virtual void accept(HLADataElementVisitor& visitor)
    {
        HLADataElement* dataElement = _getDataElement();
        if (!dataElement)
            return;
        dataElement->accept(visitor);
    }
    virtual void accept(HLAConstDataElementVisitor& visitor) const
    {
        const HLADataElement* dataElement = _getDataElement();
        if (!dataElement)
            return;
        dataElement->accept(visitor);
    }

    virtual bool encode(HLAEncodeStream& stream) const
    {
        const HLADataElement* dataElement = _getDataElement();
        if (!dataElement)
            return false;
        return dataElement->encode(stream);
    }
    virtual bool decode(HLADecodeStream& stream)
    {
        HLADataElement* dataElement = _getDataElement();
        if (!dataElement)
            return false;
        return dataElement->decode(stream);
    }

    virtual const HLADataType* getDataType() const
    {
        const HLADataElement* dataElement = _getDataElement();
        if (!dataElement)
            return 0;
        return dataElement->getDataType();
    }
    virtual bool setDataType(const HLADataType* dataType)
    {
        HLADataElement* dataElement = _getDataElement();
        if (!dataElement)
            return false;
        return dataElement->setDataType(dataType);
    }

protected:
    virtual HLADataElement* _getDataElement() = 0;
    virtual const HLADataElement* _getDataElement() const = 0;

    virtual void _setStamp(Stamp* stamp)
    {
        HLADataElement::_setStamp(stamp);
        HLADataElement* dataElement = _getDataElement();
        if (!dataElement)
            return;
        dataElement->attachStamp(*this);
    }
};

}

#endif
