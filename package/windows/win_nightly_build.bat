ECHO OFF

IF NOT DEFINED WORKSPACE SET WORKSPACE=%~dp0
ECHO Packaging root is %WORKSPACE%

subst X: /D
subst X: %WORKSPACE%.

REM construct information file to be read by Inno-setup


set PATH=%WORKSPACE%\install\msvc100\OpenSceneGraph\bin;%PATH%

REM add 7-zip to the PATH
set PATH=%PATH%;C:\Program Files\7-zip

REM indirect way to get command output into an environment variable
osgversion --so-number > %TEMP%\osg-so-number.txt
osgversion --version-number > %TEMP%\osg-version.txt
osgversion --openthreads-soversion-number > %TEMP%\openthreads-so-number.txt

SET /P FLIGHTGEAR_VERSION=<flightgear\version
SET /P OSG_VERSION=<%TEMP%\osg-version.txt
SET /P OSG_SO_NUMBER=<%TEMP%\osg-so-number.txt
SET /P OT_SO_NUMBER=<%TEMP%\openthreads-so-number.txt

ECHO #define FGVersion "%FLIGHTGEAR_VERSION%" > InstallConfig.iss
ECHO #define OSGVersion "%OSG_VERSION%" >> InstallConfig.iss
ECHO #define OSGSoNumber "%OSG_SO_NUMBER%" >> InstallConfig.iss
ECHO #define OTSoNumber "%OT_SO_NUMBER%" >> InstallConfig.iss

REM run Inno-setup!
REM use iscc instead of compil32 for better error reporting
iscc FlightGear-nightly.iss
