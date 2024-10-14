/*
 * SPDX-FileCopyrightText: (C) 2019-2022 swift Project Community / Contributors (https://swift-project.org/)
 * SPDX-FileCopyrightText: (C) 2019-2022 Lars Toenning <dev@ltoenning.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "dbus/dbus.h"
#include <string>
#include <vector>

namespace flightgear::swift {

//! DBus Message
class CDBusMessage
{
public:
    //! Constructor
    //! @{
    CDBusMessage(DBusMessage* message);
    CDBusMessage(const CDBusMessage& other);
    //! @}

    //! Destructor
    ~CDBusMessage();

    //! Assignment operator
    CDBusMessage& operator=(CDBusMessage other);

    //! Is this message a method call?
    bool isMethodCall() const;

    //! Does this message want a reply?
    bool wantsReply() const;

    //! Get the message sender
    std::string getSender() const;

    //! Get the message serial. This is usally required for reply message.
    dbus_uint32_t getSerial() const;

    //! Get the called interface name
    std::string getInterfaceName() const;

    //! Get the called object path
    std::string getObjectPath() const;

    //! Get the called method name
    std::string getMethodName() const;

    //! Begin writing argument
    void beginArgumentWrite();

    //! Append argument. Make sure to call \sa beginArgumentWrite() before.
    //! @{
    void appendArgument(bool value);
    void appendArgument(const char* value);
    void appendArgument(const std::string& value);
    void appendArgument(int value);
    void appendArgument(double value);
    void appendArgument(const std::vector<double>& array);
    void appendArgument(const std::vector<std::string>& array);
    //! @}

    //! Begin reading arguments
    void beginArgumentRead();

    //! Read single argument. Make sure to call \sa beginArgumentRead() before.
    //! @{
    void getArgument(int& value);
    void getArgument(bool& value);
    void getArgument(double& value);
    void getArgument(std::string& value);
    void getArgument(std::vector<int>& value);
    void getArgument(std::vector<bool>& value);
    void getArgument(std::vector<double>& value);
    void getArgument(std::vector<std::string>& value);
    //! @}

    //! Creates a DBus message containing a DBus signal
    static CDBusMessage createSignal(const std::string& path, const std::string& interfaceName, const std::string& signalName);

    //! Creates a DBus message containing a DBus reply
    static CDBusMessage createReply(const std::string& destination, dbus_uint32_t serial);

private:
    friend class CDBusConnection;

    DBusMessage* m_message = nullptr;
    DBusMessageIter m_messageIterator;
    CDBusMessage(DBusMessage* message, dbus_uint32_t serial);
    dbus_uint32_t m_serial = 0;
};

} // namespace flightgear::swift
