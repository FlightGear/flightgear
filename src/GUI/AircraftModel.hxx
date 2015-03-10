// AircraftModel.hxx - part of GUI launcher using Qt5
//
// Written by James Turner, started March 2015.
//
// Copyright (C) 2014 James Turner <zakalawe@mac.com>
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

#ifndef FG_GUI_AIRCRAFT_MODEL
#define FG_GUI_AIRCRAFT_MODEL

#include <QAbstractListModel>
#include <QDateTime>
#include <QDir>
#include <QPixmap>

#include <simgear/package/Root.hxx>

const int AircraftPathRole = Qt::UserRole + 1;
const int AircraftAuthorsRole = Qt::UserRole + 2;
const int AircraftVariantRole = Qt::UserRole + 3;
const int AircraftVariantCountRole = Qt::UserRole + 4;
const int AircraftRatingRole = Qt::UserRole + 100;
const int AircraftVariantDescriptionRole = Qt::UserRole + 200;

class AircraftScanThread;
class QDataStream;

struct AircraftItem
{
    AircraftItem();

    AircraftItem(QDir dir, QString filePath);
    
    // the file-name without -set.xml suffix
    QString baseName() const;
    
    void fromDataStream(QDataStream& ds);

    void toDataStream(QDataStream& ds) const;

    QPixmap thumbnail() const;

    QString path;
    QString description;
    QString authors;
    int ratings[4];
    QString variantOf;
    QDateTime pathModTime;

    QList<AircraftItem*> variants;
private:
    mutable QPixmap m_thumbnail;
};

class AircraftItemModel : public QAbstractListModel
{
    Q_OBJECT
public:
    AircraftItemModel(QObject* pr, simgear::pkg::RootRef& root);

    ~AircraftItemModel();

    virtual int rowCount(const QModelIndex& parent) const
    {
        return m_items.size();
    }

    virtual QVariant data(const QModelIndex& index, int role) const;
    
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role);

  QModelIndex indexOfAircraftPath(QString path) const;

private slots:
    void onScanResults();
    
    void onScanFinished();

private:
    AircraftScanThread* m_scanThread;
    QList<AircraftItem*> m_items;
    QList<quint32> m_activeVariant;
    simgear::pkg::RootRef m_packageRoot;
};

#endif // of FG_GUI_AIRCRAFT_MODEL
