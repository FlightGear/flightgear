#pragma once

#include <vector>

struct PidControllerInput
{
    double vspeed_fps;
    double reference;
};

struct PidControllerOutput
{
    double output;
};

// Define sequence of input values. These are the values actually used with
// Harrier-GR3 when startup_current=true, but we use the same inputs for
// startup_current=false also.
//
extern std::vector<PidControllerInput> pidControllerInputs;

// Expected output when startup_current is false (the default old behaviour).
//
// Note the large transient at the start which messes up the initial behaviour.
//
extern std::vector<PidControllerOutput> pidControllerOutputs0;

// Expected output when startup_current is true (the new improved behaviour).
//
extern std::vector<PidControllerOutput> pidControllerOutputs1;
