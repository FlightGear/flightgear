#include "PathUrlHelper.hxx"

PathUrlHelper::PathUrlHelper(QObject *parent) : QObject(parent)
{

}

QString PathUrlHelper::urlToLocalFilePath(QUrl url) const
{
    return url.toLocalFile();
}

QUrl PathUrlHelper::urlFromLocalFilePath(QString path) const
{
    return QUrl::fromLocalFile(path);
}
