// QtFileDialog.cxx - Qt5 implementation of FGFileDialog
//
// Written by Rebecca Palmer, started February 2016.
//
// Copyright (C) 2015 Rebecca Palmer <rebecca_palmer@zoho.com>
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

#include "QtFileDialog.hxx"
#include "QtLauncher.hxx"
#include <simgear/debug/logstream.hxx>

// Qt
#include <QFileDialog>
#include <QDir>
#include <QString>
#include <QStringList>

QtFileDialog::QtFileDialog(FGFileDialog::Usage use) :
    FGFileDialog(use)
{

}

QtFileDialog::~QtFileDialog() {}

void QtFileDialog::exec()
{
    // concatenate filter patterns, as Qt uses a single string
    std::string filter="";
    for( string_list::const_iterator it = _filterPatterns.begin(); it != _filterPatterns.end();++it ) {
        if(!filter.empty()){
            filter=filter+" ";
        }
        filter=filter+*it;
    }
    QFileDialog dlg(0,QString::fromStdString(_title),QString::fromStdString(_initialPath.utf8Str()),QString::fromStdString(filter));
    if (_usage==USE_SAVE_FILE) {
        dlg.setAcceptMode(QFileDialog::AcceptSave);
    }
    if (_usage==USE_CHOOSE_DIR) {
        dlg.setFileMode(QFileDialog::Directory);
    }
    if (_usage==USE_OPEN_FILE) {
        dlg.setFileMode(QFileDialog::ExistingFile);
    }
    dlg.setLabelText(QFileDialog::Accept,QString::fromStdString(_buttonText));
    dlg.selectFile(QString::fromStdString(_placeholder));
    if(_showHidden){
        dlg.setFilter(dlg.filter() | QDir::Hidden);
    }
    if(dlg.exec()){
        QStringList result = dlg.selectedFiles();
        if(!(result.isEmpty())){
            _callback->onFileDialogDone(this, SGPath(result[0].toStdString()));
        }
    }
}

void QtFileDialog::close(){}

