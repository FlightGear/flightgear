// CocoaHelpers.h - C++ interface to Cocoa/AppKit helpers

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

#ifndef FG_GUI_COCOA_HELPERS_H
#define FG_GUI_COCOA_HELPERS_H

#include <string>

#include <simgear/misc/sg_path.hxx>
/**
 * open a URL using the system's web-browser
 */
void cocoaOpenUrl(const std::string& url);

/**
 * Cocoa implementation so we can use NSURL
 */
SGPath platformDefaultDataPath();

/**
 * When we run non-bundled, we need to transform to a GUI (foreground) app
 * osgViewer does this for us normally, but we need to do it ourselves
 * to show a message box before OSG is initialized.
 */
void transformToForegroundApp();

/**
 * AppKit shuts us down via exit(), the code in main to cleanup is not run
 * in that scenario. Do some cleanup manually to avoid crashes on exit.
 */
void cocoaRegisterTerminateHandler();

/**
 * @brief helper to detect if we're running translocated or not.
 * Google 'Gatekeep app translation' for more info about this; basically it
 * happens when the user runs us directly from the DMG, and this makes
 * for very nasty file paths.
 */
bool cocoaIsRunningTranslocated();

#endif // of FG_GUI_COCOA_HELPERS_H
