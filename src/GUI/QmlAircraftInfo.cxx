#include "QmlAircraftInfo.hxx"

#include <QVariant>
#include <QDebug>

#include <simgear/package/Install.hxx>
#include <simgear/package/Root.hxx>
#include <simgear/structure/exception.hxx>

#include <Include/version.h>
#include <Main/globals.hxx>

#include "LocalAircraftCache.hxx"

using namespace simgear::pkg;

class QmlAircraftInfo::Delegate : public simgear::pkg::Delegate
{
public:
    Delegate(QmlAircraftInfo* info) :
        p(info)
    {
        globals->packageRoot()->addDelegate(this);
    }

    ~Delegate()
    {
        globals->packageRoot()->removeDelegate(this);
    }

protected:
    void catalogRefreshed(CatalogRef, StatusCode) override
    {
    }

    void startInstall(InstallRef aInstall) override
    {
        if (aInstall->package() == p->packageRef()) {
            p->setDownloadBytes(0);
        }
    }

    void installProgress(InstallRef aInstall, unsigned int bytes, unsigned int total) override
    {
        if (aInstall->package() == p->packageRef()) {
            p->setDownloadBytes(bytes);
        }
    }

    void finishInstall(InstallRef aInstall, StatusCode aReason) override
    {
        Q_UNUSED(aReason);
        if (aInstall->package() == p->packageRef()) {
            p->infoChanged();
        }
    }

    void installStatusChanged(InstallRef aInstall, StatusCode aReason) override
    {
        Q_UNUSED(aReason);
        if (aInstall->package() == p->packageRef()) {
            p->downloadChanged();
        }
    }

    void finishUninstall(const PackageRef& pkg) override
    {
        if (pkg == p->packageRef()) {
            p->downloadChanged();
        }
    }

private:

    QmlAircraftInfo* p;
};

QmlAircraftInfo::QmlAircraftInfo(QObject *parent)
    : QObject(parent)
    , _delegate(new Delegate(this))
{
}

QmlAircraftInfo::~QmlAircraftInfo()
{

}

QUrl QmlAircraftInfo::uri() const
{
    if (_item) {
        return QUrl::fromLocalFile(resolveItem()->path);
    } else if (_package) {
        return QUrl("package:" + QString::fromStdString(_package->qualifiedVariantId(_variant)));
    }

    return {};
}

int QmlAircraftInfo::numVariants() const
{
    if (_item) {
        // for on-disk, we don't count the primary item
        return _item->variants.size() + 1;
    } else if (_package) {
        // whereas for packaged aircraft we do
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

QVariantList QmlAircraftInfo::previews() const
{
    if (_item) {
        QVariantList result;
        Q_FOREACH(QUrl u, _item->previews) {
            result.append(u);
        }
        return result;
    }

    if (_package) {
        const auto& previews = _package->previewsForVariant(_variant);
        if (previews.empty()) {
            return {};
        }

        QVariantList result;
        // if we have an install, return file URLs, not remote (http) ones
        auto ex = _package->existingInstall();
        if (ex.valid()) {
            for (auto p : previews) {
                SGPath localPreviewPath = ex->path() / p.path;
                if (!localPreviewPath.exists()) {
                    qWarning() << "missing local preview" << QString::fromStdString(localPreviewPath.utf8Str());
                    continue;
                }
                result.append(QUrl::fromLocalFile(QString::fromStdString(localPreviewPath.utf8Str())));
            }
        }

        // return remote urls
        for (auto p : previews) {
            result.append(QUrl(QString::fromStdString(p.url)));
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
    return _downloadBytes;
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

void QmlAircraftInfo::setUri(QUrl u)
{
    if (uri() == u)
        return;

    _item.clear();
    _package.clear();

    if (u.isLocalFile()) {
        _item = LocalAircraftCache::instance()->findItemWithUri(u);
        int vindex = _item->indexOfVariant(u);
        // we need to offset the variant index to allow for the different
        // indexing schemes here (primary included) and in the cache (primary
        // is not counted)
        _variant = (vindex >= 0) ? vindex + 1 : 0;
    } else if (u.scheme() == "package") {
        auto ident = u.path().toStdString();
        try {
            _package = globals->packageRoot()->getPackageById(ident);
            _variant = _package->indexOfVariant(ident);
        } catch (sg_exception&) {
            qWarning() << "couldn't find package/variant for " << u;
        }
    }

    emit uriChanged();
    emit infoChanged();
    emit downloadChanged();
}

void QmlAircraftInfo::setVariant(int variant)
{
    if (!_item && !_package)
        return;

    if ((variant < 0) || (variant >= numVariants())) {
        qWarning() << Q_FUNC_INFO << uri() << "variant index out of range:" << variant;
        return;
    }

    if (_variant == variant)
        return;

    _variant = variant;
    emit infoChanged();
    emit variantChanged(_variant);
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

QVariant QmlAircraftInfo::installStatus() const
{
    if (_item) {
        return LocalAircraftCache::PackageInstalled;
    }

    if (_package) {
        auto i = _package->existingInstall();
        if (i.valid()) {
            if (i->isDownloading()) {
                return LocalAircraftCache::PackageDownloading;
            }
            if (i->isQueued()) {
                return LocalAircraftCache::PackageQueued;
            }
            if (i->hasUpdate()) {
                return LocalAircraftCache::PackageUpdateAvailable;
            }

            return LocalAircraftCache::PackageInstalled;
        } else {
            return LocalAircraftCache::PackageNotInstalled;
        }
    }

    return {};
}

PackageRef QmlAircraftInfo::packageRef() const
{
    return _package;
}

void QmlAircraftInfo::setDownloadBytes(int bytes)
{
    _downloadBytes = bytes;
    emit downloadChanged();;
}

QStringList QmlAircraftInfo::variantNames() const
{
    QStringList result;
    if (_item) {
        result.append(_item->description);
        Q_FOREACH(auto v, _item->variants) {
            if (v->description.isEmpty()) {
                qWarning() << Q_FUNC_INFO << "missing description for " << v->path;
            }
            result.append(v->description);
        }
    } else if (_package) {
        for (int vindex = 0; vindex < _package->variants().size(); ++vindex) {
            if (_package->nameForVariant(vindex).empty()) {
                qWarning() << Q_FUNC_INFO << "missing description for variant" << vindex;
            }
            result.append(QString::fromStdString(_package->nameForVariant(vindex)));
        }
    }
    return result;
}

bool QmlAircraftInfo::isPackaged() const
{
    return _package != PackageRef();
}
