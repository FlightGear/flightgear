// QtMessageBox.cxx - Qt5 implementation of MessageBox
//
// Written by Rebecca Palmer, started November 2015.
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

#include "MessageBox.hxx"
#include "QtLauncher.hxx"

// Qt
#include <QMessageBox>
#include <QString>

flightgear::MessageBoxResult
QtMessageBox(const std::string& caption,
                    const std::string& msg,
                    const std::string& moreText,
                    bool fatal)
{
    int fakeargc = 1;
    static char fakeargv0[] = "fgfs";
    static char * fakeargv[2] = {fakeargv0, 0};
    // This does nothing if it has already been run, so the fake argc/argv are
    // only used if an error box is triggered in early startup. Don't attempt
    // to initialize the QSettings, because this would require FGGlobals to be
    // initialized (for globals->get_fg_home()), which would prevent using
    // this function at early startup.
    flightgear::initApp(fakeargc, fakeargv, false /* doInitQSettings */);
    QMessageBox msgBox;
    msgBox.setWindowTitle(QString::fromStdString(caption));
    msgBox.setText(QString::fromStdString(msg));
    msgBox.setInformativeText(QString::fromStdString(moreText));
    if (fatal) {
        msgBox.setIcon(QMessageBox::Critical);
    } else {
        msgBox.setIcon(QMessageBox::Warning);
    }
    msgBox.exec();
    return flightgear::MSG_BOX_OK;
}
