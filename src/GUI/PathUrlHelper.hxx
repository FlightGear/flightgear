#ifndef PATHURLHELPER_H
#define PATHURLHELPER_H

#include <QObject>
#include <QUrl>

class FileDialogWrapper : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QUrl folder READ folder WRITE setFolder NOTIFY folderChanged)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(bool selectFolder READ selectFolder WRITE setSelectFolder NOTIFY selectFolderChanged)
    Q_PROPERTY(QUrl fileUrl READ fileUrl WRITE setFileUrl NOTIFY fileUrlChanged)
    Q_PROPERTY(QString filePath READ filePath WRITE setFilePath NOTIFY fileUrlChanged)
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)
public:
    explicit FileDialogWrapper(QObject *parent = nullptr);

    Q_INVOKABLE QString urlToLocalFilePath(QUrl url) const;

    Q_INVOKABLE QUrl urlFromLocalFilePath(QString path) const;

    Q_INVOKABLE void open();

    QUrl folder() const;

    QString title() const;

    bool selectFolder() const;

    QUrl fileUrl() const;

    QString filePath() const;

    QString filter() const;

signals:

    void accepted();
    void rejected();

    void folderChanged(QUrl folder);

    void titleChanged(QString title);

    void selectFolderChanged(bool selectFolder);

    void fileUrlChanged(QUrl fileUrl);

    void filterChanged(QString filter);

public slots:

    void setFolder(QUrl folder);

    void setTitle(QString title);

    void setSelectFolder(bool selectFolder);

    void setFileUrl(QUrl fileUrl);

    void setFilePath(QString filePath);

    void setFilter(QString filter);

private:
    QUrl m_currentFolder;
    QUrl m_fileUrl;
    bool m_selectFolder = false;
    QString m_dialogTitle;
    QString m_filter;
};

#endif // PATHURLHELPER_H
