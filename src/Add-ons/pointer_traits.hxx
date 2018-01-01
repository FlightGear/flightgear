// -*- coding: utf-8 -*-
//
// pointer_traits.hxx --- Pointer traits classes
// Copyright (C) 2018  Florent Rougon
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

#ifndef FG_ADDON_POINTER_TRAITS_HXX
#define FG_ADDON_POINTER_TRAITS_HXX

#include <memory>
#include <utility>

#include <simgear/structure/SGSharedPtr.hxx>

namespace flightgear
{

namespace addons
{

template <typename T>
struct shared_ptr_traits;

template <typename T>
struct shared_ptr_traits<SGSharedPtr<T>>
{
  using element_type = T;
  using strong_ref = SGSharedPtr<T>;

  template <typename ...Args>
  static strong_ref makeStrongRef(Args&& ...args)
  {
    return strong_ref(new T(std::forward<Args>(args)...));
  }
};

template <typename T>
struct shared_ptr_traits<std::shared_ptr<T>>
{
  using element_type = T;
  using strong_ref = std::shared_ptr<T>;

  template <typename ...Args>
  static strong_ref makeStrongRef(Args&& ...args)
  {
    return std::make_shared<T>(std::forward<Args>(args)...);
  }
};

} // of namespace addons

} // of namespace flightgear

#endif  // of FG_ADDON_POINTER_TRAITS_HXX
