#ifndef HOVERAREA_HXX
#define HOVERAREA_HXX

#include <QQuickItem>

class HoverArea : public QQuickItem
{
    Q_OBJECT

    Q_PROPERTY(bool containsMouse READ containsMouse NOTIFY containsMouseChanged);

public:
    HoverArea();

    bool containsMouse() const
    {
        return m_containsMouse;
    }

signals:

    void containsMouseChanged(bool containsMouse);

protected:
    bool eventFilter(QObject* sender, QEvent* event) override;

private:
    bool m_containsMouse = false;
};

#endif // HOVERAREA_HXX
