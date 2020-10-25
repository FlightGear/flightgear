// Written by James Turner, started October 2020
//
// Copyright (C) 2020 James Turner <james@flightgear.org>
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

#include "config.h"

#include "UpdateChecker.hxx"

#include <QSettings>
#include <QDate>
#include <QDebug>

#include <simgear/io/HTTPMemoryRequest.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/structure/exception.hxx>

#include <Network/HTTPClient.hxx>
#include <Main/globals.hxx>

#include "LauncherNotificationsController.hxx"

namespace {

class UpdateXMLRequest : public simgear::HTTP::MemoryRequest
{
public:
    UpdateXMLRequest(UpdateChecker* u, const std::string& uri) :
        simgear::HTTP::MemoryRequest(uri),
        _owner(u)
    {
        // in case it's useful, send our version as an additional header
        // this would allow a server side script to return different content
        // based on our version in the future
        requestHeader("FlightGear-Version") = FLIGHTGEAR_VERSION;
    }
private:
    void onFail() override
    {
        didFail();
    }

    void onDone() override
    {


        int response = responseCode();
        if (response == 200) {
            // send response to the main thread for processing
            QMetaObject::invokeMethod(_owner, "receivedUpdateXML", Qt::QueuedConnection,
                                      Q_ARG(QByteArray, QByteArray::fromStdString(responseBody())));

        } else {
            didFail();
        }
    }

    void didFail()
    {
        // reset check time to tomorrow
        QSettings settings;
        const QDate n = QDate::currentDate().addDays(1);
        settings.setValue("next-update-check", n);
    }
private:
    UpdateChecker* _owner;
};

} // of namespace

UpdateChecker::UpdateChecker(QObject *parent) : QObject(parent)
{
    QSettings settings;
    QDate nextCheck = settings.value("next-update-check").toDate();
    if (!nextCheck.isValid()) {
        // check tomorrow, so we don't nag immediately after insstallaion
        const QDate n = QDate::currentDate().addDays(1);
        settings.setValue("next-update-check", n);

        return;
    }

    if (nextCheck <= QDate::currentDate()) {
        // start a check
        auto http = globals->get_subsystem<FGHTTPClient>();
        if (!http) {
            return;
        }

        const string_list versionParts = simgear::strutils::split(FLIGHTGEAR_VERSION, ".");
        _majorMinorVersion = versionParts[0] + "." + versionParts[1];

        // definitiely want to ensure HTTPS for this.
        std::string uri = "https://download.flightgear.org/builds/" + _majorMinorVersion + "/updates.xml";
        auto req = new UpdateXMLRequest(this, uri);
        http->makeRequest(req);
    } else {
        // nothing to do
    }
}

void UpdateChecker::ignoreUpdate()
{
    QSettings settings;
    if (m_status == PointUpdate) {
        settings.setValue("ignored-point-release", _currentUpdateVersion);
    } else if (m_status == MajorUpdate) {
        settings.setValue("ignored-major-release", _currentUpdateVersion);
    } else {
        return;
    }

    m_status = NoUpdate;
    m_updateUri.clear();
    _currentUpdateVersion.clear();
    emit statusChanged(m_status);
}

void UpdateChecker::receivedUpdateXML(QByteArray body)
{
    SGPropertyNode_ptr props(new SGPropertyNode);
    const auto s = body.toStdString();

    QSettings settings;
    auto nc = LauncherNotificationsController::instance();

    try {
        const char* buffer = s.c_str();
        readProperties(buffer, s.size(), props, true);

        const QDate n = QDate::currentDate().addDays(7);
        settings.setValue("next-update-check", n);

        const std::string newMajorVersion = props->getStringValue("current-major-release");
        if (simgear::strutils::compare_versions(FLIGHTGEAR_VERSION, newMajorVersion) < 0) {
            // we have a newer version!
            _currentUpdateVersion = QString::fromStdString(newMajorVersion);
            if (settings.value("ignored-major-release") == _currentUpdateVersion) {
                // ignore it
            } else {
                m_status = MajorUpdate;

                const std::string newVersionUri = props->getStringValue("upgrade-uri");
                m_updateUri = QUrl(QString::fromStdString(newVersionUri));
                emit statusChanged(m_status);

                nc->postNotification("flightgear-update-major", QUrl{"qrc:///qml/NewVersionNotification.qml"});

                return; // don't consider minor updates
            }
        }

        // check current version
        const std::string newPointVersion = props->getStringValue("current-point-release");
        if (simgear::strutils::compare_versions(FLIGHTGEAR_VERSION, newPointVersion) < 0) {
            // we have a newer version!
            _currentUpdateVersion = QString::fromStdString(newPointVersion);
            if (settings.value("ignored-point-release") == _currentUpdateVersion) {
                // ignore it
            } else {
                m_status = PointUpdate;
                const std::string newVersionUri = props->getStringValue("download-uri");
                m_updateUri = QUrl(QString::fromStdString(newVersionUri));
                emit statusChanged(m_status);

                nc->postNotification("flightgear-update-point", QUrl{"qrc:///qml/NewVersionNotification.qml"});
            }
        }
    } catch (const sg_exception &e) {
        SG_LOG(SG_IO, SG_WARN, "parsing update XML failed: " << e.getFormattedMessage());
    }
}
