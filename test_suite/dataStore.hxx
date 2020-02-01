/*
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

#ifndef _FG_TEST_SUITE_DATA_STORE_HXX
#define _FG_TEST_SUITE_DATA_STORE_HXX

#include <simgear/misc/sg_path.hxx>


// The data store singleton.
class DataStore
{
public:
    // Return the singleton, instantiating it if required.
    static DataStore& get()
    {
        static DataStore instance;
        return instance;
    }

    // Function deletion to allow the class to be a singleton.
    DataStore(DataStore const&) = delete;
    void operator=(DataStore const&) = delete;

    // FGData path functions.
    int findFGRoot(const std::string& fgRootCmdLineOpt, bool debug = false);
    SGPath getFGRoot();
    int validateFGRoot();

private:
    DataStore() = default;

    // The path to FGData.
    SGPath _fgRootPath;
};


#endif // _FG_TEST_SUITE_DATA_STORE_HXX
