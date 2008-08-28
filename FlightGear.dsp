# Microsoft Developer Studio Project File - Name="FlightGear" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=FlightGear - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "FlightGear.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "FlightGear.mak" CFG="FlightGear - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "FlightGear - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "FlightGear - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD CPP /nologo /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /FD /c  /MT /I ".." /I "src" /I "src\include" /I "src\FDM\JSBsim" /I "..\SimGear" /I "..\zlib-1.2.3" /I "..\freeglut-2.4.0\include" /I "..\OpenAL 1.0 Software Development Kit\include" /I "..\Pre-built.2\include" /D "_USE_MATH_DEFINES" /D "_CRT_SECURE_NO_DEPRECATE" /D "HAVE_CONFIG_H" /D "FGFS" /D "FG_NEW_ENVIRONMENT" /D "ENABLE_AUDIO_SUPPORT" /D "ENABLE_PLIB_JOYSTICK"
# SUBTRACT CPP /YX
# ADD RSC /l 0xc09 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BSC32 /nologo
LINK32=link.exe
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib uuid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /FR /FD /GZ /c  /MTd /I ".." /I "src" /I "src\include" /I "src\FDM\JSBsim" /I "..\SimGear" /I "..\zlib-1.2.3" /I "..\freeglut-2.4.0\include" /I "..\OpenAL 1.0 Software Development Kit\include" /I "..\Pre-built.2\include" /D "_USE_MATH_DEFINES" /D "_CRT_SECURE_NO_DEPRECATE" /D "HAVE_CONFIG_H" /D "FGFS" /D "FG_NEW_ENVIRONMENT" /D "ENABLE_AUDIO_SUPPORT" /D "ENABLE_PLIB_JOYSTICK"
# ADD RSC /l 0xc09 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BSC32 /nologo
LINK32=link.exe
# ADD LINK32 kernel32.lib user32.lib winspool.lib comdlg32.lib gdi32.lib shell32.lib wsock32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept 

!ENDIF 

# Begin Target

