#include "EditCustomMPServerDialog.hxx"
#include "ui_EditCustomMPServerDialog.h"

#include <QSettings>
#include <QComboBox>

#include "Main/fg_props.hxx"

EditCustomMPServerDialog::EditCustomMPServerDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EditCustomMPServerDialog)
{
    ui->setupUi(this);
    QSettings settings;
    ui->mpServer->setText(settings.value("mp-custom-host").toString());
    ui->port->setText(settings.value("mp-custom-port").toString());
}

EditCustomMPServerDialog::~EditCustomMPServerDialog()
{
    delete ui;
}

QString EditCustomMPServerDialog::hostname() const
{
    return ui->mpServer->text();
}

void EditCustomMPServerDialog::accept()
{
    QSettings settings;
    settings.setValue("mp-custom-host", ui->mpServer->text());
    settings.setValue("mp-custom-port", ui->port->text());
    QDialog::accept();
}

void EditCustomMPServerDialog::addCustomItem(QComboBox* combo)
{
    QSettings settings;
    QString customMPHost = settings.value("mp-custom-host").toString();

    if (customMPHost.isEmpty()) {
        combo->addItem(tr("Custom server..."), "custom");
        return;
    }

    combo->addItem(tr("Custom - %1").arg(customMPHost), "custom");
}