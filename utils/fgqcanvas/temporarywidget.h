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

#ifndef TEMPORARYWIDGET_H
#define TEMPORARYWIDGET_H

#include <QWidget>
#include <QtWebSockets/QWebSocket>
#include <QAction>

namespace Ui {
class TemporaryWidget;
}

class LocalProp;
class CanvasTreeModel;
class ElementDataModel;
class QNetworkAccessManager;

class TemporaryWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TemporaryWidget(QWidget *parent = 0);
    ~TemporaryWidget();

    void setNetworkAccess(QNetworkAccessManager* dl);
private Q_SLOTS:
    void onConnect();

    void onWebSocketConnected();
    void onTextMessageReceived(QString message);

    void onSocketClosed();

    void onTreeCurrentChanged(const QModelIndex &previous, const QModelIndex &current);
    void onCanvasSelected(int index);

    void onFinishedGetCanvasList();

    void onSave();
    void onLoadSnapshot();
private:
    void saveSettings();
    void restoreSettings();

    LocalProp* propertyFromPath(QByteArray path) const;

    void createdRootProperty();

    QNetworkAccessManager* m_netAccess;
    QWebSocket m_webSocket;
    Ui::TemporaryWidget *ui;
    QByteArray m_rootPropertyPath;

    LocalProp* m_localPropertyRoot = nullptr;
    QHash<int, LocalProp*> idPropertyDict;
    CanvasTreeModel* m_canvasModel;
    ElementDataModel* m_elementModel;
};

#endif // TEMPORARYWIDGET_H
