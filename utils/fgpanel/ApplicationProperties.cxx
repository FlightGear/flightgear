//
//  Written and (c) Torsten Dreyer - Torsten(at)t3r_dot_de
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License as
//  published by the Free Software Foundation; either version 2 of the
//  License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful, but
//  WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifdef _WIN32
# include <direct.h> // for getcwd()
#else // !_WIN32
# include <unistd.h>
#endif

#include "ApplicationProperties.hxx"

double
ApplicationProperties::getDouble (const char *name, const double def) {
  const SGPropertyNode_ptr n (ApplicationProperties::Properties->getNode (name, false));
  if (n == NULL) {
    return def;
  }
  return n->getDoubleValue ();
}

SGPath
ApplicationProperties::GetCwd () {
  SGPath path (".");
  char buf[512];
  char *cwd (getcwd (buf, 511));
  buf[511] = '\0';
  if (cwd) {
    path = SGPath::fromLocal8Bit (cwd);
  }
  return path;
}

SGPath
ApplicationProperties::GetRootPath (const char *sub) {
  if (sub != NULL) {
    const SGPath subpath (sub);

    // relative path to current working dir?
    if (subpath.isRelative ()) {
      SGPath path (GetCwd ());
      path.append (sub);
      if (path.exists ()) {
        return path;
      }
    } else if (subpath.exists ()) {
      // absolute path
      return subpath;
    }
  }

  // default: relative path to FGROOT
  SGPath path (ApplicationProperties::root);
  if (sub != NULL) {
    path.append (sub);
  }
  return path;
}

string ApplicationProperties::root = ".";
SGPropertyNode_ptr ApplicationProperties::Properties = new SGPropertyNode;
