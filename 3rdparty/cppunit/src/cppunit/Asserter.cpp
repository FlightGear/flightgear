#include <cppunit/Asserter.h>
#include <cppunit/Exception.h>
#include <cppunit/Message.h>


CPPUNIT_NS_BEGIN


// coverity[+kill]
void 
Asserter::fail( std::string message, 
                const SourceLine &sourceLine )
{
  fail( Message( "assertion failed", message ), sourceLine );
}

// coverity[+kill]
void 
Asserter::fail( const Message &message, 
                const SourceLine &sourceLine )
{
  throw Exception( message, sourceLine );
}


void 
Asserter::failIf( bool shouldFail, 
                  const Message &message, 
                  const SourceLine &sourceLine )
{
  if ( shouldFail )
    fail( message, sourceLine );
}


void 
Asserter::failIf( bool shouldFail, 
                  std::string message, 
                  const SourceLine &sourceLine )
{
  failIf( shouldFail, Message( "assertion failed", message ), sourceLine );
}

std::string 
Asserter::makeExpected( const std::string &expectedValue )
{
  return "Expected: " + expectedValue;
}

std::string 
Asserter::makeExpectedEqual( const std::string &expectedValue )
{
  return "Expected: " + expectedValue;
}

std::string
Asserter::makeExpectedLess( const std::string& expectedValue )
{
    return "Expected less than: " + expectedValue;
}

std::string
Asserter::makeExpectedLessEqual( const std::string& expectedValue )
{
    return "Expected less or equal than: " + expectedValue;
}
    
std::string
Asserter::makeExpectedGreater( const std::string& expectedValue )
{
    return "Expected greater than: " + expectedValue;
}
    
std::string
Asserter::makeExpectedGreaterEqual( const std::string& expectedValue )
{
    return "Expected greater or equal than: " + expectedValue;
}

std::string 
Asserter::makeActual( const std::string &actualValue )
{
  return "Actual  : " + actualValue;
}


Message
Asserter::makeMessage( const std::string& expectedMessage,
                       const std::string& actualMessage,
                       const std::string& shortDescription,
                       const AdditionalMessage& additionalMessage)
{
  Message message( shortDescription,
                   expectedMessage,
                   actualMessage );
  message.addDetail( additionalMessage );

  return message;   
}


Message 
Asserter::makeNotEqualMessage( const std::string &expectedValue,
                               const std::string &actualValue,
                               const AdditionalMessage &additionalMessage,
                               const std::string &shortDescription )
{
  return makeMessage(makeExpectedEqual(expectedValue), makeActual(actualValue), shortDescription, additionalMessage);
}


void 
Asserter::failNotEqual( std::string expected, 
                        std::string actual, 
                        const SourceLine &sourceLine,
                        const AdditionalMessage &additionalMessage,
                        std::string shortDescription )
{
  fail( makeMessage( makeExpectedEqual(expected),
                     makeActual(actual),
                     shortDescription, 
                     additionalMessage ),
        sourceLine );
}


void 
Asserter::failNotLess( std::string expected, 
                        std::string actual, 
                        const SourceLine &sourceLine,
                        const AdditionalMessage &additionalMessage,
                        std::string shortDescription )
{
  fail( makeMessage( makeExpectedLess(expected),
                     makeActual(actual),
                     shortDescription,
                     additionalMessage),
        sourceLine );
}


void 
Asserter::failNotGreater( std::string expected, 
                        std::string actual, 
                        const SourceLine &sourceLine,
                        const AdditionalMessage &additionalMessage,
                        std::string shortDescription )
{
  fail( makeMessage( makeExpectedGreater(expected),
                     makeActual(actual),
                     shortDescription,
                     additionalMessage),
        sourceLine );
}

void 
Asserter::failNotLessEqual( std::string expected, 
                            std::string actual, 
                            const SourceLine &sourceLine,
                            const AdditionalMessage &additionalMessage,
                            std::string shortDescription )
{
  fail( makeMessage( makeExpectedLessEqual(expected),
                     makeActual(actual),
                     shortDescription,
                     additionalMessage ), 
        sourceLine );
}

void 
Asserter::failNotGreaterEqual( std::string expected, 
                            std::string actual, 
                            const SourceLine &sourceLine,
                            const AdditionalMessage &additionalMessage,
                            std::string shortDescription )
{
  fail( makeMessage( makeExpectedGreaterEqual(expected),
                     makeActual(actual),
                     shortDescription,
                     additionalMessage ), 
        sourceLine );
}
void 
Asserter::failNotEqualIf( bool shouldFail,
                          std::string expected, 
                          std::string actual, 
                          const SourceLine &sourceLine,
                          const AdditionalMessage &additionalMessage,
                          std::string shortDescription )
{
  if ( shouldFail )
    failNotEqual( expected, actual, sourceLine, additionalMessage, shortDescription );
}


CPPUNIT_NS_END
