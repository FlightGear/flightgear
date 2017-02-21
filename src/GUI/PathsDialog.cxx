#include "PathsDialog.hxx"
#include "ui_PathsDialog.h"

#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QProcess>

#include "CatalogListModel.hxx"
#include "AddCatalogDialog.hxx"
#include "AircraftModel.hxx"
#include "InstallSceneryDialog.hxx"
#include "QtLauncher.hxx"

#include <Main/options.hxx>
#include <Main/globals.hxx>
#include <Network/HTTPClient.hxx>

AddOnsPage::AddOnsPage(QWidget *parent, simgear::pkg::RootRef root) :
    QWidget(parent),
    m_ui(new Ui::AddOnsPage),
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
            this, &AddOnsPage::onAddCatalog);
    connect(m_ui->addDefaultCatalogButton, &QPushButton::clicked,
            this, &AddOnsPage::onAddDefaultCatalog);
    connect(m_ui->removeCatalog, &QToolButton::clicked,
            this, &AddOnsPage::onRemoveCatalog);
            
    connect(m_ui->addSceneryPath, &QToolButton::clicked,
            this, &AddOnsPage::onAddSceneryPath);
    connect(m_ui->removeSceneryPath, &QToolButton::clicked,
            this, &AddOnsPage::onRemoveSceneryPath);

    connect(m_ui->addAircraftPath, &QToolButton::clicked,
            this, &AddOnsPage::onAddAircraftPath);
    connect(m_ui->removeAircraftPath, &QToolButton::clicked,
            this, &AddOnsPage::onRemoveAircraftPath);

    connect(m_ui->installSceneryButton, &QPushButton::clicked,
            this, &AddOnsPage::onInstallScenery);

    m_ui->sceneryPathsList->setToolTip(
      tr("After changing this list, please restart the launcher to avoid "
         "possibly inconsistent behavior."));
    m_ui->installSceneryButton->setToolTip(
      tr("After installing scenery, you may have to restart the launcher "
         "to avoid inconsistent behavior."));

    QSettings settings;
            
    QStringList sceneryPaths = settings.value("scenery-paths").toStringList();
    m_ui->sceneryPathsList->addItems(sceneryPaths);

    QStringList aircraftPaths = settings.value("aircraft-paths").toStringList();
    m_ui->aircraftPathsList->addItems(aircraftPaths);

    updateUi();
}

AddOnsPage::~AddOnsPage()
{
    delete m_ui;
}

void AddOnsPage::onAddSceneryPath()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("Choose scenery folder"));
    if (!path.isEmpty()) {
        // validation

        SGPath p(path.toStdString());
        bool isValid = false;

        for (const auto& dir: {"Objects", "Terrain", "Buildings", "Roads",
                               "Pylons", "NavData"}) {
            if ((p / dir).exists()) {
                isValid = true;
                break;
            }
        }

        if (!isValid) {
            QMessageBox mb;
            mb.setText(QString("The folder '%1' doesn't appear to contain scenery - add anyway?").arg(path));
            mb.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            mb.setDefaultButton(QMessageBox::No);
            mb.setInformativeText(
                "Added scenery should contain at least one of the following "
                "folders: Objects, Terrain, Buildings, Roads, Pylons, NavData.");
            mb.exec();

            if (mb.result() == QMessageBox::No) {
                return;
            }
        }

        m_ui->sceneryPathsList->addItem(path);
        saveSceneryPaths();
    }
}

void AddOnsPage::onRemoveSceneryPath()
{
    if (m_ui->sceneryPathsList->currentItem()) {
        delete m_ui->sceneryPathsList->currentItem();
        saveSceneryPaths();
    }
}

void AddOnsPage::onAddAircraftPath()
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

        saveAircraftPaths();
        emit aircraftPathsChanged();
    }
}

void AddOnsPage::onRemoveAircraftPath()
{
    if (m_ui->aircraftPathsList->currentItem()) {
        delete m_ui->aircraftPathsList->currentItem();
        saveAircraftPaths();
        emit aircraftPathsChanged();
    }
}

void AddOnsPage::saveAircraftPaths()
{
    QSettings settings;
    QStringList paths;

    for (int i=0; i<m_ui->aircraftPathsList->count(); ++i) {
        paths.append(m_ui->aircraftPathsList->item(i)->text());
    }

    settings.setValue("aircraft-paths", paths);
}

void AddOnsPage::saveSceneryPaths()
{
    QSettings settings;
    QStringList paths;
    for (int i=0; i<m_ui->sceneryPathsList->count(); ++i) {
        paths.append(m_ui->sceneryPathsList->item(i)->text());
    }

    settings.setValue("scenery-paths", paths);

    emit sceneryPathsChanged();
}

bool AddOnsPage::haveSceneryPath(QString path) const
{
    for (int i=0; i<m_ui->sceneryPathsList->count(); ++i) {
        if (m_ui->sceneryPathsList->item(i)->text() == path) {
            return true;
        }
    }

    return false;
}

void AddOnsPage::onAddCatalog()
{
    QScopedPointer<AddCatalogDialog> dlg(new AddCatalogDialog(this, m_packageRoot));
    dlg->exec();
    if (dlg->result() == QDialog::Accepted) {
        m_catalogsModel->refresh();
    }
}

void AddOnsPage::onAddDefaultCatalog()
{
    addDefaultCatalog(this, false /* not silent */);

    m_catalogsModel->refresh();
    updateUi();
}

void AddOnsPage::addDefaultCatalog(QWidget* pr, bool silent)
{
    // check it's not a duplicate somehow
    FGHTTPClient* http = globals->get_subsystem<FGHTTPClient>();
    if (http->isDefaultCatalogInstalled())
        return;

    QScopedPointer<AddCatalogDialog> dlg(new AddCatalogDialog(pr, globals->packageRoot()));
    QUrl url(QString::fromStdString(http->getDefaultCatalogUrl()));
    if (silent) {
        dlg->setNonInteractiveMode();
    }
    dlg->setUrlAndDownload(url);
    dlg->exec();

}

void AddOnsPage::onRemoveCatalog()
{
    QModelIndex mi = m_ui->catalogsList->currentIndex();
    FGHTTPClient* http = globals->get_subsystem<FGHTTPClient>();

    if (mi.isValid()) {
        QString s = QString("Remove aircraft hangar '%1'? All installed aircraft from this "
                            "hangar will be removed.");
        QString pkgId = mi.data(CatalogIdRole).toString();

        if (pkgId.toStdString() == http->getDefaultCatalogId()) {
            s = QString("Remove the default aircraft hangar? "
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

void AddOnsPage::onInstallScenery()
{
    QSettings settings;
    QString downloadDir = settings.value("download-dir").toString();
    InstallSceneryDialog dlg(this, downloadDir);
    if (dlg.exec() == QDialog::Accepted) {
        if (!haveSceneryPath(dlg.sceneryPath())) {
            m_ui->sceneryPathsList->addItem(dlg.sceneryPath());
            saveSceneryPaths();
        }
    }
}

void AddOnsPage::updateUi()
{
    FGHTTPClient* http = globals->get_subsystem<FGHTTPClient>();
    m_ui->addDefaultCatalogButton->setEnabled(!http->isDefaultCatalogInstalled());
}
