//
// Copyright (C) 2017 James Turner  zakalawe@mac.com
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

#include "temporarywidget.h"
#include "ui_temporarywidget.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QSettings>
#include <QItemSelectionModel>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "fgqcanvasfontcache.h"
#include "fgqcanvasimageloader.h"
#include "canvastreemodel.h"
#include "localprop.h"
#include "elementdatamodel.h"
#include "fgcanvastext.h"

TemporaryWidget::TemporaryWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TemporaryWidget)
{
    ui->setupUi(this);
    connect(ui->connectButton, &QPushButton::clicked, this, &TemporaryWidget::onConnect);
    restoreSettings();

    ui->quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    ui->quickWidget->setSource(QUrl("root.qml"));

    connect(ui->canvasSelectCombo, SIGNAL(activated(int)),
            this, SLOT(onCanvasSelected(int)));
    ui->canvasSelectCombo->hide();
}

TemporaryWidget::~TemporaryWidget()
{
    disconnect(&m_webSocket, &QWebSocket::disconnected, this, &TemporaryWidget::onSocketClosed);
    m_webSocket.close();
    delete ui;
}

void TemporaryWidget::setNetworkAccess(QNetworkAccessManager *dl)
{
    m_netAccess = dl;
}

void TemporaryWidget::onConnect()
{
    QUrl queryUrl;
    queryUrl.setScheme("http");
    queryUrl.setHost(ui->hostName->text());
    queryUrl.setPort(ui->portEdit->text().toInt());
    queryUrl.setPath("/json/canvas/by-index");
    queryUrl.setQuery("d=2");

    QNetworkReply* reply = m_netAccess->get(QNetworkRequest(queryUrl));
    connect(reply, &QNetworkReply::finished, this, &TemporaryWidget::onFinishedGetCanvasList);

    ui->connectButton->setEnabled(false);
}

QJsonObject jsonPropNodeFindChild(QJsonObject obj, QByteArray name)
{
    Q_FOREACH (QJsonValue v, obj.value("children").toArray()) {
        QJsonObject vo = v.toObject();
        if (vo.value("name").toString() == name) {
            return vo;
        }
    }

    return QJsonObject();
}

void TemporaryWidget::onFinishedGetCanvasList()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    reply->deleteLater();
    ui->connectButton->setEnabled(true);

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "failed to get canvases";
        // reset some state
        return;
    }

    ui->connectButton->hide();
    ui->canvasSelectCombo->show();

    QJsonDocument json = QJsonDocument::fromJson(reply->readAll());

    ui->canvasSelectCombo->clear();
    QJsonArray canvasArray = json.object().value("children").toArray();
    Q_FOREACH (QJsonValue canvasValue, canvasArray) {
        QJsonObject canvas = canvasValue.toObject();
        QString canvasName = jsonPropNodeFindChild(canvas, "name").value("value").toString();
        QString propPath = canvas.value("path").toString();
        ui->canvasSelectCombo->addItem(canvasName, propPath);
    }
}

void TemporaryWidget::onWebSocketConnected()
{
    connect(&m_webSocket, &QWebSocket::textMessageReceived,
            this, &TemporaryWidget::onTextMessageReceived);

    m_localPropertyRoot = new LocalProp(nullptr, NameIndexTuple(""));

    ui->canvas->setRootProperty(m_localPropertyRoot);
    ui->stack->setCurrentIndex(1);

    ui->canvas->rootElement()->createQuickItem(ui->quickWidget->rootObject());

    m_canvasModel = new CanvasTreeModel(ui->canvas->rootElement());
    ui->treeView->setModel(m_canvasModel);

    m_elementModel = new ElementDataModel(this);
    ui->elementData->setModel(m_elementModel);

    connect(ui->treeView->selectionModel(), &QItemSelectionModel::currentChanged, this, &TemporaryWidget::onTreeCurrentChanged);

    FGQCanvasFontCache::instance()->setHost(ui->hostName->text(),
                                            ui->portEdit->text().toInt());
    FGQCanvasImageLoader::instance()->setHost(ui->hostName->text(),
                                              ui->portEdit->text().toInt());
}

void TemporaryWidget::onCanvasSelected(int index)
{
    QString path = ui->canvasSelectCombo->itemData(index).toString();

    QUrl wsUrl;
    wsUrl.setScheme("ws");
    wsUrl.setHost(ui->hostName->text());
    wsUrl.setPort(ui->portEdit->text().toInt());
    wsUrl.setPath("/PropertyTreeMirror" + path);
    m_rootPropertyPath = path.toUtf8();

    connect(&m_webSocket, &QWebSocket::connected, this, &TemporaryWidget::onWebSocketConnected);
    connect(&m_webSocket, &QWebSocket::disconnected, this, &TemporaryWidget::onSocketClosed);

    saveSettings();

    qDebug() << "starting connection to:" << wsUrl;
    m_webSocket.open(wsUrl);
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
            if (nodePath.indexOf(m_rootPropertyPath) != 0) {
                qWarning() << "not a property path we are mirroring:" << nodePath;
                continue;
            }

            QByteArray localPath = nodePath.mid(m_rootPropertyPath.size() + 1);
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

void TemporaryWidget::onTreeCurrentChanged(const QModelIndex &current, const QModelIndex &previous)
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
    //settings.setValue("prop-path", ui->propertyPath->text());
}

void TemporaryWidget::restoreSettings()
{
    QSettings settings;
    ui->hostName->setText(settings.value("ws-host").toString());
    ui->portEdit->setText(settings.value("ws-port").toString());
  //  ui->propertyPath->setText(settings.value("prop-path").toString());
}

LocalProp *TemporaryWidget::propertyFromPath(QByteArray path) const
{
    return m_localPropertyRoot->getOrCreateWithPath(path);
}
