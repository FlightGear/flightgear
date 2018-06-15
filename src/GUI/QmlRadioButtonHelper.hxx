// QmlRadioButtonHelper.hxx - helper for QtQuick radio button impl
//
// Written by James Turner, started April 2018.
//
// Copyright (C) 2015 James Turner <james@flightgear.org>
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

#ifndef QMLRADIOBUTTONHELPER_HXX
#define QMLRADIOBUTTONHELPER_HXX

#include <QObject>
#include <QtQml>

class QmlRadioButtonGroup;

class QmlRadioButtonGroupAttached : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QmlRadioButtonGroup* group READ group WRITE setGroup NOTIFY groupChanged)
    Q_PROPERTY(bool isSelected READ isSelected NOTIFY isSelectedChanged)
public:
    QmlRadioButtonGroupAttached(QObject* pr = nullptr);

    QmlRadioButtonGroup* group() const;
    bool isSelected() const;

public slots:
    void setGroup(QmlRadioButtonGroup* group);

signals:
    void groupChanged(QmlRadioButtonGroup* group);
    void isSelectedChanged(bool isSelected);

private:
    void onGroupSelectionChanged();

    QmlRadioButtonGroup* m_group = nullptr;
};

class QmlRadioButtonGroup : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QObject* selected READ selected WRITE setSelected NOTIFY selectedChanged)
public:
    explicit QmlRadioButtonGroup(QObject *parent = nullptr);

    static QmlRadioButtonGroupAttached *qmlAttachedProperties(QObject *);

    QObject* selected() const;

signals:
    void selectedChanged(QObject* selected);

public slots:
    void setSelected(QObject* selected);

private:
    QObject* m_selected = nullptr;
};

QML_DECLARE_TYPEINFO(QmlRadioButtonGroup, QML_HAS_ATTACHED_PROPERTIES)

#endif // QMLRADIOBUTTONHELPER_HXX
