#include "simgear/simgear_config.h"

int main(void) {
  #ifdef ENABLE_SIMD
  return 1;
  #else
  return 0;
  #endif
}