# Name "FlightGear - Win32 Release"
# Name "FlightGear - Win32 Debug"
# Begin Group "Lib_Aircraft"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\Aircraft\aircraft.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Aircraft"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Aircraft"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Aircraft\aircraft.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Aircraft"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Aircraft"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Aircraft\controls.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Aircraft"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Aircraft"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Aircraft\controls.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Aircraft"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Aircraft"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Aircraft\replay.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Aircraft"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Aircraft"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Aircraft\replay.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Aircraft"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Aircraft"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_Airports"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\Airports\apt_loader.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Airports"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Airports"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Airports\apt_loader.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Airports"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Airports"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Airports\runways.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Airports"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Airports"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Airports\runways.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Airports"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Airports"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Airports\simple.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Airports"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Airports"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Airports\simple.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Airports"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Airports"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Airports\runwayprefs.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Airports"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Airports"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Airports\runwayprefs.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Airports"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Airports"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Airports\parking.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Airports"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Airports"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Airports\parking.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Airports"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Airports"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Airports\gnnode.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Airports"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Airports"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Airports\gnnode.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Airports"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Airports"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Airports\groundnetwork.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Airports"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Airports"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Airports\groundnetwork.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Airports"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Airports"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Airports\dynamics.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Airports"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Airports"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Airports\dynamics.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Airports"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Airports"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Airports\dynamicloader.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Airports"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Airports"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Airports\dynamicloader.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Airports"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Airports"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Airports\runwayprefloader.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Airports"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Airports"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Airports\runwayprefloader.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Airports"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Airports"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Airports\xmlloader.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Airports"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Airports"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Airports\xmlloader.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Airports"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Airports"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_ATC"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\ATC\trafficcontrol.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_ATC"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_ATC"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\ATC\trafficcontrol.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_ATC"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_ATC"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_ATCDCL"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\ATCDCL\ATC.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_ATCDCL"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_ATCDCL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\ATCDCL\ATC.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_ATCDCL"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_ATCDCL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\ATCDCL\atis.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_ATCDCL"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_ATCDCL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\ATCDCL\atis.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_ATCDCL"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_ATCDCL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\ATCDCL\tower.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_ATCDCL"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_ATCDCL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\ATCDCL\tower.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_ATCDCL"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_ATCDCL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\ATCDCL\approach.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_ATCDCL"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_ATCDCL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\ATCDCL\approach.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_ATCDCL"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_ATCDCL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\ATCDCL\ground.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_ATCDCL"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_ATCDCL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\ATCDCL\ground.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_ATCDCL"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_ATCDCL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\ATCDCL\commlist.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_ATCDCL"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_ATCDCL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\ATCDCL\commlist.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_ATCDCL"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_ATCDCL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\ATCDCL\ATCDialog.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_ATCDCL"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_ATCDCL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\ATCDCL\ATCDialog.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_ATCDCL"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_ATCDCL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\ATCDCL\ATCVoice.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_ATCDCL"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_ATCDCL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\ATCDCL\ATCVoice.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_ATCDCL"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_ATCDCL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\ATCDCL\ATCmgr.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_ATCDCL"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_ATCDCL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\ATCDCL\ATCmgr.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_ATCDCL"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_ATCDCL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\ATCDCL\ATCutils.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_ATCDCL"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_ATCDCL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\ATCDCL\ATCutils.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_ATCDCL"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_ATCDCL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\ATCDCL\ATCProjection.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_ATCDCL"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_ATCDCL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\ATCDCL\ATCProjection.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_ATCDCL"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_ATCDCL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\ATCDCL\AIMgr.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_ATCDCL"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_ATCDCL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\ATCDCL\AIMgr.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_ATCDCL"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_ATCDCL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\ATCDCL\AIEntity.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_ATCDCL"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_ATCDCL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\ATCDCL\AIEntity.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_ATCDCL"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_ATCDCL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\ATCDCL\AIPlane.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_ATCDCL"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_ATCDCL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\ATCDCL\AIPlane.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_ATCDCL"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_ATCDCL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\ATCDCL\AILocalTraffic.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_ATCDCL"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_ATCDCL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\ATCDCL\AILocalTraffic.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_ATCDCL"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_ATCDCL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\ATCDCL\AIGAVFRTraffic.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_ATCDCL"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_ATCDCL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\ATCDCL\AIGAVFRTraffic.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_ATCDCL"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_ATCDCL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\ATCDCL\transmission.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_ATCDCL"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_ATCDCL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\ATCDCL\transmission.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_ATCDCL"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_ATCDCL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\ATCDCL\transmissionlist.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_ATCDCL"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_ATCDCL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\ATCDCL\transmissionlist.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_ATCDCL"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_ATCDCL"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_Autopilot"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\Autopilot\route_mgr.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Autopilot"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Autopilot"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Autopilot\route_mgr.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Autopilot"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Autopilot"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Autopilot\xmlauto.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Autopilot"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Autopilot"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Autopilot\xmlauto.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Autopilot"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Autopilot"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_Cockpit"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\Cockpit\cockpit.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Cockpit"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Cockpit"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Cockpit\cockpit.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Cockpit"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Cockpit"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Cockpit\hud.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Cockpit"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Cockpit"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Cockpit\hud.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Cockpit"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Cockpit"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Cockpit\hud_card.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Cockpit"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Cockpit"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Cockpit\hud_dnst.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Cockpit"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Cockpit"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Cockpit\hud_gaug.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Cockpit"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Cockpit"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Cockpit\hud_inst.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Cockpit"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Cockpit"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Cockpit\hud_labl.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Cockpit"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Cockpit"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Cockpit\hud_ladr.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Cockpit"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Cockpit"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Cockpit\hud_rwy.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Cockpit"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Cockpit"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Cockpit\hud_scal.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Cockpit"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Cockpit"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Cockpit\hud_tbi.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Cockpit"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Cockpit"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Cockpit\panel.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Cockpit"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Cockpit"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Cockpit\panel.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Cockpit"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Cockpit"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Cockpit\panel_io.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Cockpit"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Cockpit"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Cockpit\panel_io.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Cockpit"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Cockpit"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_Built_in"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\Cockpit\built_in\FGMagRibbon.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Built_in"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Built_in"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Cockpit\built_in\FGMagRibbon.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Built_in"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Built_in"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_Environment"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\Environment\environment.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Environment"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Environment"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Environment\environment.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Environment"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Environment"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Environment\environment_mgr.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Environment"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Environment"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Environment\environment_mgr.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Environment"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Environment"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Environment\environment_ctrl.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Environment"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Environment"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Environment\environment_ctrl.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Environment"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Environment"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Environment\fgmetar.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Environment"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Environment"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Environment\fgmetar.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Environment"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Environment"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Environment\fgclouds.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Environment"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Environment"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Environment\fgclouds.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Environment"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Environment"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Environment\atmosphere.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Environment"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Environment"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Environment\atmosphere.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Environment"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Environment"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Environment\precipitation_mgr.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Environment"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Environment"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Environment\precipitation_mgr.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Environment"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Environment"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_ExternalNet"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\FDM\ExternalNet\ExternalNet.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_ExternalNet"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_ExternalNet"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\ExternalNet\ExternalNet.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_ExternalNet"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_ExternalNet"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_ExternalPipe"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\FDM\ExternalPipe\ExternalPipe.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_ExternalPipe"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_ExternalPipe"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\ExternalPipe\ExternalPipe.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_ExternalPipe"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_ExternalPipe"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_JSBSim"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\FDM\JSBSim\FGFDMExec.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_JSBSim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_JSBSim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\FGJSBBase.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_JSBSim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_JSBSim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\FGState.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_JSBSim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_JSBSim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\JSBSim.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_JSBSim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_JSBSim"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_Init"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\FDM\JSBSim\initialization\FGInitialCondition.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Init"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Init"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\initialization\FGTrim.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Init"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Init"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\initialization\FGTrimAxis.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Init"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Init"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_InputOutput"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\FDM\JSBSim\input_output\FGGroundCallback.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_InputOutput"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_InputOutput"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\input_output\FGPropertyManager.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_InputOutput"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_InputOutput"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\input_output\FGScript.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_InputOutput"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_InputOutput"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\input_output\FGXMLElement.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_InputOutput"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_InputOutput"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\input_output\FGXMLParse.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_InputOutput"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_InputOutput"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\input_output\FGfdmSocket.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_InputOutput"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_InputOutput"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_Math"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\FDM\JSBSim\math\FGColumnVector3.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Math"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Math"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\math\FGFunction.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Math"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Math"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\math\FGLocation.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Math"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Math"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\math\FGMatrix33.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Math"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Math"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\math\FGPropertyValue.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Math"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Math"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\math\FGQuaternion.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Math"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Math"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\math\FGRealValue.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Math"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Math"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\math\FGTable.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Math"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Math"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\math\FGCondition.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Math"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Math"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_Models"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\FGAerodynamics.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Models"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Models"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\FGAircraft.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Models"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Models"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\FGAtmosphere.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Models"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Models"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\FGAuxiliary.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Models"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Models"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\FGFCS.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Models"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Models"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\FGGroundReactions.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Models"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Models"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\FGInertial.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Models"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Models"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\FGBuoyantForces.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Models"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Models"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\FGExternalForce.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Models"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Models"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\FGLGear.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Models"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Models"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\FGMassBalance.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Models"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Models"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\FGModel.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Models"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Models"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\FGOutput.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Models"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Models"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\FGPropagate.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Models"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Models"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\FGPropulsion.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Models"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Models"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\FGInput.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Models"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Models"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\FGExternalReactions.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Models"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Models"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\FGGasCell.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Models"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Models"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_FlightControl"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\flight_control\FGPID.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_FlightControl"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_FlightControl"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\flight_control\FGDeadBand.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_FlightControl"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_FlightControl"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\flight_control\FGFCSComponent.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_FlightControl"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_FlightControl"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\flight_control\FGFilter.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_FlightControl"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_FlightControl"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\flight_control\FGGain.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_FlightControl"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_FlightControl"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\flight_control\FGGradient.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_FlightControl"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_FlightControl"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\flight_control\FGKinemat.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_FlightControl"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_FlightControl"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\flight_control\FGSummer.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_FlightControl"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_FlightControl"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\flight_control\FGSwitch.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_FlightControl"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_FlightControl"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\flight_control\FGFCSFunction.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_FlightControl"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_FlightControl"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\flight_control\FGSensor.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_FlightControl"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_FlightControl"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\flight_control\FGActuator.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_FlightControl"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_FlightControl"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_Atmosphere"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\atmosphere\FGMSIS.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Atmosphere"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Atmosphere"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\atmosphere\FGMSISData.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Atmosphere"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Atmosphere"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\atmosphere\FGMars.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Atmosphere"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Atmosphere"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_Propulsion"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\propulsion\FGElectric.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Propulsion"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Propulsion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\propulsion\FGEngine.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Propulsion"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Propulsion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\propulsion\FGForce.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Propulsion"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Propulsion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\propulsion\FGNozzle.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Propulsion"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Propulsion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\propulsion\FGPiston.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Propulsion"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Propulsion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\propulsion\FGPropeller.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Propulsion"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Propulsion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\propulsion\FGRocket.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Propulsion"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Propulsion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\propulsion\FGRotor.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Propulsion"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Propulsion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\propulsion\FGTank.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Propulsion"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Propulsion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\propulsion\FGThruster.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Propulsion"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Propulsion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\propulsion\FGTurbine.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Propulsion"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Propulsion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\models\propulsion\FGTurboProp.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Propulsion"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Propulsion"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_LaRCsim"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\LaRCsim.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\LaRCsim.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\LaRCsimIC.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\LaRCsimIC.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\IO360.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\IO360.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\atmos_62.c

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\atmos_62.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\default_model_routines.c

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\default_model_routines.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\ls_accel.c

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\ls_accel.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\ls_aux.c

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\ls_aux.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\ls_cockpit.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\ls_constants.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\ls_generic.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\ls_geodesy.c

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\ls_geodesy.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\ls_gravity.c

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\ls_gravity.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\ls_init.c

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\ls_init.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\ls_matrix.c

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\ls_matrix.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\ls_model.c

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\ls_model.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\ls_sim_control.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\ls_step.c

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\ls_step.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\ls_sym.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\ls_types.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\c172_aero.c

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\c172_engine.c

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\c172_gear.c

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\c172_init.c

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\basic_init.c

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\basic_init.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\basic_aero.c

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\basic_aero.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\basic_engine.c

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\basic_gear.c

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\navion_init.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\navion_aero.c

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\navion_engine.c

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\navion_gear.c

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\navion_init.c

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\uiuc_aero.c

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\cherokee_aero.c

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\cherokee_engine.c

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\cherokee_gear.c

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\cherokee_init.c

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\ls_interface.c

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\ls_interface.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_LaRCsim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_LaRCsim"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_SPFDM"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\FDM\SP\ADA.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_SPFDM"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_SPFDM"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\SP\ADA.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_SPFDM"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_SPFDM"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\SP\ACMS.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_SPFDM"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_SPFDM"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\SP\ACMS.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_SPFDM"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_SPFDM"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\SP\Balloon.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_SPFDM"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_SPFDM"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\SP\Balloon.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_SPFDM"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_SPFDM"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\SP\BalloonSim.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_SPFDM"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_SPFDM"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\SP\BalloonSim.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_SPFDM"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_SPFDM"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\SP\MagicCarpet.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_SPFDM"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_SPFDM"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\SP\MagicCarpet.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_SPFDM"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_SPFDM"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_UIUCModel"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_1DdataFileReader.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_1DdataFileReader.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_1Dinterpolation.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_1Dinterpolation.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_2DdataFileReader.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_2DdataFileReader.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_2Dinterpolation.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_2Dinterpolation.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_3Dinterpolation.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_3Dinterpolation.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_aerodeflections.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_aerodeflections.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_aircraft.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_alh_ap.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_alh_ap.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_auto_pilot.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_auto_pilot.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_betaprobe.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_betaprobe.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_coef_drag.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_coef_drag.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_coef_lift.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_coef_lift.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_coef_pitch.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_coef_pitch.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_coef_roll.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_coef_roll.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_coef_sideforce.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_coef_sideforce.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_coef_yaw.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_coef_yaw.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_coefficients.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_coefficients.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_controlInput.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_controlInput.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_convert.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_convert.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_engine.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_engine.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_flapdata.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_flapdata.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_find_position.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_find_position.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_fog.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_fog.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_gear.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_gear.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_get_flapper.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_get_flapper.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_getwind.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_getwind.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_hh_ap.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_hh_ap.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_ice.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_ice.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_iceboot.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_iceboot.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_iced_nonlin.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_iced_nonlin.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_icing_demo.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_icing_demo.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_initializemaps.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_initializemaps.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_map_CD.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_map_CD.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_map_CL.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_map_CL.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_map_CY.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_map_CY.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_map_Cm.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_map_Cm.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_map_Cn.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_map_Cn.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_map_Croll.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_map_Croll.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_map_controlSurface.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_map_controlSurface.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_map_engine.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_map_engine.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_map_fog.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_map_fog.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_map_geometry.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_map_geometry.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_map_ice.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_map_ice.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_map_gear.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_map_gear.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_map_init.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_map_init.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_map_keyword.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_map_keyword.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_map_mass.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_map_mass.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_map_misc.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_map_misc.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_map_record1.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_map_record1.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_map_record2.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_map_record2.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_map_record3.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_map_record3.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_map_record4.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_map_record4.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_map_record5.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_map_record5.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_map_record6.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_map_record6.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_menu.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_menu.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_menu_init.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_menu_init.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_menu_geometry.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_menu_geometry.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_menu_controlSurface.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_menu_controlSurface.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_menu_mass.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_menu_mass.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_menu_engine.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_menu_engine.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_menu_CD.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_menu_CD.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_menu_CL.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_menu_CL.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_menu_Cm.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_menu_Cm.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_menu_CY.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_menu_CY.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_menu_Croll.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_menu_Croll.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_menu_Cn.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_menu_Cn.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_menu_gear.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_menu_gear.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_menu_ice.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_menu_ice.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_menu_fog.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_menu_fog.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_menu_record.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_menu_record.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_menu_misc.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_menu_misc.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_menu_functions.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_menu_functions.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_pah_ap.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_pah_ap.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_parsefile.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_parsefile.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_rah_ap.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_rah_ap.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_recorder.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_recorder.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_warnings_errors.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_warnings_errors.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_wrapper.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UIUCModel\uiuc_wrapper.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_YASim"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\FDM\YASim\YASim.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\YASim.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\FGGround.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\FGGround.hpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\Airplane.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\Airplane.hpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\Atmosphere.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\Atmosphere.hpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\BodyEnvironment.hpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\ControlMap.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\ControlMap.hpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\FGFDM.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\FGFDM.hpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\Gear.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\Gear.hpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\Glue.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\Glue.hpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\Ground.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\Ground.hpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\Hitch.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\Hitch.hpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\Hook.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\Hook.hpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\Launchbar.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\Launchbar.hpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\Integrator.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\Integrator.hpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\Jet.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\Jet.hpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\Math.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\Math.hpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\Model.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\Model.hpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\PropEngine.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\PropEngine.hpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\Propeller.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\Propeller.hpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\Engine.hpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\PistonEngine.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\PistonEngine.hpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\TurbineEngine.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\TurbineEngine.hpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\RigidBody.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\RigidBody.hpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\Rotor.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\Rotor.hpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\Rotorpart.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\Rotorpart.hpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\SimpleJet.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\SimpleJet.hpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\Surface.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\Surface.hpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\Thruster.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\Thruster.hpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\Vector.hpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\Wing.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\Wing.hpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\Turbulence.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\YASim\Turbulence.hpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_YASim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_YASim"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_Flight"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\FDM\flight.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Flight"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Flight"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\flight.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Flight"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Flight"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\groundcache.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Flight"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Flight"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\groundcache.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Flight"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Flight"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UFO.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Flight"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Flight"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\UFO.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Flight"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Flight"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\NullFDM.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Flight"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Flight"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\NullFDM.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Flight"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Flight"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_GUI"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\GUI\new_gui.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_GUI"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_GUI"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\GUI\new_gui.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_GUI"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_GUI"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\GUI\dialog.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_GUI"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_GUI"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\GUI\dialog.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_GUI"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_GUI"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\GUI\menubar.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_GUI"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_GUI"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\GUI\menubar.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_GUI"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_GUI"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\GUI\gui.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_GUI"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_GUI"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\GUI\gui.h

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_GUI"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_GUI"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\GUI\gui_funcs.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_GUI"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_GUI"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\GUI\fonts.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_GUI"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_GUI"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\GUI\AirportList.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_GUI"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_GUI"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\GUI\AirportList.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_GUI"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_GUI"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\GUI\property_list.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_GUI"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_GUI"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\GUI\property_list.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_GUI"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_GUI"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\GUI\layout.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_GUI"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_GUI"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\GUI\layout-props.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_GUI"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_GUI"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\GUI\layout.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_GUI"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_GUI"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\GUI\SafeTexFont.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_GUI"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_GUI"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\GUI\SafeTexFont.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_GUI"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_GUI"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_Input"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\Input\input.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Input"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Input"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Input\input.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Input"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Input"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_Instrumentation"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\Instrumentation\instrument_mgr.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\instrument_mgr.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\adf.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\adf.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\airspeed_indicator.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\airspeed_indicator.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\altimeter.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\altimeter.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\attitude_indicator.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\attitude_indicator.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\clock.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\clock.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\dme.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\dme.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\gps.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\gps.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\gsdi.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\gsdi.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\gyro.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\gyro.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\heading_indicator.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\heading_indicator.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\heading_indicator_fg.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\heading_indicator_fg.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\heading_indicator_dg.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\heading_indicator_dg.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\kr_87.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\kr_87.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\kt_70.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\kt_70.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\mag_compass.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\mag_compass.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\marker_beacon.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\marker_beacon.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\mrg.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\mrg.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\navradio.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\navradio.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\slip_skid_ball.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\slip_skid_ball.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\transponder.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\transponder.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\turn_indicator.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\turn_indicator.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\vertical_speed_indicator.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\vertical_speed_indicator.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\inst_vertical_speed_indicator.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\inst_vertical_speed_indicator.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\od_gauge.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\od_gauge.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\wxradar.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\wxradar.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\tacan.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\tacan.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\mk_viii.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\mk_viii.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\dclgps.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\dclgps.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\render_area_2d.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\render_area_2d.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\groundradar.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\groundradar.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\agradar.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\agradar.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\rad_alt.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\rad_alt.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Instrumentation"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Instrumentation"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_KLN89"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\Instrumentation\KLN89\kln89.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_KLN89"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_KLN89"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\KLN89\kln89.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_KLN89"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_KLN89"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\KLN89\kln89_page.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_KLN89"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_KLN89"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\KLN89\kln89_page.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_KLN89"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_KLN89"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\KLN89\kln89_page_act.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_KLN89"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_KLN89"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\KLN89\kln89_page_act.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_KLN89"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_KLN89"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\KLN89\kln89_page_apt.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_KLN89"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_KLN89"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\KLN89\kln89_page_apt.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_KLN89"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_KLN89"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\KLN89\kln89_page_cal.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_KLN89"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_KLN89"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\KLN89\kln89_page_cal.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_KLN89"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_KLN89"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\KLN89\kln89_page_dir.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_KLN89"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_KLN89"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\KLN89\kln89_page_dir.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_KLN89"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_KLN89"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\KLN89\kln89_page_fpl.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_KLN89"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_KLN89"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\KLN89\kln89_page_fpl.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_KLN89"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_KLN89"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\KLN89\kln89_page_int.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_KLN89"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_KLN89"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\KLN89\kln89_page_int.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_KLN89"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_KLN89"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\KLN89\kln89_page_nav.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_KLN89"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_KLN89"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\KLN89\kln89_page_nav.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_KLN89"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_KLN89"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\KLN89\kln89_page_ndb.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_KLN89"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_KLN89"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\KLN89\kln89_page_ndb.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_KLN89"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_KLN89"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\KLN89\kln89_page_nrst.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_KLN89"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_KLN89"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\KLN89\kln89_page_nrst.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_KLN89"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_KLN89"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\KLN89\kln89_page_oth.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_KLN89"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_KLN89"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\KLN89\kln89_page_oth.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_KLN89"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_KLN89"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\KLN89\kln89_page_set.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_KLN89"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_KLN89"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\KLN89\kln89_page_set.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_KLN89"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_KLN89"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\KLN89\kln89_page_usr.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_KLN89"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_KLN89"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\KLN89\kln89_page_usr.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_KLN89"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_KLN89"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\KLN89\kln89_page_vor.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_KLN89"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_KLN89"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\KLN89\kln89_page_vor.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_KLN89"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_KLN89"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\KLN89\kln89_symbols.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_KLN89"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_KLN89"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_HUD"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\Instrumentation\HUD\HUD.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_HUD"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_HUD"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\HUD\HUD.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_HUD"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_HUD"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\HUD\HUD_tape.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_HUD"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_HUD"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\HUD\HUD_dial.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_HUD"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_HUD"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\HUD\HUD_gauge.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_HUD"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_HUD"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\HUD\HUD_instrument.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_HUD"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_HUD"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\HUD\HUD_label.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_HUD"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_HUD"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\HUD\HUD_ladder.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_HUD"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_HUD"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\HUD\HUD_misc.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_HUD"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_HUD"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\HUD\HUD_runway.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_HUD"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_HUD"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\HUD\HUD_scale.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_HUD"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_HUD"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Instrumentation\HUD\HUD_tbi.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_HUD"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_HUD"

