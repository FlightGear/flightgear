#ifndef CPPUNIT_EXTENSIONS_TESTNAMER_H
#define CPPUNIT_EXTENSIONS_TESTNAMER_H

#include <cppunit/Portability.h>
#include <string>
#include <cppunit/tools/StringHelper.h>

#include <typeinfo>



/*! \def CPPUNIT_TESTNAMER_DECL( variableName, FixtureType )
 * \brief Declares a TestNamer.
 *
 * Declares a TestNamer for the specified type
 *
 * \code
 * void someMethod() 
 * {
 *   CPPUNIT_TESTNAMER_DECL( namer, AFixtureType );
 *   std::string fixtureName = namer.getFixtureName();
 *   ...
 * \endcode
 *
 * \relates TestNamer
 * \see TestNamer
 */
#  define CPPUNIT_TESTNAMER_DECL( variableName, FixtureType )       \
              CPPUNIT_NS::TestNamer variableName( typeid(FixtureType) )

CPPUNIT_NS_BEGIN

/*! \brief Names a test or a fixture suite.
 *
 * TestNamer is usually instantiated using CPPUNIT_TESTNAMER_DECL.
 *
 */
class CPPUNIT_API TestNamer
{
public:
  /*! \brief Constructs a namer using the fixture's type-info.
   * \param typeInfo Type-info of the fixture type. Use to name the fixture suite.
   */
  TestNamer( const std::type_info &typeInfo );

  /*! \brief Constructs a namer using the specified fixture name.
   * \param fixtureName Name of the fixture suite. Usually extracted using a macro.
   */
  TestNamer( const std::string &fixtureName );

  virtual ~TestNamer();

  /*! \brief Returns the name of the fixture.
   * \return Name of the fixture.
   */
  virtual std::string getFixtureName() const;

  /*! \brief Returns the name of the test for the specified method.
   * \param testMethodName Name of the method that implements a test.
   * \return A string that is the concatenation of the test fixture name 
   *         (returned by getFixtureName()) and\a testMethodName, 
   *         separated using '::'. This provides a fairly unique name for a given
   *         test.
   */
  virtual std::string getTestNameFor( const std::string &testMethodName ) const;

  template<typename E>
  std::string getTestNameFor( const std::string& testMethodName, const E& val) const
  {
      return getTestNameFor(testMethodName) + " with parameter: " + CPPUNIT_NS::StringHelper::toString(val);
  }

protected:
  std::string m_fixtureName;
};

CPPUNIT_NS_END

#endif // CPPUNIT_EXTENSIONS_TESTNAMER_H

