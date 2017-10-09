#include "QmlAircraftInfo.hxx"

#include <QVariant>
#include <QDebug>

#include <simgear/package/Install.hxx>

#include <Include/version.h>

#include "LocalAircraftCache.hxx"

QmlAircraftInfo::QmlAircraftInfo(QObject *parent) : QObject(parent)
{

}

QmlAircraftInfo::~QmlAircraftInfo()
{

}

int QmlAircraftInfo::numPreviews() const
{
    return 0;
}

int QmlAircraftInfo::numVariants() const
{
    if (_item) {
        return _item->variants.size();
    } else if (_package) {
        return _package->variants().size();
    }

    return 0;
}

QString QmlAircraftInfo::name() const
{
    if (_item) {
        return resolveItem()->description;
    } else if (_package) {
        return QString::fromStdString(_package->nameForVariant(_variant));
    }

    return {};
}

QString QmlAircraftInfo::description() const
{
    if (_item) {
        return resolveItem()->longDescription;
    } else if (_package) {
        std::string longDesc = _package->getLocalisedProp("description", _variant);
        return QString::fromStdString(longDesc).simplified();
    }

    return {};
}

QString QmlAircraftInfo::authors() const
{
    if (_item) {
        return resolveItem()->authors;
    } else if (_package) {
        std::string authors = _package->getLocalisedProp("author", _variant);
        return QString::fromStdString(authors);
    }

    return {};
}

QVariantList QmlAircraftInfo::ratings() const
{
    if (_item) {
        QVariantList result;
        auto actualItem = resolveItem();
        for (int i=0; i<4; ++i) {
            result << actualItem->ratings[i];
        }
        return result;
    } else if (_package) {
        SGPropertyNode* ratings = _package->properties()->getChild("rating");
        if (!ratings) {
            return {};
        }

        QVariantList result;
        for (int i=0; i<4; ++i) {
            result << ratings->getChild(i)->getIntValue();
        }
        return result;
    }
    return {};
}

QUrl QmlAircraftInfo::thumbnail() const
{
    if (_item) {
        return QUrl::fromLocalFile(resolveItem()->thumbnailPath);
    } else if (_package) {
        auto t = _package->thumbnailForVariant(_variant);
        if (QFileInfo::exists(QString::fromStdString(t.path))) {
            return QUrl::fromLocalFile(QString::fromStdString(t.path));
        }
        return QUrl(QString::fromStdString(t.url));
    }

    return {};
}

QString QmlAircraftInfo::pathOnDisk() const
{
    if (_item) {
        return resolveItem()->path;
    } else if (_package) {
        auto install = _package->existingInstall();
        if (install.valid()) {
            return QString::fromStdString(install->primarySetPath().utf8Str());
        }
    }

    return {};
}

QString QmlAircraftInfo::packageId() const
{
    if (_package) {
        return QString::fromStdString(_package->variants()[_variant]);
    }

    return {};
}

int QmlAircraftInfo::packageSize() const
{
    if (_package) {
        return _package->fileSizeBytes();
    }

    return 0;
}

int QmlAircraftInfo::downloadedBytes() const
{
    return 0;
}

QVariant QmlAircraftInfo::status() const
{
    if (_item) {
        return _item->status(_variant);
    } else if (_package) {
        return packageAircraftStatus(_package);
    }

    return {};
}

QString QmlAircraftInfo::minimumFGVersion() const
{
    if (_item) {
        return resolveItem()->minFGVersion;
    } else if (_package) {
        const std::string v = _package->properties()->getStringValue("minimum-fg-version");
        if (!v.empty()) {
            return QString::fromStdString(v);
        }
    }

    return {};
}

AircraftItemPtr QmlAircraftInfo::resolveItem() const
{
    if (_variant > 0) {
        return _item->variants.at(_variant - 1);
    }

    return _item;
}

void QmlAircraftInfo::setUri(QUrl uri)
{
    if (_uri == uri)
        return;

    _uri = uri;


    emit uriChanged();
    emit infoChanged();
}

void QmlAircraftInfo::requestInstallUpdate()
{

}

void QmlAircraftInfo::requestUninstall()
{

}

void QmlAircraftInfo::requestInstallCancel()
{

}

QVariant QmlAircraftInfo::packageAircraftStatus(simgear::pkg::PackageRef p)
{
    if (p->hasTag("needs-maintenance")) {
        return LocalAircraftCache::AircraftUnmaintained;
    }

    if (!p->properties()->hasChild("minimum-fg-version")) {
        return LocalAircraftCache::AircraftOk;
    }

    const std::string minFGVersion = p->properties()->getStringValue("minimum-fg-version");
    const int c = simgear::strutils::compare_versions(FLIGHTGEAR_VERSION, minFGVersion, 2);
    return (c < 0) ? LocalAircraftCache::AircraftNeedsNewerSimulator :
                     LocalAircraftCache::AircraftOk;
}
