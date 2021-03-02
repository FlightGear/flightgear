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
// Harrier-GR3 when startup_zeros=false, but we use the same inputs for
// startup_zeros=true also.
//
extern std::vector<PidControllerInput> pidControllerInputs;

// Expected output.
//
extern std::vector<PidControllerOutput> pidControllerOutputs;
