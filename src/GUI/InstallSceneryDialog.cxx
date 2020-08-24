// InstallSceneryDialog.cxx - part of GUI launcher using Qt5
//
// Written by James Turner, started June 2016.
//
// Copyright (C) 2016 James Turner <zakalawe@mac.com>
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

#include "config.h"

#include "InstallSceneryDialog.hxx"
#include "ui_InstallSceneryDialog.h"

#include <QPushButton>
#include <QDebug>
#include <QFileDialog>
#include <QThread>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QDir>
#include <QStandardPaths>

#include <Main/globals.hxx>
#include <Main/options.hxx>

#include <simgear/io/untar.hxx>

class SceneryExtractor : public simgear::ArchiveExtractor
{
public:
    SceneryExtractor(const SGPath& root) :
        ArchiveExtractor(root)
    {}
protected:

    auto filterPath(std::string& path) -> PathResult override
    {
        if ((path.find("Objects/") == 0) || (path.find("Terrain/") == 0)) {
            return Accepted;
        }
        
        path = "Terrain/" + path;
        return Modified;
    }
};

class InstallSceneryThread : public QThread
{
    Q_OBJECT
public:
    InstallSceneryThread(QString extractDir, QStringList files) :
        m_extractDir(extractDir),
        m_remainingPaths(files),
        m_error(false),
        m_totalBytes(0),
        m_bytesRead(0)
    {

    }

    virtual void run()
    {
        // pre-check each file
        Q_FOREACH (QString path, m_remainingPaths) {
            QFileInfo finfo(path);
            QString baseName = finfo.baseName();
            QRegularExpression re("[e|w]\\d{2}0[n|s]\\d0", QRegularExpression::CaseInsensitiveOption);
            Q_ASSERT(re.isValid());
            if (!re.match(baseName).hasMatch()) {
                emit extractionError(path,tr("scenery archive name is not correct."));
                m_error = true;
                return;
            }

            QFile f(path);
            f.open(QIODevice::ReadOnly);
            QByteArray firstData = f.read(8192);

            auto archiveType = simgear::ArchiveExtractor::determineType((uint8_t*) firstData.data(), firstData.count());
            if (archiveType == simgear::ArchiveExtractor::Invalid) {
                emit extractionError(path,tr("file does not appear to be a scenery archive."));
                m_error = true;
                return;
            }

            m_totalBytes += f.size();
        }

        while (!m_remainingPaths.isEmpty() && !m_error) {
            extractNextArchive();
        }
    }

signals:
    void extractionError(QString file, QString msg);

    void progress(int percent);

    void extractingArchive(QString archiveName);
private:
    void extractNextArchive()
    {
        SGPath root(m_extractDir.toStdString());
        m_archive.reset(new SceneryExtractor(root));

        QString path = m_remainingPaths.front();
        m_remainingPaths.pop_front();
        QFileInfo finfo(path);

        emit extractingArchive(path);

        QFile f(path);
        f.open(QIODevice::ReadOnly);
        Q_ASSERT(f.isOpen());

        while (!f.atEnd()) {
            QByteArray bytes = f.read(4 * 1024 * 1024);
            m_archive->extractBytes((const uint8_t*) bytes.constData(), bytes.size());
            m_bytesRead += bytes.size();

            if (m_archive->hasError()) {
                break;
            }

            emit progress((m_bytesRead * 100) / m_totalBytes);
        }

        m_archive->flush();
        if (m_archive->hasError() || !m_archive->isAtEndOfArchive()) {
            emit extractionError(path, tr("unarchiving failed"));
            m_error = true;
            // try to clean up?
        }
    }

    QString m_extractDir;
    QStringList m_remainingPaths;
    std::unique_ptr<simgear::ArchiveExtractor> m_archive;
    bool m_error;
    quint64 m_totalBytes;
    quint64 m_bytesRead;
};

InstallSceneryDialog::InstallSceneryDialog(QWidget *parent, QString downloadDir) :
    QDialog(parent, Qt::Dialog
                    | Qt::CustomizeWindowHint
                    | Qt::WindowTitleHint
                    | Qt::WindowSystemMenuHint
                    | Qt::WindowContextHelpButtonHint
                    | Qt::MSWindowsFixedSizeDialogHint),
    m_state(STATE_START),
    m_downloadDir(downloadDir),
    ui(new Ui::InstallSceneryDialog)
{
    ui->setupUi(this);
    if (m_downloadDir.isEmpty()) {
        m_downloadDir = QString::fromStdString(flightgear::defaultDownloadDir().utf8Str());
    }

    QString baseIntroString = ui->introText->text();
    ui->introText->setText(baseIntroString.arg(m_downloadDir));
    updateUi();
}

