// AddonsModel.hxx - part of GUI launcher using Qt5
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

#ifndef FG_GUI_METADATA_LIST_MODEL
#define FG_GUI_METADATA_LIST_MODEL

#include <QAbstractListModel>
#include <QDateTime>
#include <QDir>
#include <QPixmap>
#include <QStringList>
#include <set>
#include <simgear/package/Root.hxx>
#include <simgear/package/Catalog.hxx>
#include "Add-ons/addon_fwd.hxx"
#include "Add-ons/Addon.hxx"
#include "Add-ons/AddonMetadataParser.hxx"



class AddonsModel : public QAbstractListModel
{
    Q_OBJECT

    using AddonsMeta = struct { bool enable; flightgear::addons::AddonRef addon; };
    using AddonsMap = QHash<QString, AddonsMeta>;
public:

    AddonsModel(QObject* pr);

    ~AddonsModel();

    int rowCount(const QModelIndex& parent) const override;

    QVariant data(const QModelIndex& index, int role) const override;
    Q_INVOKABLE QVariant get(int index, int role) const;
    Q_INVOKABLE QVariant get(int index, QString role) const;

    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    QHash<int, QByteArray> roleNames() const override;

    bool append(QString path, flightgear::addons::AddonRef& addon, bool enable);

    void resetData(const QStringList& ndata);

    Q_INVOKABLE bool checkVersion(QString path) const;

    bool getPathEnable(const QString& path) { return m_addonsMap[path].enable; }
    bool containsPath(const QString& path) { return m_addonsMap.contains(path); }

    Q_INVOKABLE void enable(int index, bool enable);

    // this must stay in sync with m_roles stringlist
    enum AddonRoles
    {
        IdRole = Qt::UserRole + 1,
        NameRole,
        PathRole,
        VersionRole,
        AuthorsRole,
        MaintainersRole,
        ShortDescriptionRole,
        LongDescriptionRole,
        LicenseDesignationRole,
        LicenseUrlRole,
        TagsRole,
        MinFGVersionRole,
        MaxFGVersionRole,
        HomePageRole,
        DownloadUrlRole,
        SupportUrlRole,
        CodeRepoUrlRole,
        EnableRole
    };

    Q_ENUMS(AddonRoles)

signals:
    void modulesChanged();

private:
    // this must stay in sync with MetaRoles enum
    const QStringList m_roles = {
        "id",
        "name",
        "path",
        "version",
        "authors",
        "maintainers",
        "short_description",
        "long_description",
        "license_designation",
        "license_url",
        "tags",
        "minversion_fg",
        "maxversion_fg",
        "homepage",
        "download_url",
        "support_url",
        "coderepo_url",
        "enable"
    };

    QStringList m_addonsList;
    AddonsMap m_addonsMap;
    QHash<int, QByteArray> m_roleToName;
    QHash<QString, int> m_nameToRole;
};

#endif // of Metadata_LIST_MODEL
