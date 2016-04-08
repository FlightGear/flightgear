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
#include "QtLauncher_private.hxx"

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

    connect(m_ui->changeDownloadDir, &QPushButton::clicked,
            this, &AddOnsPage::onChangeDownloadDir);

    connect(m_ui->clearDownloadDir, &QPushButton::clicked,
            this, &AddOnsPage::onClearDownloadDir);

    connect(m_ui->changeDataDir, &QPushButton::clicked,
            this, &AddOnsPage::onChangeDataDir);

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

AddOnsPage::~AddOnsPage()
{
    delete m_ui;
}

void AddOnsPage::onAddSceneryPath()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("Choose scenery folder"));
    if (!path.isEmpty()) {
        m_ui->sceneryPathsList->addItem(path);
        saveSceneryPaths();
    }

    // work around a Qt OS-X bug - this dialog is ending ordered
    // behind the main settings dialog (consequence of modal-dialog
    // showing a modla dialog showing a modial dialog)
    window()->raise();
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
    }
    // work around a Qt OS-X bug - this dialog is ending ordered
    // behind the main settings dialog (consequence of modal-dialog
    // showing a modla dialog showing a modial dialog)
    window()->raise();
}

void AddOnsPage::onRemoveAircraftPath()
{
    if (m_ui->aircraftPathsList->currentItem()) {
        delete m_ui->aircraftPathsList->currentItem();
        saveAircraftPaths();
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

void AddOnsPage::onRemoveCatalog()
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

void AddOnsPage::onChangeDownloadDir()
{
    QString path = QFileDialog::getExistingDirectory(this,
                                                     tr("Choose downloads folder"),
                                                     m_downloadDir);
    if (path.isEmpty()) {
        return; // user cancelled
    }

    m_downloadDir = path;
    setDownloadDir();
}

void AddOnsPage::onClearDownloadDir()
{
    // does this need an 'are you sure'?
    m_downloadDir.clear();

    setDownloadDir();
}

void AddOnsPage::setDownloadDir()
{
    QSettings settings;
    if (m_downloadDir.isEmpty()) {
        settings.remove("download-dir");
    } else {
        settings.setValue("download-dir", m_downloadDir);
    }

    if (m_downloadDir.isEmpty()) {
        flightgear::Options::sharedInstance()->clearOption("download-dir");
    } else {
        flightgear::Options::sharedInstance()->setOption("download-dir", m_downloadDir.toStdString());
    }

    emit downloadDirChanged();
    updateUi();
}

void AddOnsPage::onChangeDataDir()
{
    QMessageBox mbox(this);
    mbox.setText(tr("Change the data files used by FlightGear?"));
    mbox.setInformativeText(tr("FlightGear requires additional files to operate. "
                               "(Also called the base package, or fg-data) "
                               "You can restart FlightGear and choose a "
                               "different data files location, or restore the default setting."));
    QPushButton* quitButton = mbox.addButton(tr("Restart FlightGear now"), QMessageBox::YesRole);
    mbox.addButton(QMessageBox::Cancel);
    mbox.setDefaultButton(QMessageBox::Cancel);
    mbox.setIconPixmap(QPixmap(":/app-icon-large"));

    mbox.exec();
    if (mbox.clickedButton() != quitButton) {
        return;
    }

    {
        QSettings settings;
        // set the option to the magic marker value
        settings.setValue("fg-root", "!ask");
    } // scope the ensure settings are written nicely

    QtLauncher::restartTheApp(QStringList());
}

void AddOnsPage::updateUi()
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

    QString dataLoc;
    QSettings settings;
    QString root = settings.value("fg-root").toString();
    if (root.isNull()) {
        dataLoc = tr("built-in");
    } else {
        dataLoc = root;
    }

    m_ui->dataLocation->setText(tr("Data location: %1").arg(dataLoc));


    FGHTTPClient* http = globals->get_subsystem<FGHTTPClient>();
    m_ui->addDefaultCatalogButton->setEnabled(!http->isDefaultCatalogInstalled());
}
