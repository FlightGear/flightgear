#ifndef POPUPWINDOWTRACKER_HXX
#define POPUPWINDOWTRACKER_HXX

#include <QObject>

class QWindow;

class PopupWindowTracker : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QWindow* window READ window WRITE setWindow NOTIFY windowChanged)
    QWindow* m_window = nullptr;

public:
    explicit PopupWindowTracker(QObject *parent = nullptr);

    ~PopupWindowTracker();

    QWindow* window() const
    {
        return m_window;
    }

signals:

    void windowChanged(QWindow* window);

public slots:
    void setWindow(QWindow* window);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
};

#endif // POPUPWINDOWTRACKER_HXX
