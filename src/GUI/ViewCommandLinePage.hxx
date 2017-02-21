#ifndef VIEWCOMMANDLINEPAGE_HXX
#define VIEWCOMMANDLINEPAGE_HXX

#include <QWidget>

class QTextEdit;
class LaunchConfig;

class ViewCommandLinePage : public QWidget
{
    Q_OBJECT
public:
    explicit ViewCommandLinePage(QWidget *parent = 0);

    void setLaunchConfig(LaunchConfig* config);

    void update();
signals:

public slots:

private:
    QTextEdit* m_browser;
    LaunchConfig* m_config = nullptr;
};

#endif // VIEWCOMMANDLINEPAGE_HXX
