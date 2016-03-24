#include "PathsDialog.hxx"
#include "ui_PathsDialog.h"

#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>

#include "CatalogListModel.hxx"
#include "AddCatalogDialog.hxx"
#include "AircraftModel.hxx"

#include <Main/options.hxx>
#include <Main/globals.hxx>
#include <Network/HTTPClient.hxx>

PathsDialog::PathsDialog(QWidget *parent, simgear::pkg::RootRef root) :
    QDialog(parent),
    m_ui(new Ui::PathsDialog),
    m_packageRoot(root)
{
    m_ui->setupUi(this);
    
    m_catalogsModel = new CatalogListModel(this, m_packageRoot);
    m_ui->catalogsList->setModel(m_catalogsModel);

 // enable drag-drop to re-order the paths
    m_ui->sceneryPathsList->setDragEnabled(true);
    m_ui->sceneryPathsList->setDragDropMode(QAbstractItemView::InternalMove);
    m_ui->sceneryPathsList->setDropIndicatorShown(true);

    m_ui->aircraftPathsList->setDragEnabled(true);
    m_ui->aircraftPathsList->setDragDropMode(QAbstractItemView::InternalMove);
    m_ui->aircraftPathsList->setDropIndicatorShown(true);

    connect(m_ui->addCatalog, &QToolButton::clicked,
            this, &PathsDialog::onAddCatalog);
    connect(m_ui->addDefaultCatalogButton, &QPushButton::clicked,
            this, &PathsDialog::onAddDefaultCatalog);
    connect(m_ui->removeCatalog, &QToolButton::clicked,
            this, &PathsDialog::onRemoveCatalog);
            
    connect(m_ui->addSceneryPath, &QToolButton::clicked,
            this, &PathsDialog::onAddSceneryPath);
    connect(m_ui->removeSceneryPath, &QToolButton::clicked,
            this, &PathsDialog::onRemoveSceneryPath);

    connect(m_ui->addAircraftPath, &QToolButton::clicked,
            this, &PathsDialog::onAddAircraftPath);
    connect(m_ui->removeAircraftPath, &QToolButton::clicked,
            this, &PathsDialog::onRemoveAircraftPath);

    connect(m_ui->changeDownloadDir, &QPushButton::clicked,
            this, &PathsDialog::onChangeDownloadDir);

    connect(m_ui->clearDownloadDir, &QPushButton::clicked,
            this, &PathsDialog::onClearDownloadDir);

    QSettings settings;
            
    QStringList sceneryPaths = settings.value("scenery-paths").toStringList();
    m_ui->sceneryPathsList->addItems(sceneryPaths);

    QStringList aircraftPaths = settings.value("aircraft-paths").toStringList();
    m_ui->aircraftPathsList->addItems(aircraftPaths);

    QVariant downloadDir = settings.value("download-dir");
    if (downloadDir.isValid()) {
        m_downloadDir = downloadDir.toString();
    }

    updateUi();
}

PathsDialog::~PathsDialog()
{
    delete m_ui;
}

void PathsDialog::accept()
{
    QSettings settings;
    QStringList paths;
    for (int i=0; i<m_ui->sceneryPathsList->count(); ++i) {
        paths.append(m_ui->sceneryPathsList->item(i)->text());
    }

    settings.setValue("scenery-paths", paths);
    paths.clear();

    for (int i=0; i<m_ui->aircraftPathsList->count(); ++i) {
        paths.append(m_ui->aircraftPathsList->item(i)->text());
    }

    settings.setValue("aircraft-paths", paths);

    if (m_downloadDir.isEmpty()) {
        settings.remove("download-dir");
    } else {
        settings.setValue("download-dir", m_downloadDir);
    }
    
    QDialog::accept();
}

void PathsDialog::onAddSceneryPath()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("Choose scenery folder"));
    if (!path.isEmpty()) {
        m_ui->sceneryPathsList->addItem(path);
    }

    // work around a Qt OS-X bug - this dialog is ending ordered
    // behind the main settings dialog (consequence of modal-dialog
    // showing a modla dialog showing a modial dialog)
    window()->raise();
}

void PathsDialog::onRemoveSceneryPath()
{
    if (m_ui->sceneryPathsList->currentItem()) {
        delete m_ui->sceneryPathsList->currentItem();
    }
}

