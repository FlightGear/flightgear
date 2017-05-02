#include "Atmosphere.hpp"

using namespace yasim;

int main(int argc, char** argv)
{
  Atmosphere a;
  if (a.test()) {
    return 0;
  }
  return 1;
}
