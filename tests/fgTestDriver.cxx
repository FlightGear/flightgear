#include <vector>

#include <simgear/props/condition.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/math/SGMath.hxx>

class TestStep
{
public:
  virtual void run() = 0;
  // name for logging purposes
};

typedef std::vector<TestStep*> TestStepSequence;

class SimulateStep : public TestStep
{
public:
  void run() override
  {

  }

private:
  double m_count;
  double m_dt;

  SGGeod m_endPosition;
  double m_endHeadingTrueDeg;
  // other fake FDM data
};

class CommandStep : public TestStep
{
public:
  void run() override
  {
    // build the command and run it
  }
private:
  SGPropertyNode_ptr m_cmd;
};

class CheckStep : public TestStep
{
public:
  void run() override
  {
    for (auto cond : m_conditions) {
      // eval

      // if failed, boom
    }
  }

private:
  std::vector<SGConditionRef> m_conditions;
};

int main(int argc, char* argv[])
{
  // parse XML

  // find test name

  // wipe working dir

  // create working dir

  // build test stages

  // read and create subsystems

  // execute in turn

  return EXIT_SUCCESS;
}
