// AddCatalogDialog.cxx - part of GUI launcher using Qt5
//
// Written by James Turner, started March 2015.
//
// Copyright (C) 2015 James Turner <zakalawe@mac.com>
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

#include "AddCatalogDialog.hxx"
#include "ui_AddCatalogDialog.h"

#include <QPushButton>
#include <QDebug>

#include <Include/version.h>

using namespace simgear::pkg;

AddCatalogDialog::AddCatalogDialog(QWidget *parent, RootRef root) :
    QDialog(parent),
    m_state(STATE_START),
    ui(new Ui::AddCatalogDialog),
    m_packageRoot(root)
{
    ui->setupUi(this);

    connect(ui->urlEdit, &QLineEdit::textEdited,
            this, &AddCatalogDialog::onUrlTextChanged);

    updateUi();
}

AddCatalogDialog::~AddCatalogDialog()
{
    delete ui;
}

CatalogRef AddCatalogDialog::addedCatalog()
{
    return m_result;
}

void AddCatalogDialog::onUrlTextChanged()
{
    m_catalogUrl = QUrl::fromUserInput(ui->urlEdit->text());
    updateUi();
}

void AddCatalogDialog::updateUi()
{
    QPushButton* b = ui->buttonBox->button(QDialogButtonBox::Ok);

    switch (m_state) {
    case STATE_START:
        b->setText(tr("Next"));
        b->setEnabled(m_catalogUrl.isValid() && !m_catalogUrl.isRelative());
        break;

    case STATE_DOWNLOADING:
        b->setEnabled(false);
        break;

    case STATE_DOWNLOAD_FAILED:
        b->setEnabled(false);
        break;

    case STATE_FINISHED:
        b->setEnabled(true);
        b->setText(tr("Okay"));
        break;
    }

    if (m_state == STATE_FINISHED) {
        QString catDesc = QString::fromStdString(m_result->description());
        QString s = tr("Successfully retrieved aircraft information from '%1'. "
                       "%2 aircraft are included in this hangar.").arg(catDesc).arg(m_result->packages().size());
        ui->resultsSummaryLabel->setText(s);
    } else if (m_state == STATE_DOWNLOAD_FAILED) {
        Delegate::FailureCode code = m_result->status();
        qWarning() << Q_FUNC_INFO << "failed with code" << code;
        QString s;
        switch (code) {
        case Delegate::FAIL_DOWNLOAD:
            s =  tr("Failed to download aircraft descriptions from '%1'. "
                    "Check the address (URL) and your network connection.").arg(m_catalogUrl.toString());
            break;

        case Delegate::FAIL_VERSION:
            s = tr("The provided hangar is for a different version of FLightGear. "
                   "(This is version %1)").arg(QString::fromUtf8(FLIGHTGEAR_VERSION));
            break;

        default:
            s = tr("Unknown error occured trying to set up the hangar.");
        }

        ui->resultsSummaryLabel->setText(s);
    }
}

void AddCatalogDialog::startDownload()
{
    Q_ASSERT(m_catalogUrl.isValid());

    m_result = Catalog::createFromUrl(m_packageRoot, m_catalogUrl.toString().toStdString());
    m_result->addStatusCallback(this, &AddCatalogDialog::onCatalogStatusChanged);
    m_state = STATE_DOWNLOADING;
    updateUi();
    ui->stack->setCurrentIndex(STATE_DOWNLOADING);
}

void AddCatalogDialog::accept()
{
    switch (m_state) {
    case STATE_START:
        startDownload();
        break;

    case STATE_DOWNLOADING:
    case STATE_DOWNLOAD_FAILED:
        // can't happen, button is disabled
        break;

    case STATE_FINISHED:
        QDialog::accept();
        break;
    }
}

void AddCatalogDialog::reject()
{
    if (m_result && !m_result->id().empty()) {
        // user may have successfully download the catalog, but choosen
        // not to add it. so remove it here
        m_packageRoot->removeCatalogById(m_result->id());
    }

    QDialog::reject();
}

void AddCatalogDialog::onCatalogStatusChanged(Catalog* cat)
{
    Delegate::FailureCode s = cat->status();
    qDebug() << Q_FUNC_INFO << "cat status:" << s;
    switch (s) {
    case Delegate::CATALOG_REFRESHED:
        m_state = STATE_FINISHED;
        break;

    case Delegate::FAIL_IN_PROGRESS:
        // don't jump to STATE_FINISHED
        return;

    // all the actual failure codes
    default:
        m_state = STATE_DOWNLOAD_FAILED;
        break;
    }

    ui->stack->setCurrentIndex(STATE_FINISHED);
    updateUi();
}

