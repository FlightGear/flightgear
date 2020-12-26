# CMake in FlightGear overview 

CMake has evolved considerably in the past decade; if you're reading external tutorials abput it, ensure
they mention 'modern CMake', or the information will be incorrect.

The top-level `CMakeLists.txt` handles configuration options, finding dependencies, and scanning
the rest of the source tree. Across the source tree we add executables, including the main FGFS
binary but also other helpers and utilities. We also define targets for helper libraries, for
various reasons; for example to build some code with different include paths or flags.

Due to the historical code structure, we use some helper functions to collect most of the 
application sources into two global CMake variables, which are then read and added to the
main executable target, in `src/Main/CMakeList.txt`. Therefore, many subdirectories have
a trivial `CMakeLists.txt` which simply calls the helper functions to add its sources:

```
include(FlightGearComponent)

set(SOURCES
	foo.cxx
	bar.cxx
	)

set(HEADERS
	foo.hxx
	bar.hxx
	)
    	
flightgear_component(MyComp "${SOURCES}" "${HEADERS}")
```

The global properties used are `FG_SOURCES` and `FG_HEADERS`.

## Configurations 

Official release builds are built with `RelWithDebInfo`; this is also the most useful configuration for
development, since on Windows, `Debug` is unusably slow. If trying to optimise performance,
keep in mind that compiler flags must be manually set for `RelWithDebInfo`; they are _not_
automatically inherited from `Release`.

Adding additional configurations is possible: for example for profiling. CMake also picks up
the `CXXFLAGS` environment variable to pass ad-hoc compiler options, without modifying the
build systen.

## Dependencies

All dependencies should be handled via an `IMPORTED` target: this ensures that include paths,
link options, etc specific to the dependency are handled correctly across different platforms.

For some dependencies, there may be a zFoo-Config.cmakez which defines such a target for
you automatically. Or there may be an existing `FindFoo.cmake` which does the same. If neither
of these situations exist, create a custom finder file in `CMakeModules`, following the existing
examples. 

CMake tracks transitive dependencies precisely, so for example if your new dependency is used
in SimGear, it will automatically be added to the include / link paths for FlightGear based on
the SimGear build type.

If you encounter a case where a downstream target is missing an include path or flag for a
dependency, it typically indicates a bug in your dependency graph. Do _not_ fix it by maanually
modifying the downstream target's include path or flags. Rather, fix your dependency graph 
and/or `INTERFACE` exports from your dependency, so that CMake can see the required transitive
dependencies correctly.

