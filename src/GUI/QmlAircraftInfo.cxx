#include "config.h"

#include "QmlAircraftInfo.hxx"

#include <QVariant>
#include <QDebug>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QTimer>

#include <simgear/package/Install.hxx>
#include <simgear/package/Root.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/props/props_io.hxx>

#include <Main/globals.hxx>

#include "LocalAircraftCache.hxx"
#include "FavouriteAircraftData.hxx"

using namespace simgear::pkg;

const int QmlAircraftInfo::StateTagRole =  Qt::UserRole + 1;
const int QmlAircraftInfo::StateDescriptionRole = Qt::UserRole + 2;
const int QmlAircraftInfo::StateExplicitRole = Qt::UserRole + 3;

class QmlAircraftInfo::Delegate : public simgear::pkg::Delegate
{
public:
    Delegate(QmlAircraftInfo* info) :
        p(info)
    {
        globals->packageRoot()->addDelegate(this);
    }

    ~Delegate() override
    {
        globals->packageRoot()->removeDelegate(this);
    }

protected:
    void finishInstall(InstallRef aInstall, StatusCode aReason) override;

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
        Q_UNUSED(total)
        if (aInstall->package() == p->packageRef()) {
            p->setDownloadBytes(bytes);
        }
    }

    void installStatusChanged(InstallRef aInstall, StatusCode aReason) override
    {
        Q_UNUSED(aReason)
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

////////////////////////////////////////////////////////////////////////////


struct StateInfo
{
    string_list tags;
    QString name;   // human-readable name, or blank if we auto-generate this
    QString description; // human-readable description

    std::string primaryTag() const
    { return tags.front(); }
};

using AircraftStateVec = std::vector<StateInfo>;

static bool readAircraftStates(const SGPropertyNode_ptr root, AircraftStateVec& result)
{
    result.clear();
    if (!root->getNode("sim/state")) {
        return false; // no states
    }

    auto nodes = root->getNode("sim")->getChildren("state");
    result.reserve(nodes.size());
    for (auto cn : nodes) {
        string_list stateNames;
        for (auto nameNode : cn->getChildren("name")) {
            stateNames.push_back(nameNode->getStringValue());
        }

        if (stateNames.empty()) {
            qWarning() << "state with no names defined, skipping" << QString::fromStdString(cn->getPath());
            continue;
        }

        result.push_back({stateNames,
                          QString::fromStdString(cn->getStringValue("readable-name")),
                          QString::fromStdString(cn->getStringValue("description"))
                         });
    }

    return true;
}

QString humanNameFromStateTag(const std::string& tag)
{
    if (tag == "approach") return QObject::tr("On approach");
    if ((tag == "take-off") || (tag == "takeoff"))
        return QObject::tr("Ready for take-off");
    if ((tag == "parked") || (tag == "parking") || (tag == "cold-and-dark"))
        return QObject::tr("Parked, cold & dark");
    if (tag == "auto")
        return QObject::tr("Automatic");
    if (tag == "cruise")
        return QObject::tr("Cruise");
    if (tag == "taxi")
        return QObject::tr("Ready to taxi");
    if (tag == "carrier-approach")
        return QObject::tr("On approach to a carrier");
    if (tag == "carrier-take-off")
        return QObject::tr("Ready for catapult launch");

    qWarning() << Q_FUNC_INFO << "add translation / string for" << QString::fromStdString(tag);
    // no mapping, let's use the tag directly
    return QString::fromStdString(tag);
}

class StatesModel : public QAbstractListModel
{
    Q_OBJECT

public:
    StatesModel(QObject* pr) : QAbstractListModel(pr)
    {
    }

    void clear()
    {
        beginResetModel();
        _data.clear();
        _explicitAutoState = false;
        endResetModel();
    }

    void initWithStates(const AircraftStateVec& states)
    {
        beginResetModel();
        _data = states;
        _explicitAutoState = false;

        // we use an empty model for aircraft with no states defined
        if (states.empty()) {
            endResetModel();
            return;
        }

        // sort which places 'auto' item at the front if it exists
        std::sort(_data.begin(), _data.end(), [](const StateInfo& a, const StateInfo& b) {
            if (a.primaryTag() == "auto") return true;
            if (b.primaryTag() == "auto") return false;
            return a.primaryTag() < b.primaryTag();
        });

        if (_data.front().primaryTag() == "auto") {
            // track if the aircraft supplied an 'auto' state, in which case
            // we will not run our own selection logic
            _explicitAutoState = true;
        } else {
            _data.insert(_data.begin(), {{"auto"}, {}, tr("Select state based on startup position.")});
        }

        endResetModel();
    }

    Q_INVOKABLE int indexForTag(QString s) const
    {
        return indexForTag(s.toStdString());
    }

    int indexForTag(const std::string &tag) const
    {
        auto it = std::find_if(_data.begin(), _data.end(), [tag](const StateInfo& i) {
            auto tagIt = std::find(i.tags.begin(), i.tags.end(), tag);
            return (tagIt != i.tags.end());
        });

        if (it == _data.end())
            return -1;

        return static_cast<int>(std::distance(_data.begin(), it));
    }

    int rowCount(const QModelIndex &) const override
    {
        return static_cast<int>(_data.size());
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        size_t i;
        if (!makeSafeIndex(index.row(), i))
            return {};
        const StateInfo& s = _data.at(i);
        if (role == Qt::DisplayRole) {
            if (s.name.isEmpty()) {
                return humanNameFromStateTag(s.primaryTag());
            }
            return s.name;
        } else if (role == QmlAircraftInfo::StateTagRole) {
            return QString::fromStdString(s.primaryTag());
        } else if (role == QmlAircraftInfo::StateDescriptionRole) {
            return s.description;
        } else if (role == QmlAircraftInfo::StateExplicitRole) {
            if (s.primaryTag() == "auto")
                return _explicitAutoState;
            return true;
        }

        return {};
    }

    QHash<int, QByteArray> roleNames() const override
    {
        auto result = QAbstractListModel::roleNames();
        result[Qt::DisplayRole] = "name";
        result[QmlAircraftInfo::StateTagRole] = "tag";
        result[QmlAircraftInfo::StateDescriptionRole] = "description";
        return result;
    }

    Q_INVOKABLE QString descriptionForState(int row) const
    {
        size_t index;
        if (!makeSafeIndex(row, index))
            return {};

        const StateInfo& s = _data.at(index);
        return s.description;
    }

    Q_INVOKABLE QString tagForState(int row) const
    {
        size_t index;
        if (!makeSafeIndex(row, index))
            return {};

        return QString::fromStdString(_data.at(index).primaryTag());
    }

    bool hasExplicitAuto() const
    {
        return _explicitAutoState;
    }

    bool isEmpty() const
    {
        return _data.empty();
    }

    bool hasState(QString st) const
    {
        return indexForTag(st.toStdString()) != -1;
    }
private:
    bool makeSafeIndex(int row, size_t& t) const
    {
        if (row < 0) {
            return false;
        }
        t = static_cast<size_t>(row);
        return (t < _data.size());
    }

    AircraftStateVec _data;
    bool _explicitAutoState = false;
};

////////////////////////////////////////////////////////////////////////////

void QmlAircraftInfo::Delegate::finishInstall(InstallRef aInstall, StatusCode aReason)
{
    Q_UNUSED(aReason)
    if (aInstall->package() == p->packageRef()) {
        p->_cachedProps.reset();
        if (p->_statesModel) {
            p->_statesModel->clear();
        }
        p->_statesChecked = false;
        p->infoChanged();
    }
}


////////////////////////////////////////////////////////////////////////////

QmlAircraftInfo::QmlAircraftInfo(QObject *parent)
    : QObject(parent)
    , _delegate(new Delegate(this))
{
    qmlRegisterUncreatableType<StatesModel>("FlightGear.Launcher", 1, 0, "StatesModel", "no");
    connect(FavouriteAircraftData::instance(), &FavouriteAircraftData::changed,
            this, &QmlAircraftInfo::onFavouriteChanged);
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

quint32 QmlAircraftInfo::numVariants() const
{
    if (_item) {
        // for on-disk, we don't count the primary item
        return static_cast<quint32>(_item->variants.size() + 1);
    } else if (_package) {
        // whereas for packaged aircraft we do
        return static_cast<quint32>(_package->variants().size());
    }

    return 0;
}

QString QmlAircraftInfo::name() const
{
    if (_item) {
        return resolveItem()->name();
    } else if (_package) {
        return QString::fromStdString(_package->nameForVariant(_variant));
    }

    return {};
}

QString QmlAircraftInfo::description() const
{
    if (_item) {
        return resolveItem()->description();
    } else if (_package) {
        std::string longDesc = _package->getLocalisedProp("description", _variant);
        return QString::fromStdString(longDesc).simplified();
    }

    return {};
}

QString QmlAircraftInfo::authors() const
{
    SGPropertyNode_ptr structuredAuthors;
    if (_item) {
        validateLocalProps();
        if (_cachedProps)
            structuredAuthors = _cachedProps->getNode("sim/authors");

        if (!structuredAuthors)
            return resolveItem()->authors;
    } else if (_package) {
        if (_package->properties()->hasChild("authors")) {
            structuredAuthors = _package->properties()->getChild("authors");
        } else {
            std::string authors = _package->getLocalisedProp("author", _variant);
            return QString::fromStdString(authors);
        }
    }

    if (structuredAuthors) {
        // build formatted HTML based on authors data
        QString html = "<ul>\n";
        for (auto a : structuredAuthors->getChildren("author")) {
            html += "<li>";
            html += a->getStringValue("name");
            if (a->hasChild("nick")) {
                html += QStringLiteral(" '") + QString::fromStdString(a->getStringValue("nick")) + QStringLiteral("'");
            }
            if (a->hasChild("description")) {
                html += QStringLiteral(" - <i>") +  QString::fromStdString(a->getStringValue("description")) + QStringLiteral("</i>");
            }
            html  += "</li>\n";
        }
        html += "<ul>\n";
        return html;
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
        auto actualItem = resolveItem();
        Q_FOREACH(QUrl u, actualItem->previews) {
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
                    // this happens when the aircraft is being installed, for example
                    continue;
                }
                result.append(QUrl::fromLocalFile(QString::fromStdString(localPreviewPath.utf8Str())));
            }
            return result;
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

QUrl QmlAircraftInfo::homePage() const
{
    if (_item) {
        return resolveItem()->homepageUrl;
    } else if (_package) {
        const auto u = _package->properties()->getStringValue("urls/home-page");
        return QUrl(QString::fromStdString(u));
    }

    return {};
}

QUrl QmlAircraftInfo::supportUrl() const
{
    if (_item) {
        return resolveItem()->supportUrl;
    } else if (_package) {
        const auto u = _package->properties()->getStringValue("urls/support");
        return QUrl(QString::fromStdString(u));
    }

    return {};
}

QUrl QmlAircraftInfo::wikipediaUrl() const
{
    if (_item) {
        return resolveItem()->wikipediaUrl;
    } else if (_package) {
        const auto u = _package->properties()->getStringValue("urls/wikipedia");
        return QUrl(QString::fromStdString(u));
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

quint64 QmlAircraftInfo::packageSize() const
{
    if (_package) {
        return _package->fileSizeBytes();
    }

    return 0;
}

quint64 QmlAircraftInfo::downloadedBytes() const
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

    return LocalAircraftCache::AircraftOk;
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

void QmlAircraftInfo::validateStates() const
{
    // after calling this, StatesModel must exist, but can be empty
    if (!_statesModel) {
        _statesModel = new StatesModel(const_cast<QmlAircraftInfo*>(this));
    }

    if (_statesChecked)
        return;

    validateLocalProps();
    if (!_cachedProps)
        return;

    AircraftStateVec statesData;
    readAircraftStates(_cachedProps, statesData);
    _statesModel->initWithStates(statesData);
    _statesChecked = true;
}

void QmlAircraftInfo::validateLocalProps() const
{
    if (!_cachedProps) {
        // clear any previous states if we're reusing this object
        if (_statesModel)
            _statesModel->clear();

        _statesChecked = false;

        SGPath path = SGPath::fromUtf8(pathOnDisk().toStdString());
        if (!path.exists())
            return;
        _cachedProps = new SGPropertyNode;
        auto r = LocalAircraftCache::instance()->readAircraftProperties(path, _cachedProps);
        if (r == LocalAircraftCache::ParseSetXMLResult::Retry) {
            _cachedProps.reset();

            // give the AircraftScsn threa d abit more time
            QTimer::singleShot(2000, this, &QmlAircraftInfo::retryValidateLocalProps);
        } else if (r == LocalAircraftCache::ParseSetXMLResult::Ok) {
            // we're good
        } else {
            // failure
            _cachedProps.reset();
        }
    }
}

void QmlAircraftInfo::setUri(QUrl u)
{
    if (uri() == u)
        return;

    _item.clear();
    _package.clear();
    _cachedProps.clear();
    _statesChecked = false;
    if (_statesModel)
        _statesModel->clear();

    if (u.isLocalFile()) {
        _item = LocalAircraftCache::instance()->findItemWithUri(u);
        if (!_item) {
            // scan still active or aircraft not found, let's bail out
            // and rely on caller to try again
            return;
        }

        int vindex = _item->indexOfVariant(u);
        // we need to offset the variant index to allow for the different
        // indexing schemes here (primary included) and in the cache (primary
        // is not counted)
        _variant = (vindex >= 0) ? static_cast<quint32>(vindex + 1) : 0U;
    } else if (u.scheme() == "package") {
        auto ident = u.path().toStdString();
        try {
            _package = globals->packageRoot()->getPackageById(ident);
            if (_package) {
                _variant = _package->indexOfVariant(ident);
            }
        } catch (sg_exception&) {
            qWarning() << "couldn't find package/variant for " << u;
        }
    }

    emit uriChanged();
    emit infoChanged();
    emit downloadChanged();
    emit favouriteChanged();
}

void QmlAircraftInfo::setVariant(quint32 variant)
{
    if (!_item && !_package)
        return;

    if (variant >= numVariants()) {
        qWarning() << Q_FUNC_INFO << uri() << "variant index out of range:" << variant;
        return;
    }

    if (_variant == variant)
        return;

    _variant = variant;
    _cachedProps.clear();
    _statesChecked = false;
    if (_statesModel)
        _statesModel->clear();

    emit infoChanged();
    emit variantChanged(_variant);
}

void QmlAircraftInfo::setFavourite(bool favourite)
{
    FavouriteAircraftData::instance()->setFavourite(uri(), favourite);
}

void QmlAircraftInfo::onFavouriteChanged(QUrl u)
{
    if (u != uri())
        return;

    emit favouriteChanged();
}

void QmlAircraftInfo::retryValidateLocalProps()
{
    qInfo() << Q_FUNC_INFO << "Retrying validation of local props for:" << uri();
    validateLocalProps();
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

    return LocalAircraftCache::NotPackaged;
}

PackageRef QmlAircraftInfo::packageRef() const
{
    return _package;
}

void QmlAircraftInfo::setDownloadBytes(quint64 bytes)
{
    _downloadBytes = bytes;
    emit downloadChanged();
}

QStringList QmlAircraftInfo::variantNames() const
{
    QStringList result;
    if (_item) {
        result.append(_item->name());
        Q_FOREACH(auto v, _item->variants) {
            if (v->name().isEmpty()) {
                qWarning() << Q_FUNC_INFO << "missing description for " << v->path;
            }
            result.append(v->name());
        }
    } else if (_package) {
        for (quint32 vindex = 0; vindex < _package->variants().size(); ++vindex) {
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

bool QmlAircraftInfo::hasStates() const
{
    validateStates();
    return !_statesModel->isEmpty();
}

bool QmlAircraftInfo::hasState(QString name) const
{
    validateStates();
    return _statesModel->hasState(name);
}

bool QmlAircraftInfo::haveExplicitAutoState() const
{
    validateStates();
    return _statesModel->hasExplicitAuto();
}

StatesModel *QmlAircraftInfo::statesModel()
{
    validateStates();
    return _statesModel;
}

QuantityValue QmlAircraftInfo::cruiseSpeed() const
{
    validateLocalProps();
    if (!_cachedProps) {
        return {};
    }

    if (_cachedProps->hasValue("aircraft/performance/cruise/mach")) {
        return QuantityValue{Units::Mach, _cachedProps->getDoubleValue("aircraft/performance/cruise/mach")};
    }

    if (_cachedProps->hasValue("aircraft/performance/cruise/airpseed-knots")) {
        return QuantityValue{Units::Knots, _cachedProps->getIntValue("aircraft/performance/cruise/airpseed-knots")};
    }

    return {};
}

QuantityValue QmlAircraftInfo::approachSpeed() const
{
    validateLocalProps();
    if (!_cachedProps) {
        return {};
    }

    if (_cachedProps->hasValue("aircraft/performance/approach/airpseed-knots")) {
        return QuantityValue{Units::Knots, _cachedProps->getIntValue("aircraft/performance/approach/airpseed-knots")};
    }

    return {};
}

QuantityValue QmlAircraftInfo::cruiseAltitude() const
{
    validateLocalProps();
    if (!_cachedProps) {
        return {};
    }

    if (_cachedProps->hasValue("aircraft/performance/cruise/flight-level")) {
        return QuantityValue{Units::FlightLevel, _cachedProps->getIntValue("aircraft/performance/cruise/flight-level")};
    }

    if (_cachedProps->hasValue("aircraft/performance/cruise/airpseed-knots")) {
        return QuantityValue{Units::FeetMSL, _cachedProps->getIntValue("aircraft/performance/cruise/altitude-ft")};
    }

    return {};
}

QString QmlAircraftInfo::icaoType() const
{
    validateLocalProps();
    if (!_cachedProps) {
        return {};
    }

    return QString::fromStdString(_cachedProps->getStringValue("aircraft/icao/type"));
}

bool QmlAircraftInfo::isSpeedBelowLimits(QuantityValue speed) const
{
    Q_UNUSED(speed)
    return true;
}

bool QmlAircraftInfo::isAltitudeBelowLimits(QuantityValue speed) const
{
    Q_UNUSED(speed)
    return true;
}

bool QmlAircraftInfo::hasTag(QString tag) const
{
    if (_item) {
        return resolveItem()->tags.contains(tag);
    } else if (_package) {
        const auto& tags = _package->tags();
        auto it = tags.find(tag.toStdString());
        return (it != tags.end());
    }

    return false;
}

bool QmlAircraftInfo::favourite() const
{
    return FavouriteAircraftData::instance()->isFavourite(uri());
}

#include "QmlAircraftInfo.moc"
