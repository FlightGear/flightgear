#include "temporarywidget.h"
#include "ui_temporarywidget.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QSettings>
#include <QItemSelectionModel>

#include "fgqcanvasfontcache.h"
#include "fgqcanvasimageloader.h"
#include "canvastreemodel.h"
#include "localprop.h"
#include "elementdatamodel.h"

TemporaryWidget::TemporaryWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TemporaryWidget)
{
    ui->setupUi(this);
    connect(ui->connectButton, &QPushButton::clicked, this, &TemporaryWidget::onStartConnect);
    restoreSettings();
}

TemporaryWidget::~TemporaryWidget()
{
    disconnect(&m_webSocket, &QWebSocket::disconnected, this, &TemporaryWidget::onSocketClosed);
    m_webSocket.close();
    delete ui;
}

void TemporaryWidget::onStartConnect()
{
    QUrl wsUrl;
    wsUrl.setScheme("ws");
    wsUrl.setHost(ui->hostName->text());
    wsUrl.setPort(ui->portEdit->text().toInt());

// string clean up, to ensure our root path has a leading slash but
// no trailing slash
    QString propPath = ui->propertyPath->text();
    QString rootPath = propPath;
    if (!propPath.startsWith('/')) {
        rootPath = '/' + propPath;
    }

    if (propPath.endsWith('/')) {
        rootPath.chop(1);
    }

    wsUrl.setPath("/PropertyTreeMirror" + rootPath);
    rootPropertyPath = rootPath.toUtf8();

    connect(&m_webSocket, &QWebSocket::connected, this, &TemporaryWidget::onConnected);
    connect(&m_webSocket, &QWebSocket::disconnected, this, &TemporaryWidget::onSocketClosed);

    saveSettings();

    qDebug() << "starting connection to:" << wsUrl;
    m_webSocket.open(wsUrl);
}

void TemporaryWidget::onConnected()
{
    qDebug() << "connected";
    connect(&m_webSocket, &QWebSocket::textMessageReceived,
            this, &TemporaryWidget::onTextMessageReceived);
    m_webSocket.sendTextMessage(QStringLiteral("Hello, world!"));

    m_localPropertyRoot = new LocalProp(nullptr, NameIndexTuple(""));

    ui->canvas->setRootProperty(m_localPropertyRoot);
    ui->stack->setCurrentIndex(1);

    m_canvasModel = new CanvasTreeModel(ui->canvas->rootElement());
    ui->treeView->setModel(m_canvasModel);

    m_elementModel = new ElementDataModel(this);
    ui->elementData->setModel(m_elementModel);

    connect(ui->treeView->selectionModel(), &QItemSelectionModel::currentChanged, this, &TemporaryWidget::onTreeCurrentChanged);

    FGQCanvasFontCache::instance()->setHost(ui->hostName->text(),
                                            ui->portEdit->text().toInt());
    FGQCanvasImageLoader::instance()->setHost(ui->hostName->text(),
                                              ui->portEdit->text().toInt());
    m_webSocket.sendTextMessage("nonsense");
}

void TemporaryWidget::onTextMessageReceived(QString message)
{
    QJsonDocument json = QJsonDocument::fromJson(message.toUtf8());
    if (json.isObject()) {
        // process new nodes
        QJsonArray created = json.object().value("created").toArray();
        Q_FOREACH (QJsonValue v, created) {
            QJsonObject newProp = v.toObject();

            QByteArray nodePath = newProp.value("path").toString().toUtf8();
            if (nodePath.indexOf(rootPropertyPath) != 0) {
                qWarning() << "not a property path we are mirroring:" << nodePath;
                continue;
            }

            QByteArray localPath = nodePath.mid(rootPropertyPath.size() + 1);
            LocalProp* newNode = propertyFromPath(localPath);
            newNode->setPosition(newProp.value("position").toInt());
            // store in the global dict
            unsigned int propId = newProp.value("id").toInt();
            if (idPropertyDict.contains(propId)) {
                qWarning() << "duplicate add of:" << nodePath << "old is" << idPropertyDict.value(propId)->path();
            } else {
                idPropertyDict.insert(propId, newNode);
            }

            // set initial value
            newNode->processChange(newProp.value("value"));
        }

        // process removes
        QJsonArray removed = json.object().value("removed").toArray();
        Q_FOREACH (QJsonValue v, removed) {
            unsigned int propId = v.toInt();
            if (idPropertyDict.contains(propId)) {
                LocalProp* prop = idPropertyDict.value(propId);
                idPropertyDict.remove(propId);
                prop->parent()->removeChild(prop);
            }
        }

        // process changes
        QJsonArray changed = json.object().value("changed").toArray();

        Q_FOREACH (QJsonValue v, changed) {
            QJsonArray change = v.toArray();
            if (change.size() != 2) {
                qWarning() << "malformed change notification";
                continue;
            }

            unsigned int propId = change.at(0).toInt();
            if (!idPropertyDict.contains(propId)) {
                qWarning() << "ignoring unknown prop ID " << propId;
                continue;
            }

            LocalProp* lp = idPropertyDict.value(propId);
            lp->processChange(change.at(1));
        }
    }

    ui->canvas->update();
}

void TemporaryWidget::onSocketClosed()
{
    qDebug() << "saw web-socket closed";
    delete m_localPropertyRoot;
    m_localPropertyRoot = nullptr;
    idPropertyDict.clear();

    ui->stack->setCurrentIndex(0);
}

void TemporaryWidget::onTreeCurrentChanged(const QModelIndex &previous, const QModelIndex &current)
{
    FGCanvasElement* prev = m_canvasModel->elementFromIndex(previous);
    if (prev) {
        prev->setHighlighted(false);
    }

    FGCanvasElement* e = m_canvasModel->elementFromIndex(current);
    m_elementModel->setElement(e);
    if (e) {
        e->setHighlighted(true);
    }
}

void TemporaryWidget::saveSettings()
{
    QSettings settings;
    settings.setValue("ws-host", ui->hostName->text());
    settings.setValue("ws-port", ui->portEdit->text());
    settings.setValue("prop-path", ui->propertyPath->text());
}

void TemporaryWidget::restoreSettings()
{
    QSettings settings;
    ui->hostName->setText(settings.value("ws-host").toString());
    ui->portEdit->setText(settings.value("ws-port").toString());
    ui->propertyPath->setText(settings.value("prop-path").toString());
}

LocalProp *TemporaryWidget::propertyFromPath(QByteArray path) const
{
    return m_localPropertyRoot->getOrCreateWithPath(path);
}
