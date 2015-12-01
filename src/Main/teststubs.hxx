#ifndef TESTSTUBS_HXX
#define TESTSTUBS_HXX

#include <simgear/structure/exception.hxx>

namespace flightgear
{

namespace test
{

void initHomeAndData(int argc, char* argv[]);

void bindAndInitSubsystems();

void verify(int a, int b);

void verify(const std::string& a, const std::string& b);

void verifyDouble(double a, double b);

void verifyDoubleE(double a, double b, double epsilon);

    void verifyNullPtr(void* p);

class test_failure : public sg_exception
{
public:
    test_failure(const std::string& msg, const std::string& description);
};

} // of namespace test

} // of namespace flightgear

#endif // TESTSTUBS_HXX

