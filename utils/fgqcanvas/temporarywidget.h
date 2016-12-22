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

class TemporaryWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TemporaryWidget(QWidget *parent = 0);
    ~TemporaryWidget();

private Q_SLOTS:
    void onStartConnect();

    void onConnected();
    void onTextMessageReceived(QString message);

    void onSocketClosed();

    void onCanvasTreeElementClicked(const QModelIndex& index);

private:
    void saveSettings();
    void restoreSettings();

    LocalProp* propertyFromPath(QByteArray path) const;

    QWebSocket m_webSocket;
    Ui::TemporaryWidget *ui;
    QByteArray rootPropertyPath;

    LocalProp* m_localPropertyRoot = nullptr;
    QHash<int, LocalProp*> idPropertyDict;
    CanvasTreeModel* m_canvasModel;
    ElementDataModel* m_elementModel;
};

#endif // TEMPORARYWIDGET_H
