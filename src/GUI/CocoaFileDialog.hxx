// CocoaFileDialog.h - Cocoa implementation of file-dialog interface

// Copyright (C) 2013 James Turner <zakalawe@mac.com>
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
//


#ifndef FG_COCOA_FILE_DIALOG_HXX
#define FG_COCOA_FILE_DIALOG_HXX 1

#include <GUI/FileDialog.hxx>

class CocoaFileDialog : public FGFileDialog
{
public:
    CocoaFileDialog(FGFileDialog::Usage use);
    
    virtual ~CocoaFileDialog();
    
    virtual void exec();
    virtual void close();
private:
    class CocoaFileDialogPrivate;
    std::unique_ptr<CocoaFileDialogPrivate> d;
};

#endif // FG_COCOA_FILE_DIALOG_HXX
