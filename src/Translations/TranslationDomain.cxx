// SPDX-FileCopyrightText: (C) 2025  Florent Rougon
// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileComment: Container for all TranslationResource's belonging to a domain

#include <memory>
#include <string>

#include <simgear/debug/logstream.hxx>

#include "TranslationDomain.hxx"

namespace flightgear
{

TranslationDomain::ResourceRef
TranslationDomain::getResourceCreate(const std::string& resourceName)
{
    if (_map.find(resourceName) == _map.end()) {
        _map[resourceName] = std::make_shared<TranslationResource>();
    }

    return _map[resourceName];
}

TranslationDomain::ResourceRef
TranslationDomain::getResource(const std::string& resourceName) const
{
    auto elt = _map.find(resourceName);

    if (elt == _map.end()) {
        SG_LOG(SG_GENERAL, SG_ALERT,
               "TranslationDomain::getResource(): unable to find requested "
               "resource '" << resourceName << "'.");
        return {};
    } else {
        return elt->second;
    }
}

} // namespace flightgear
