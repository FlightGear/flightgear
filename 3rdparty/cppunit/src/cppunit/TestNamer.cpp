#include <cppunit/extensions/TestNamer.h>
#include <cppunit/extensions/TypeInfoHelper.h>
#include <cppunit/tools/StringHelper.h>
#include <string>

CPPUNIT_NS_BEGIN

TestNamer::TestNamer( const std::type_info &typeInfo )
    : m_fixtureName( TypeInfoHelper::getClassName( typeInfo ) )
{
}

TestNamer::TestNamer( const std::string &fixtureName )
  : m_fixtureName( fixtureName )
{
}

TestNamer::~TestNamer()
{
}

std::string 
TestNamer::getFixtureName() const
{
  return m_fixtureName;
}

std::string 
TestNamer::getTestNameFor( const std::string &testMethodName ) const
{
  return getFixtureName() + "::" + testMethodName;
}

CPPUNIT_NS_END
