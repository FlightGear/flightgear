// SPDX-FileCopyrightText: (C) 2025  Florent Rougon
// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileComment: Container for all TranslationResource's belonging to a domain

#pragma once

#include <map>
#include <memory>
#include <string>

#include "TranslationResource.hxx"

namespace flightgear
{

class TranslationDomain
{
public:
    using ResourceRef = std::shared_ptr<TranslationResource>;
    /**
     * Get the specified TranslationResource instance.
     *
     * Create, insert and return an empty one if there is no such resource yet.
     */
    ResourceRef getResourceCreate(const std::string& resourceName);
    /**
     * Get the specified TranslationResource instance.
     *
     * Return an empty shared pointer if there is no such resource yet.
     */
    ResourceRef getResource(const std::string& resourceName) const;

private:
    std::map<std::string, ResourceRef> _map;
};

} // namespace flightgear
