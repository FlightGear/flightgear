#ifndef CPPUNIT_TOOLS_STRINGHELPER_H
#define CPPUNIT_TOOLS_STRINGHELPER_H

#include <cppunit/Portability.h>
#include <cppunit/portability/Stream.h>
#include <string>
#include <type_traits>


CPPUNIT_NS_BEGIN


/*! \brief Methods for converting values to strings. Replaces CPPUNIT_NS::StringTools::toString
 */
namespace StringHelper
{

// work around to handle C++11 enum class correctly. We need an own conversion to std::string
// as there is no implicit coversion to int for enum class.

template<typename T>
typename std::enable_if<!std::is_enum<T>::value, std::string>::type toString(const T& x)
{
    OStringStream ost;
    ost << x;

    return ost.str();
}

template<typename T>
typename std::enable_if<std::is_enum<T>::value, std::string>::type toString(const T& x)
{
    OStringStream ost;
    ost << static_cast<typename std::underlying_type<T>::type>(x);

    return ost.str();
}

}


CPPUNIT_NS_END

#endif  // CPPUNIT_TOOLS_STRINGHELPER_H