!ENDIF 

# End Source File
# End Group
# Begin Group "main"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\Main\bootstrap.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\main"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_Main"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\Main\main.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\main.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\renderer.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\renderer.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\fg_commands.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\fg_commands.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\fg_init.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\fg_init.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\fg_io.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\fg_io.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\fg_props.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\fg_props.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\globals.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\globals.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\logger.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\logger.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\options.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\options.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\splash.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\splash.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\util.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\util.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\viewer.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\viewer.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\viewmgr.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\viewmgr.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\CameraGroup.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\CameraGroup.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\FGManipulator.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\FGManipulator.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\ViewPartitionNode.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\ViewPartitionNode.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\WindowSystemAdapter.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\WindowSystemAdapter.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\WindowBuilder.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\WindowBuilder.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\fg_os_osgviewer.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\$(GFX_COMMON)

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Main"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_Model"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\Model\acmodel.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Model"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Model"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Model\acmodel.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Model"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Model"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Model\model_panel.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Model"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Model"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Model\model_panel.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Model"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Model"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Model\modelmgr.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Model"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Model"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Model\modelmgr.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Model"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Model"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Model\panelnode.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Model"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Model"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Model\panelnode.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Model"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Model"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_AIModel"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\AIModel\submodel.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_AIModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_AIModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\AIModel\submodel.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_AIModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_AIModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\AIModel\AIManager.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_AIModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_AIModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\AIModel\AIManager.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_AIModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_AIModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\AIModel\AIBase.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_AIModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_AIModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\AIModel\AIBase.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_AIModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_AIModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\AIModel\AIModelData.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_AIModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_AIModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\AIModel\AIModelData.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_AIModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_AIModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\AIModel\AIAircraft.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_AIModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_AIModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\AIModel\AIAircraft.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_AIModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_AIModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\AIModel\AIMultiplayer.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_AIModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_AIModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\AIModel\AIMultiplayer.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_AIModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_AIModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\AIModel\AIShip.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_AIModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_AIModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\AIModel\AIShip.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_AIModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_AIModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\AIModel\AIBallistic.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_AIModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_AIModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\AIModel\AIBallistic.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_AIModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_AIModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\AIModel\AIStorm.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_AIModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_AIModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\AIModel\AIStorm.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_AIModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_AIModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\AIModel\AIThermal.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_AIModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_AIModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\AIModel\AIThermal.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_AIModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_AIModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\AIModel\AIFlightPlan.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_AIModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_AIModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\AIModel\AIFlightPlan.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_AIModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_AIModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\AIModel\AIFlightPlanCreate.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_AIModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_AIModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\AIModel\AIFlightPlanCreatePushBack.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_AIModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_AIModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\AIModel\AIFlightPlanCreateCruise.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_AIModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_AIModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\AIModel\AICarrier.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_AIModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_AIModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\AIModel\AICarrier.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_AIModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_AIModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\AIModel\AIStatic.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_AIModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_AIModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\AIModel\AIStatic.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_AIModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_AIModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\AIModel\AITanker.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_AIModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_AIModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\AIModel\AITanker.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_AIModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_AIModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\AIModel\AIWingman.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_AIModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_AIModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\AIModel\AIWingman.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_AIModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_AIModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\AIModel\performancedata.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_AIModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_AIModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\AIModel\performancedata.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_AIModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_AIModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\AIModel\performancedb.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_AIModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_AIModel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\AIModel\performancedb.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_AIModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_AIModel"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_MultiPlayer"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\MultiPlayer\multiplaymgr.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_MultiPlayer"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_MultiPlayer"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\MultiPlayer\multiplaymgr.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_MultiPlayer"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_MultiPlayer"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\MultiPlayer\mpmessages.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_MultiPlayer"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_MultiPlayer"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\MultiPlayer\tiny_xdr.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_MultiPlayer"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_MultiPlayer"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\MultiPlayer\tiny_xdr.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_MultiPlayer"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_MultiPlayer"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_Navaids"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\Navaids\navdb.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Navaids"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Navaids"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Navaids\navdb.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Navaids"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Navaids"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Navaids\fix.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Navaids"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Navaids"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Navaids\fixlist.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Navaids"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Navaids"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Navaids\fixlist.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Navaids"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Navaids"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Navaids\awynet.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Navaids"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Navaids"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Navaids\awynet.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Navaids"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Navaids"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Navaids\navrecord.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Navaids"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Navaids"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Navaids\navrecord.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Navaids"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Navaids"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Navaids\navlist.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Navaids"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Navaids"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Navaids\navlist.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Navaids"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Navaids"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_Network"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\Network\protocol.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\protocol.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\ATC-Main.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\ATC-Main.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\ATC-Inputs.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\ATC-Inputs.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\ATC-Outputs.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\ATC-Outputs.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\atlas.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\atlas.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\AV400.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\AV400.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\garmin.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\garmin.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\lfsglass.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\lfsglass.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\lfsglass_data.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\httpd.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\httpd.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\joyclient.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\joyclient.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\jsclient.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\jsclient.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\native.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\native.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\native_ctrls.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\native_ctrls.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\native_fdm.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\native_fdm.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\native_gui.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\native_gui.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\net_ctrls.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\net_fdm.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\net_fdm_mini.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\net_gui.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\nmea.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\nmea.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\opengc.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\opengc.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\opengc_data.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\multiplay.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\multiplay.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\props.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\props.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\pve.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\pve.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\ray.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\ray.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\rul.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\rul.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\generic.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Network\generic.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Network"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Network"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_Scenery"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\Scenery\redout.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Scenery"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Scenery"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Scenery\redout.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Scenery"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Scenery"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Scenery\scenery.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Scenery"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Scenery"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Scenery\scenery.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Scenery"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Scenery"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Scenery\SceneryPager.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Scenery"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Scenery"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Scenery\SceneryPager.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Scenery"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Scenery"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Scenery\tilemgr.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Scenery"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Scenery"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Scenery\tilemgr.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Scenery"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Scenery"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_Scripting"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\Scripting\NasalSys.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Scripting"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Scripting"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Scripting\NasalSys.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Scripting"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Scripting"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Scripting\nasal-props.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Scripting"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Scripting"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_Sound"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\Sound\beacon.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Sound"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Sound"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Sound\beacon.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Sound"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Sound"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Sound\fg_fx.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Sound"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Sound"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Sound\fg_fx.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Sound"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Sound"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Sound\morse.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Sound"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Sound"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Sound\morse.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Sound"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Sound"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Sound\voice.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Sound"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Sound"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Sound\voice.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Sound"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Sound"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_Systems"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\Systems\system_mgr.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Systems"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Systems\system_mgr.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Systems"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Systems\electrical.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Systems"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Systems\electrical.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Systems"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Systems\pitot.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Systems"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Systems\pitot.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Systems"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Systems\static.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Systems"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Systems\static.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Systems"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Systems\vacuum.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Systems"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Systems\vacuum.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Systems"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Systems"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_Time"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\Time\fg_timer.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Time"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Time"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Time\fg_timer.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Time"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Time"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Time\light.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Time"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Time"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Time\light.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Time"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Time"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Time\sunsolver.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Time"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Time"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Time\sunsolver.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Time"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Time"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Time\tmp.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Time"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Time"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Time\tmp.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Time"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Time"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_Traffic"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\Traffic\SchedFlight.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Traffic"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Traffic"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Traffic\SchedFlight.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Traffic"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Traffic"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Traffic\Schedule.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Traffic"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Traffic"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Traffic\Schedule.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Traffic"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Traffic"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Traffic\TrafficMgr.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Traffic"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Traffic"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Traffic\TrafficMgr.hxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Traffic"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Traffic"

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE = .\src\Include\config.h-msvc6

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# Begin Custom Build - Creating config.h
InputPath=.\src\Include\config.h-msvc6

".\src\Include\config.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy .\src\Include\config.h-msvc6 .\src\Include\config.h

# End Custom Build

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# Begin Custom Build - Creating config.h
InputPath=.\src\Include\config.h-msvc6

".\src\Include\config.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy .\src\Include\config.h-msvc6 .\src\Include\config.h

# End Custom Build

!ENDIF

# End Source File
# End Target
# End Project