InstallSceneryDialog::~InstallSceneryDialog()
{
    delete ui;
}

void InstallSceneryDialog::updateUi()
{
    QPushButton* b = ui->buttonBox->button(QDialogButtonBox::Ok);
    QPushButton* cancel = ui->buttonBox->button(QDialogButtonBox::Cancel);

    switch (m_state) {
    case STATE_START:
        b->setText(tr("Next"));
     //   b->setEnabled(m_catalogUrl.isValid() && !m_catalogUrl.isRelative());
        break;

    case STATE_EXTRACTING:
        b->setEnabled(false);
        cancel->setEnabled(false);
        ui->progressText->setText(tr("Extracting"));
        ui->stack->setCurrentIndex(1);
        break;

    case STATE_EXTRACT_FAILED:
        b->setEnabled(false);
        cancel->setEnabled(true);
        ui->stack->setCurrentIndex(2);
        break;

    case STATE_FINISHED:
        b->setEnabled(true);
        cancel->setEnabled(false);
        b->setText(tr("Okay"));
        ui->stack->setCurrentIndex(2);
        QString basicDesc = ui->resultsSummaryLabel->text();

        ui->resultsSummaryLabel->setText(basicDesc.arg(sceneryPath()));
        break;
    }
}


void InstallSceneryDialog::accept()
{
    switch (m_state) {
    case STATE_START:
        pickFiles();
        break;

    case STATE_EXTRACTING:
    case STATE_EXTRACT_FAILED:
        // can't happen, button is disabled
        break;

    case STATE_FINISHED:
        // check if download path is in scenery list, add if not
        QDialog::accept();
        break;
    }
}

void InstallSceneryDialog::reject()
{

    QDialog::reject();
}

void InstallSceneryDialog::pickFiles()
{
    QStringList downloads = QStandardPaths::standardLocations(QStandardPaths::DownloadLocation);
    QStringList files = QFileDialog::getOpenFileNames(this, tr("Choose scenery to install"),
                                                      downloads.first(), "Compressed data (*.tar *.gz *.tgz *.zip)");
    if (!files.isEmpty()) {
        QDir d(m_downloadDir);
        if (!d.exists("Scenery")) {
            d.mkdir("Scenery");
        }

        m_state = STATE_EXTRACTING;
        m_thread.reset(new InstallSceneryThread(d.filePath("Scenery"), files));
        // connect up some signals
        connect(m_thread.data(), &QThread::finished, this, &InstallSceneryDialog::onThreadFinished);
        connect(m_thread.data(), &InstallSceneryThread::extractionError,
                this, &InstallSceneryDialog::onExtractError);
        connect(m_thread.data(), &InstallSceneryThread::progress,
                this, &InstallSceneryDialog::onExtractProgress);
        connect(m_thread.data(), &InstallSceneryThread::extractingArchive,
                this, &InstallSceneryDialog::onExtractFile);
        updateUi();
        m_thread->start();
    } else {
        // user cancelled file dialog, cancel us as well
        QDialog::reject();
    }
}

void InstallSceneryDialog::onThreadFinished()
{
    m_state = STATE_FINISHED;
    updateUi();
}

void InstallSceneryDialog::onExtractError(QString file, QString msg)
{
    ui->resultsSummaryLabel->setText(tr("Problems occured extracting the archive '%1': %2").arg(file).arg(msg));
    m_state = STATE_EXTRACT_FAILED;
    updateUi();
}

void InstallSceneryDialog::onExtractProgress(int percent)
{
    ui->progressBar->setValue(percent);
}

void InstallSceneryDialog::onExtractFile(QString file)
{
    ui->progressText->setText(tr("Extracting %1").arg(file));
}

QString InstallSceneryDialog::sceneryPath()
{
    if (m_state == STATE_FINISHED) {
        QDir d(m_downloadDir);
        return d.filePath("Scenery");
    }

    return QString();
}

#include "InstallSceneryDialog.moc"
