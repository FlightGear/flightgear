// CarrierDiagram.hxx - part of GUI launcher using Qt5
//
// Written by Stuart Buchanan, started April 2020.
//
// Copyright (C) 2022 Stuart Buchanan <stuart13@gmail.com>
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

#ifndef GUI_CARRIER_DIAGRAM_HXX
#define GUI_CARRIER_DIAGRAM_HXX

#include "BaseDiagram.hxx"
#include "QmlPositioned.hxx"
#include "UnitsModel.hxx"

#include <Navaids/navrecord.hxx>
#include <simgear/math/sg_geodesy.hxx>

class CarrierDiagram : public BaseDiagram
{
    Q_OBJECT

    Q_PROPERTY(QmlGeod geod READ geod WRITE setGeod NOTIFY locationChanged)
    Q_PROPERTY(bool offsetEnabled READ isOffsetEnabled WRITE setOffsetEnabled NOTIFY offsetChanged)
    Q_PROPERTY(bool abeam READ isAbeam WRITE setAbeam NOTIFY offsetChanged)
    Q_PROPERTY(QuantityValue offsetDistance READ offsetDistance WRITE setOffsetDistance NOTIFY offsetChanged)
public:
    CarrierDiagram(QQuickItem* pr = nullptr);

    //void setCarrier(qlonglong nav);
    //qlonglong navaid() const;

    void setGeod(QmlGeod geod);
    QmlGeod geod() const;

    void setGeod(const SGGeod& geod);

    bool isOffsetEnabled() const
    { return m_offsetEnabled; }

    bool isAbeam() const
    { return m_abeam; }

    void setOffsetEnabled(bool offset);
    void setAbeam(bool abeam);

    void setOffsetDistance(QuantityValue distance);
    QuantityValue offsetDistance() const
    { return m_offsetDistance; }

signals:
    void locationChanged();
    void offsetChanged();

protected:
    void paintContents(QPainter *) override;

    void doComputeBounds() override;
private:
    FGPositionedRef m_Carrier;
    SGGeod m_geod;

    bool m_offsetEnabled = false;
    bool m_abeam = false;
    QuantityValue m_offsetDistance;
};

#endif // of GUI_CARRIER_DIAGRAM_HXX
