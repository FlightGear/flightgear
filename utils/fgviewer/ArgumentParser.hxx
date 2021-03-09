// ArgumentParser.hxx -- flightgear viewer argument parser
//
// Copyright (C) 2021 by Erik Hofman
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

#pragma once

#include <vector>
#include <map>

#include <osg/ArgumentParser>

class ArgumentParser
{
public:
    ArgumentParser(int argc, char **argv);
    ~ArgumentParser() = default;

    bool read(const char *arg);
    bool read(const char *arg, std::string& value);
    bool read(const char *arg, std::string& name, std::string& value);

    int argc() { return files.size(); }
    std::string& getApplicationName() { return appName; }

    void reportRemainingOptionsAsUnrecognized();
    void writeErrorMessages(std::ostream& output);

    osg::ArgumentParser& osg() { return arguments; }

    operator std::vector<std::string>&() { return files; }

private:
    int arg_num;
    char **arg_values;
    std::string appName;

    std::vector<std::string> errors;
    std::map<std::string, std::string> args;
    std::vector<std::string> files;

    osg::ArgumentParser arguments;
};

