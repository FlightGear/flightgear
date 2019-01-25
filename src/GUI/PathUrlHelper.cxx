// PathUrlHelper.cxx - manage a stack of QML items
//
// Written by James Turner, started March 2018
//
// Copyright (C) 2018 James Turner <james@flightgear.org>
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


#include "PathUrlHelper.hxx"

#include <QFileDialog>

FileDialogWrapper::FileDialogWrapper(QObject *parent) : QObject(parent)
{

}

QString FileDialogWrapper::urlToLocalFilePath(QUrl url) const
{
    return url.toLocalFile();
}

QUrl FileDialogWrapper::urlFromLocalFilePath(QString path) const
{
    return QUrl::fromLocalFile(path);
}

void FileDialogWrapper::open()
{
    QUrl u;
    if (m_selectFolder) {
        u = QFileDialog::getExistingDirectoryUrl(nullptr, m_dialogTitle,
                                             m_currentFolder);

    } else {
        u = QFileDialog::getOpenFileUrl(nullptr, m_dialogTitle, m_currentFolder,
                                        m_filter);
    }

    if (u.isValid()) {
        m_fileUrl = u;
        accepted();
    } else {
        rejected();
    }
}

QUrl FileDialogWrapper::folder() const
{
    return m_currentFolder;
}

QString FileDialogWrapper::title() const
{
    return m_dialogTitle;
}

bool FileDialogWrapper::selectFolder() const
{
    return m_selectFolder;
}

QUrl FileDialogWrapper::fileUrl() const
{
    return m_fileUrl;
}

QString FileDialogWrapper::filePath() const
{
    return urlToLocalFilePath(fileUrl());
}

QString FileDialogWrapper::filter() const
{
    return m_filter;
}

void FileDialogWrapper::setFolder(QUrl folder)
{
    if (m_currentFolder == folder)
        return;

    m_currentFolder = folder;
    emit folderChanged(m_currentFolder);
}

void FileDialogWrapper::setTitle(QString title)
{
    if (m_dialogTitle == title)
        return;

    m_dialogTitle = title;
    emit titleChanged(m_dialogTitle);
}

void FileDialogWrapper::setSelectFolder(bool selectFolder)
{
    if (m_selectFolder == selectFolder)
        return;

    m_selectFolder = selectFolder;
    emit selectFolderChanged(m_selectFolder);
}

void FileDialogWrapper::setFileUrl(QUrl fileUrl)
{
    if (m_fileUrl == fileUrl)
        return;

    m_fileUrl = fileUrl;
    emit fileUrlChanged(m_fileUrl);
}

void FileDialogWrapper::setFilePath(QString filePath)
{
    setFileUrl(urlFromLocalFilePath(filePath));
}

void FileDialogWrapper::setFilter(QString filter)
{
    if (m_filter == filter)
        return;

    m_filter = filter;
    emit filterChanged(m_filter);
}
