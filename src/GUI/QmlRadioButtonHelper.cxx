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

#include "QmlRadioButtonHelper.hxx"

#include <QMetaObject>
#include <QDebug>
#include <QQmlEngine>

QmlRadioButtonGroup::QmlRadioButtonGroup(QObject *parent) : QObject(parent)
{

}


QmlRadioButtonGroupAttached* QmlRadioButtonGroup::qmlAttachedProperties(QObject *object)
{
    return new QmlRadioButtonGroupAttached(object);
}

QObject *QmlRadioButtonGroup::selected() const
{
    return m_selected;
}

void QmlRadioButtonGroup::setSelected(QObject *selected)
{
    if (m_selected == selected)
        return;

    m_selected = selected;
    emit selectedChanged(m_selected);
}

QmlRadioButtonGroupAttached::QmlRadioButtonGroupAttached(QObject *pr) :
    QObject(pr)
{
}

QmlRadioButtonGroup *QmlRadioButtonGroupAttached::group() const
{
    return m_group;
}

bool QmlRadioButtonGroupAttached::isSelected() const
{
    if (!m_group)
        return false;

    return (m_group->selected() == this);
}

void QmlRadioButtonGroupAttached::setGroup(QmlRadioButtonGroup *group)
{
    if (m_group == group)
        return;

    if (m_group) {
        disconnect(m_group, &QmlRadioButtonGroup::selectedChanged,
                   this, &QmlRadioButtonGroupAttached::onGroupSelectionChanged);
    }

    m_group = group;

    if (m_group) {
        connect(m_group, &QmlRadioButtonGroup::selectedChanged,
                this, &QmlRadioButtonGroupAttached::onGroupSelectionChanged);
    }

    emit groupChanged(m_group);
    emit isSelectedChanged(isSelected());
}

void QmlRadioButtonGroupAttached::onGroupSelectionChanged()
{
    emit isSelectedChanged(isSelected());
}
