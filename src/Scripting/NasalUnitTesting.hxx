#pragma once

#include <Scripting/NasalSys.hxx>

class SGPath;

naRef initNasalUnitTestInSim(naRef globals, naContext c);

void shutdownNasalUnitTestInSim();

void executeNasalTestsInDir(const SGPath& path);
bool executeNasalTest(const SGPath& path);