void PathsDialog::onAddAircraftPath()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("Choose aircraft folder"));
    if (!path.isEmpty()) {
        // the user might add a directory containing an 'Aircraft' subdir. Let's attempt
        // to check for that case and handle it gracefully.
        bool pathOk = false;

        if (AircraftItemModel::isCandidateAircraftPath(path)) {
            m_ui->aircraftPathsList->addItem(path);
            pathOk = true;
        } else {
            // no aircraft in speciied path, look for Aircraft/ subdir
            QDir d(path);
            if (d.exists("Aircraft")) {
                QString p2 = d.filePath("Aircraft");
                if (AircraftItemModel::isCandidateAircraftPath(p2)) {
                    m_ui->aircraftPathsList->addItem(p2);
                    pathOk = true;
                }
            }
        }

        if (!pathOk) {
            QMessageBox mb;
            mb.setText(QString("No aircraft found in the folder '%1' - add anyway?").arg(path));
            mb.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            mb.setDefaultButton(QMessageBox::No);
            mb.exec();

            if (mb.result() == QMessageBox::Yes) {
                m_ui->aircraftPathsList->addItem(path);
            }
        }
    }
    // work around a Qt OS-X bug - this dialog is ending ordered
    // behind the main settings dialog (consequence of modal-dialog
    // showing a modla dialog showing a modial dialog)
    window()->raise();
}

void PathsDialog::onRemoveAircraftPath()
{
    if (m_ui->aircraftPathsList->currentItem()) {
        delete m_ui->aircraftPathsList->currentItem();
    }
}

void PathsDialog::onAddCatalog()
{
    QScopedPointer<AddCatalogDialog> dlg(new AddCatalogDialog(this, m_packageRoot));
    dlg->exec();
    if (dlg->result() == QDialog::Accepted) {
        m_catalogsModel->refresh();
    }
}

void PathsDialog::onAddDefaultCatalog()
{
    // check it's not a duplicate somehow
    FGHTTPClient* http = globals->get_subsystem<FGHTTPClient>();
    if (http->isDefaultCatalogInstalled())
        return;

     QScopedPointer<AddCatalogDialog> dlg(new AddCatalogDialog(this, m_packageRoot));
     QUrl url(QString::fromStdString(http->getDefaultCatalogUrl()));
     dlg->setUrlAndDownload(url);
     dlg->exec();
     if (dlg->result() == QDialog::Accepted) {
         m_catalogsModel->refresh();
         updateUi();
     }
}

void PathsDialog::onRemoveCatalog()
{
    QModelIndex mi = m_ui->catalogsList->currentIndex();
    FGHTTPClient* http = globals->get_subsystem<FGHTTPClient>();

    if (mi.isValid()) {
        QString s = QString("Remove aircraft hangar '%1'? All installed aircraft from this "
                            "hangar will be removed.");
        QString pkgId = mi.data(CatalogIdRole).toString();

        if (pkgId.toStdString() == http->getDefaultCatalogId()) {
            s = QString("Remove default aircraft hangar? "
                        "This hangar contains all the default aircraft included with FlightGear. "
                        "If you change your mind in the future, click the 'restore' button.");
        } else {
            s = s.arg(mi.data(Qt::DisplayRole).toString());
        }

        QMessageBox mb;
        mb.setText(s);
        mb.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        mb.setDefaultButton(QMessageBox::No);
        mb.exec();
        
        if (mb.result() == QMessageBox::Yes) {
            m_packageRoot->removeCatalogById(pkgId.toStdString());
        }
    }

    updateUi();
}

void PathsDialog::onChangeDownloadDir()
{
    QString path = QFileDialog::getExistingDirectory(this,
                                                     tr("Choose downloads folder"),
                                                     m_downloadDir);
    if (!path.isEmpty()) {
        m_downloadDir = path;
        updateUi();
    }
}

void PathsDialog::onClearDownloadDir()
{
    // does this need an 'are you sure'?
    m_downloadDir.clear();
    updateUi();
}

void PathsDialog::updateUi()
{
    QString s = m_downloadDir;
    if (s.isEmpty()) {
        s = QString::fromStdString(flightgear::defaultDownloadDir());
        s.append(tr(" (default)"));
        m_ui->clearDownloadDir->setEnabled(false);
    } else {
        m_ui->clearDownloadDir->setEnabled(true);
    }

    QString m = tr("Download location: %1").arg(s);
    m_ui->downloadLocation->setText(m);

    FGHTTPClient* http = globals->get_subsystem<FGHTTPClient>();
    m_ui->addDefaultCatalogButton->setEnabled(!http->isDefaultCatalogInstalled());
}
