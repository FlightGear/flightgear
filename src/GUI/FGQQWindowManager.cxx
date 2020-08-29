#include "FGQQWindowManager.hxx"

#include <algorithm>

#include <QAbstractListModel>
#include <QQmlComponent>
#include <QRectF>
#include <QSettings>

#include <simgear/props/props_io.hxx>

#include "QtHelpers.hxx"
#include <Main/globals.hxx>

////////////////////////////////////////////////////////////////////////////

/**
 * @brief information on all the dialogs / windows we have registered
 */
struct DialogInfo {
    DialogInfo(SGPropertyNode* node)
    {
        id = QString::fromStdString(node->getStringValue("id"));
        if (id.isEmpty()) {
            throw std::runtime_error("missing ID");
        }

        title = QString::fromStdString(node->getStringValue("title"));

        SGPath p = globals->get_fg_root() / "gui" / "quick" / node->getStringValue("file");
        if (!p.exists()) {
            qWarning() << "missing dialog QML file: " << p;
        }
        resource = QUrl::fromLocalFile(QString::fromStdString(p.utf8Str()));
    }

    QString id;
    QString title;
    QUrl resource;
    // flags?
};

////////////////////////////////////////////////////////////////////////////

const int WindowContentUrl = Qt::UserRole + 1;
const int WindowSize = Qt::UserRole + 2;
const int WindowPosition = Qt::UserRole + 3;
const int WindowId = Qt::UserRole + 4;
const int WindowTitle = Qt::UserRole + 5;

/**
 * @brief data about an active window
 */
struct WindowData {
    QString windowId;
    QUrl source;
    QRectF geometry;
    QString title;

    // frame flags (close, popout, etc)
};

class WindowsModel : public QAbstractListModel
{
public:
    WindowsModel(FGQQWindowManager* wm) : QAbstractListModel(wm)
    {
    }

    int rowCount(const QModelIndex&) const override
    {
        return static_cast<int>(_windowData.size());
    }

    QHash<int, QByteArray> roleNames() const override
    {
        QHash<int, QByteArray> r;
        r[WindowId] = "windowId";
        r[WindowContentUrl] = "windowContentSource";
        r[WindowSize] = "windowSize";
        r[WindowPosition] = "windowPosition";
        r[WindowTitle] = "windowTitle";

        return r;
    }

    QVariant data(const QModelIndex& index, int role) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;

    void show(const DialogInfo& dinfo)
    {
        QSettings settings;
        QRectF geom = settings.value(dinfo.id + "-geometry").toRectF();
        if (!geom.isValid()) {
            // fixme: define default window geometry
            geom = QRectF{100, 100, 400, 400};
        }

        WindowData winfo = {dinfo.id, dinfo.resource, geom, ""};

        beginInsertRows({}, _windowData.size(), _windowData.size());
        _windowData.push_back(winfo);
        endInsertRows();
    }

    void close(QString windowId)
    {
        auto it = findWindow(windowId);
        if (it == _windowData.end()) {
            SG_LOG(SG_GUI, SG_DEV_WARN, "trying to close non-open window:" << windowId.toStdString());
            return;
        }

        const int index = std::distance(_windowData.begin(), it);
        beginRemoveRows({}, index, index);
        _windowData.erase(it);
        endRemoveRows();
    }

    std::vector<WindowData>::const_iterator findWindow(QString windowId) const
    {
        return std::find_if(_windowData.begin(), _windowData.end(),
                            [windowId](const WindowData& d) { return d.windowId == windowId; });
    }

    std::vector<WindowData>::iterator findWindow(QString windowId)
    {
        return std::find_if(_windowData.begin(), _windowData.end(),
                            [windowId](const WindowData& d) { return d.windowId == windowId; });
    }

    std::vector<WindowData> _windowData;
};

QVariant WindowsModel::data(const QModelIndex& index, int role) const
{
    auto tRow = static_cast<size_t>(index.row());
    if ((index.row() < 0) || (tRow >= _windowData.size()))
        return {};

    const auto& d = _windowData.at(tRow);
    switch (role) {
    case WindowId:
        return d.windowId;
    case WindowContentUrl:
        return d.source;
    case WindowSize:
        return d.geometry.size();
    case WindowPosition:
        return d.geometry.topLeft();
    case WindowTitle:
        return d.title;

    default: break;
    }
    return {};
}

bool WindowsModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    auto tRow = static_cast<size_t>(index.row());
    if ((index.row() < 0) || (tRow >= _windowData.size()))
        return false;

    auto& d = _windowData[tRow];
    QSettings settings;
    if (role == WindowSize) {
        d.geometry.setSize(value.toSizeF());
        settings.setValue(d.windowId + "-geometry", d.geometry);
        emit dataChanged(index, index, {role});
        return true;
    } else if (role == WindowPosition) {
        d.geometry.setTopLeft(value.toPointF());
        settings.setValue(d.windowId + "-geometry", d.geometry);
        emit dataChanged(index, index, {role});
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////

class FGQQWindowManager::WindowManagerPrivate
{
public:
    WindowManagerPrivate(FGQQWindowManager* outer) : _p(outer),
                                                     _windows(new WindowsModel{outer})
    {
        // scan dir to find built-in dialogs for now
        SGPath p = globals->get_fg_root() / "gui" / "quick" / "dialogs.xml";
        if (p.exists()) {
            SGPropertyNode_ptr dialogProps(new SGPropertyNode);
            readProperties(p, dialogProps);
            for (auto nd : dialogProps->getChildren("dialog")) {
                try {
                    DialogInfo dinfo(nd);
                    _dialogDefinitions.push_back(dinfo);
                } catch (std::exception&) {
                    // skip this dialog
                    SG_LOG(SG_GUI, SG_DEV_WARN, "Skipped bad QML dialog definition");
                }
            }
        } else {
            SG_LOG(SG_GUI, SG_DEV_WARN, "No QML dialog definitions found");
        }
    }

    FGQQWindowManager* _p;
    WindowsModel* _windows;
    std::vector<DialogInfo> _dialogDefinitions;
};

////////////////////////////////////////////////////////////////////////////

FGQQWindowManager::FGQQWindowManager(QQmlEngine* engine, QObject* parent) : QObject(parent),
                                                                            _d(new WindowManagerPrivate(this)),
                                                                            _engine(engine)
{
    // register commands to open / close / etc
}

FGQQWindowManager::~FGQQWindowManager()
{
}

QAbstractItemModel* FGQQWindowManager::windows() const
{
    return _d->_windows;
}

bool FGQQWindowManager::show(QString windowId)
{
    // find QML file corresponding to windowId
    auto it = std::find_if(_d->_dialogDefinitions.begin(),
                           _d->_dialogDefinitions.end(),
                           [windowId](const DialogInfo& dd) { return dd.id == windowId; });
    if (it == _d->_dialogDefinitions.end()) {
        SG_LOG(SG_GUI, SG_DEV_WARN, "Unknown dialog ID:" << windowId.toStdString());
        return false;
    }

    _d->_windows->show(*it);
    return true;
}

bool FGQQWindowManager::requestPopout(QString windowId)
{
    return false;
}

bool FGQQWindowManager::requestClose(QString windowId)
{
    _d->_windows->close(windowId);
    return true;
}

bool FGQQWindowManager::requestPopin(QString windowId)
{
    return false;
}
