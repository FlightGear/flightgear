// dbuserror.h 
// 
// Copyright (C) 2019 - swift Project Community / Contributors (http://swift-project.org/)
// Adapted to Flightgear by Lars Toenning <dev@ltoenning.de>
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

#ifndef BLACKSIM_FGSWIFTBUS_DBUSERROR_H
#define BLACKSIM_FGSWIFTBUS_DBUSERROR_H

#include <dbus/dbus.h>
#include <string>

namespace FGSwiftBus
{

    //! DBus error
    class CDBusError
    {
    public:
        //! Error type
        enum ErrorType
        {
            NoError,
            Other
        };

        //! Default constructur
        CDBusError() = default;

        //! Constructor
        explicit CDBusError(const DBusError *error);

        //! Get error type
        ErrorType getType() const { return m_errorType; }

    private:
        ErrorType m_errorType = NoError;
        std::string m_name;
        std::string m_message;
    };

}

#endif // guard
