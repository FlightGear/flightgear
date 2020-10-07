// AddonsModel.cxx - part of GUI launcher using Qt5
//
// Written by Dan Wickstrom, started February 2019.
//
// Copyright (C) 2019 Daniel Wickstrom <daniel.c.wickstrom@gmail.com>
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

#include "config.h"

#include <QDebug>
#include <QUrl>
#include <string>
#include <simgear/misc/strutils.hxx>

#include "AddonsModel.hxx"
#include "Add-ons/AddonMetadataParser.hxx"

AddonsModel::AddonsModel(QObject* pr) :
    QAbstractListModel(pr)
{
    m_roleToName[Qt::DisplayRole] = "display";
    m_nameToRole["display"] = Qt::DisplayRole;

    int roleValue = IdRole;

    for (auto it = m_roles.begin(); it != m_roles.end(); ++it) {
        QByteArray name = it->toUtf8();
        m_roleToName[roleValue] = name;
        m_nameToRole[*it] = roleValue++;
    }
}

AddonsModel::~AddonsModel()
{

}

void AddonsModel::resetData(const QStringList& ndata)
{
    beginResetModel();
    QSet<QString> newSet = QSet<QString>::fromList(ndata);

    for(const auto& path : m_addonsList) {
        if (!newSet.contains(path)) {
            m_addonsMap.remove(path);
        }
    }
    m_addonsList = ndata;
    endResetModel();
}

int AddonsModel::rowCount(const QModelIndex& parent) const
{
    return m_addonsList.size();
}


QVariant AddonsModel::data(const QModelIndex& index, int role) const
{
    auto idx = index.row();
    return get(idx, role);
}

QVariant AddonsModel::get(int idx, QString role) const
{
    int role_idx = m_nameToRole[role];
    return get(idx, role_idx);
}

QVariant AddonsModel::get(int idx, int role) const
{
    if (idx >= 0 && idx < m_addonsList.size()) {
        auto path = m_addonsList[idx];
        if (!m_addonsMap.contains(path)) {
            if ((role == PathRole) || (role == Qt::DisplayRole)) {
                return path;
            }

            return {};
        }

        auto addon = m_addonsMap[path].addon;
        if (!addon)
            return {};

        if (role == Qt::DisplayRole) {
            QString name = QString::fromStdString(addon->getName());
            QString desc = QString::fromStdString(addon->getShortDescription());
            return tr("%1 - %2").arg(name).arg(desc);
        }
        else if (role == IdRole) {
            return QString::fromStdString(addon->getId());
        }
        else if (role == NameRole) {
            return QString::fromStdString(addon->getName());
        }
        else if (role == PathRole) {
            return path;
        }
        else if (role == VersionRole) {
            const auto v = addon->getVersion()->str();
            return QString::fromStdString(v);
        }
        else if (role == AuthorsRole) {
            QStringList authors;
            for (auto author : addon->getAuthors()) {
                authors.push_back(QString::fromStdString(author->getName()));
            }
            return authors;
        }
        else if (role == MaintainersRole) {
            QStringList maintainers;
            for (auto maintainer : addon->getMaintainers()) {
                maintainers.push_back(QString::fromStdString(maintainer->getName()));
            }
            return maintainers;
        }
        else if (role == ShortDescriptionRole) {
            return QString::fromStdString(addon->getShortDescription());
        }
        else if (role == LongDescriptionRole) {
            return QString::fromStdString(addon->getLongDescription());
        }
        else if (role == LicenseDesignationRole) {
            return QString::fromStdString(addon->getLicenseDesignation());
        }
        else if (role == LicenseUrlRole) {
            return QUrl(QString::fromStdString(addon->getLicenseUrl()));
        }
        else if (role == TagsRole) {
            QStringList tags;
            for (auto tag : addon->getTags()) {
                tags.push_back(QString::fromStdString(tag));
            }
            return tags;
        }
        else if (role == MinFGVersionRole) {
            const auto v = addon->getMinFGVersionRequired();
            if (v == "none")
                return QStringLiteral("-");
            return QString::fromStdString(v);
        }
        else if (role == MaxFGVersionRole) {
            const auto v = addon->getMaxFGVersionRequired();
            if (v == "none")
                return QStringLiteral("-");
            return QString::fromStdString(v);
        }
        else if (role == HomePageRole) {
            return QUrl(QString::fromStdString(addon->getHomePage()));
        }
        else if (role == DownloadUrlRole) {
            return QUrl(QString::fromStdString(addon->getDownloadUrl()));
        }
        else if (role == SupportUrlRole) {
            return QUrl(QString::fromStdString(addon->getSupportUrl()));
        }
        else if (role == CodeRepoUrlRole) {
            return QUrl(QString::fromStdString(addon->getCodeRepositoryUrl()));
        }
        else if (role == EnableRole) {
            return QVariant(m_addonsMap[path].enable && checkVersion(path));
        }
    }
    return QVariant();
}


bool AddonsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    return false;
}

bool AddonsModel::append(QString path, flightgear::addons::AddonRef& addon, bool enable) {

    if (!m_addonsMap.contains(path)) {
        m_addonsList.push_back(path);
        m_addonsMap[path].addon = addon;
        m_addonsMap[path].enable = enable && checkVersion(path);
        emit dataChanged(index(m_addonsList.size()-1), index(m_addonsList.size()-1));

        return true;
    }

    return false;
}

Qt::ItemFlags AddonsModel::flags(const QModelIndex &index) const
{
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

QHash<int, QByteArray> AddonsModel::roleNames() const
{
    return m_roleToName;
}

void AddonsModel::enable(int row, bool enable)
{
    if ((row < 0) || (row >= m_addonsList.size())) {
        return;
    }

    auto path = m_addonsList[row];
    if (!m_addonsMap.contains(path))
        return;

    const bool wasEnabled = m_addonsMap[path].enable;
    const bool nowEnabled = enable && checkVersion(path);
    if (wasEnabled == nowEnabled)
        return;

    m_addonsMap[path].enable = nowEnabled;
    const auto mindex = index(row, 0);
    emit dataChanged(mindex, mindex, {EnableRole});
    emit modulesChanged();
}

bool AddonsModel::checkVersion(QString path) const
{
    using namespace simgear;

    if (!m_addonsMap.contains(path)) {
        return false;
    }

    // Check that the FlightGear version satisfies the add-on requirements
    std::string minFGversion = m_addonsMap[path].addon->getMinFGVersionRequired();
    if (strutils::compare_versions(FLIGHTGEAR_VERSION, minFGversion) < 0) {
        return false;
    }

    std::string maxFGversion = m_addonsMap[path].addon->getMaxFGVersionRequired();
    if (maxFGversion != "none" &&
        strutils::compare_versions(FLIGHTGEAR_VERSION, maxFGversion) > 0) {
        return false;
    }

    return true;
}
