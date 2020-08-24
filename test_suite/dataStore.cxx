/*
 * Copyright (C) 2018 James Turner
 * Copyright (C) 2018 Edward d'Auvergne
 *
 * This file is part of the program FlightGear.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <iostream>

#include <simgear/misc/strutils.hxx>

#include <Main/fg_init.hxx>


#include "dataStore.hxx"


// Sanity check.
bool looksLikeFGData(const SGPath& path)
{
    return (path / "defaults.xml").exists();
}


// Set up the path to FGData.
int DataStore::findFGRoot(const std::string& fgRootCmdLineOpt, bool debug)
{
    SGPath fgRoot;

    // Command line supplied path.
    if (!fgRootCmdLineOpt.empty()) {
        fgRoot = SGPath::fromUtf8(fgRootCmdLineOpt);
        if (looksLikeFGData(fgRoot)) {
            if (debug)
                std::cerr << "FGdata from the command line: " << fgRoot << std::endl;
            _fgRootPath = fgRoot;
            return 0;

        // Fail if an invalid path is supplied.
        } else {
            std::cerr << "The supplied path \"" << fgRootCmdLineOpt << "\" is not a valid FGData path." << std::endl;
            return 1;
        }
    }

    // The FG_DATA_DIR CMake option.
    fgRoot = SGPath::fromUtf8(PKGLIBDIR);
    if (looksLikeFGData(fgRoot)) {
        if (debug)
            std::cerr << "FGdata found via $PKGLIBDIR: " << fgRoot << std::endl;
        _fgRootPath = fgRoot;
        return 0;
    }

    // The FG_ROOT environmental variable.
    if (std::getenv("FG_ROOT")) {
        fgRoot = SGPath::fromEnv("FG_ROOT");
        if (looksLikeFGData(fgRoot)) {
            if (debug)
                std::cerr << "FGdata found via $FG_ROOT: " << fgRoot << std::endl;
            _fgRootPath = fgRoot;
            return 0;
        }
    }

    // Try in ../fgdata.
    fgRoot = SGPath::fromUtf8(FGSRCDIR) / ".." / "fgdata";
    if (looksLikeFGData(fgRoot)) {
        if (debug)
            std::cerr << "FGdata found at \"$FGSRCDIR/../fgdata\": " << fgRoot << std::endl;
        _fgRootPath = fgRoot;
        return 0;
    }

    // Try in ../data.
    fgRoot = SGPath::fromUtf8(FGSRCDIR) / ".." / "data";
    if (looksLikeFGData(fgRoot)) {
        if (debug)
            std::cerr << "FGdata found at \"$FGSRCDIR/../data\": " << std::endl;
        _fgRootPath = fgRoot;
        return 0;
    }

    // No FGData.
    std::cerr << "FGData not found." << std::endl;
    return 1;
}

// Get the path to FGData.
SGPath DataStore::getFGRoot()
{
    return _fgRootPath;
}

// Perform a version validation of FGData, returning zero if ok.
int DataStore::validateFGRoot()
{
    // The FGData version.
    SGPath root = getFGRoot();
    std::string base_version = fgBasePackageVersion(root);
    if (base_version.empty()) {
        std::cout << "Base package not found." << std::endl;
        std::cout << "Required FGData files not found, please check your configuration." << std::endl;
        std::cout << "Looking for FGData files in '" << root.str() << "'." << std::endl;
        std::cout.flush();
        return 1;
    }

    // Comparison of only the major and minor version numbers.
    const int versionComp = simgear::strutils::compare_versions(FLIGHTGEAR_VERSION, base_version, 2);
    if (versionComp != 0) {
        std::cout << "Base package version mismatch." << std::endl;
        std::cout << "Version check failed, please check your configuration." << std::endl;
        std::cout << "The FGData version '" << base_version << "' in '" << root.str();
        std::cout << "does not match the FlightGear version '" << std::string(FLIGHTGEAR_VERSION);
        std::cout << "'." << std::endl;
        std::cout.flush();
    }
    return versionComp;
}
