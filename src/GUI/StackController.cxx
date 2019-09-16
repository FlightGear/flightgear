// StackController.cxx - manage a stack of QML items
//
// Written by James Turner, started February 2019
//
// Copyright (C) 2019 James Turner <james@flightgear.org>
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

#include "StackController.hxx"

StackController::StackController()
{
}

QUrl StackController::currentPageSource() const
{
    if (m_stack.empty())
        return {};
    return m_stack.front();
}

QString StackController::currentPageTitle() const
{
    if (m_stack.empty())
        return {};
    return m_titles.front();
}

QString StackController::previousPageTitle() const
{
    if (m_titles.size() < 2)
        return {};
    return m_titles.at(1);
}

bool StackController::canGoBack() const
{
    return (m_stack.size() > 1);
}

void StackController::push(QUrl page, QString title)
{
    m_stack.push_front(page);
    m_titles.push_front(title);
    emit currentPageChanged();
}

void StackController::pop()
{
    if (m_stack.empty())
        return;
    m_stack.pop_front();
    m_titles.pop_front();
    emit currentPageChanged();
}

void StackController::replace(QUrl url, QString title)
{
    m_stack.clear();
    m_stack.push_back(url);
    m_titles.clear();
    m_titles.push_back(title);

    emit currentPageChanged();
}
