Welcome to the FlightGear Flight Simulator project.
===================================================
![](https://www.flightgear.org/wp-content/uploads/2017/03/cropped-0egncJ0.png)
FlightGear is a free and open-source flight simulator.

🌍The primary web page for this project is: http://www.flightgear.org

**Sections:**
* [The "base" pack](https://github.com/matteo-andreuzza/flightgear/redme.md#the-base-pack)
* [Installation](https://github.com/matteo-andreuzza/flightgear/redme.md#installation)
* [Install aircraft](https://github.com/matteo-andreuzza/flightgear/redme.md#Install-aircraft)
* [FlightGear World Scenery — v1.0.1 and v2.0.1](https://github.com/matteo-andreuzza/flightgear/redme.md#FlightGear-World-Scenery-—-v1.0.1-and-v2.0.1)
* [Thanks](https://github.com/matteo-andreuzza/flightgear/redme.md#Thanks)
## The "base" pack
When you first run FlightGear, you will find a scenario and aircraft already installed. 
You will be able to download other scenarios 🗺️ to fly to other parts of the world, 
and you can download other planes 🛩️ using the official FlightGear hangar or by downloading them from [here](https://mirrors.ibiblio.org/flightgear/ftp/Aircraft/)

📄For additional install help for specific platforms please browse the
"docs-mini/" subdirectory.

More complete documentation is available from our web page as a
separate distribution.

## Installation

For basic installation instructions see the "INSTALL" file.

FlightGear is available for windows, mac and linux systems. To download the latest version for your OS, visit [the download page](https://www.flightgear.org/download), 
or download the installation file from [sourceforge](https://sourceforge.net/projects/flightgear/).

FlightGear uses the [CMake build system](https://cmake.org/) to generate a platform-specific
build environment.  CMake reads the CMakeLists.txt files that you'll
find throughout the source directories, checks for installed
dependencies and then generates the appropriate build system.
If you don't already have CMake installed on your system you can grab
it from http://www.cmake.org, use version 2.6.4 or later.

Under unices (i.e. Linux, Solaris, Free-BSD, HP-Ux, OSX) use the cmake
or ccmake command-line utils. Preferably, create an out-of-source
build directory and run cmake or ccmake from there. The advantage to
this approach is that the temporary files created by CMake won't
clutter the source directory, and also makes it possible to have
multiple independent build targets by creating multiple build
directories. In a directory alongside the FlightGear source directory
use:

```bash
    mkdir fgbuild
    cd fgbuild
    cmake ../flightgear -DCMAKE_BUILD_TYPE=Release
    make
    sudo make install
```

For further information see:
* http://wiki.flightgear.org/Building_Flightgear

For detailed instructions see:
* [README.cmake](https://github.com/FlightGear/flightgear/blob/next/README.cmake) for Linux, OSX...
* [README.msvc](https://github.com/FlightGear/flightgear/blob/next/README.msvc) for Windows / Visual Studio

## Install aircraft
There are several methods for installing aircraft in FlightGear.

* The first method is to use hangars for FlightGear. If you installed FlightGear using the installer, chances are you are already connected to the official hangar. To check if you are already connected to the official FlightGear hangar, go to the "add-on" section in the sidebar and look for a hangar in the "Aircraft Hangar" section. If there is no hangar connected, click on the writing to add a hangar and follow the procedure.

* The second method is to install the planes using online mirrors. To install aircraft from mirrors you just need to go to ahaha and download the aircraft file you want to install. Once downloaded, you will need to extract it and move the extracted folder to a folder. To have FlightGear recognize the aircraft, go to the "Add-ons" section of the FlightGear sidebar. Click "add +" in the "Additional airline folders" section. Add the folder you chose earlier. From now on, all aircraft in this folder will be recognized by FlightGear. Restart FlightGear and the aircraft you installed will appear in the "planes" section.

* The third method is to install aircraft from other sources. You can install aircraft developed by other developers who make their aircraft available. For example you can install planes from GitHub. 

## FlightGear World Scenery — v1.0.1 and v2.0.1
FlightGear scenery is available for the entire world.  There are two versions available:

* v1.0.1 is an older world scenery build. You can think of it as “SD” or standard detail level.  The entire world consumes about 12Gb compressed and fits onto a 3-DVD set.  In some places this scenery is referred to as version 2.12 which matches the version of FlightGear that was current at the time this world scenery set was generated.  Some people prefer this scenery version because it is simpler and allows FlightGear to run at faster frame rates on less capable computer hardware.
* v2.0.1 is a newer world scenery build based on updated and more detailed source information.  Think of it as “HD” or high detail.  It has wonderful details through the European region, and pretty good detail through the USA.  It consumes about 86Gb compressed and fits on a 4-bluray + 1-dvd.
Getting the World Scenery for FlightGear

* v1.0.1 “SD” – Click on a file to download the corresponding 10×10 degree scenery tile.
* v2.0.1 via Terrasync – TerraSync is an in-app service that will download the local scenery ahead of where you are flying.  This is a great way to keep current, but requires an active internet connection, and may trigger quite a bit of bandwidth usage as you fly.
From the FlightGear store – If your bandwidth is limited or expensive, you can avoid huge downloads and purchase the complete FlightGear distribution including both Windows and MacOSX installers, the full (and most recent) world scenery set, the entire collection of aircraft, and source code.
When downloading and installing scenery, the files are named according to the coordinates of their lower, left hand corner.

#### TerraSync
There is a utility available (now built into FlightGear) called “TerraSync”. This utility runs in the background in a separate process, monitors your position, and downloads (or updates) the latest greatest scenery from the master scenery server “just in time”.  The wiki has more information on launching and running TerraSync.

More in our wiki: https://wiki.flightgear.org/Howto:Install_scenery

## Thanks
Please take a look at the "Thanks" file for a list of people who have
contributed to this project.

For a summary of changes/additions by version see the [NEWS](https://github.com/FlightGear/flightgear/blob/next/NEWS) file.
This project is GPL'd.  For complete details on our licensing please
see the "COPYING" file.

For information on available mailing lists, mailing list archives, and
other available source code and documenation, please visit our web
site.

FlightGear is a product of the collaboration of large international
group of volunteers.  FlightGear is a work in progress.  FlightGear
comes with no warrantee.  We hope you enjoy FlightGear and/or find it
of some value!
