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

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I ".\src" /I ".\src\Include" /I "\usr\include" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "HAVE_CONFIG_H" /D "FGFS" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0xc09 /d "NDEBUG"
# ADD RSC /l 0xc09 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib glut32.lib wsock32.lib simgear.lib fnt.lib pui.lib sg.lib sl.lib ssg.lib ul.lib mk4vc60_d.lib /nologo /subsystem:console /machine:I386 /libpath:"\usr\lib" /libpath:"\usr\lib\simgear" /libpath:"\usr\lib\plib

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I ".\src" /I ".\src\Include" /I "\usr\include" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "HAVE_CONFIG_H" /D "FGFS" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0xc09 /d "_DEBUG"
# ADD RSC /l 0xc09 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib winspool.lib comdlg32.lib gdi32.lib shell32.lib glut32.lib wsock32.lib simgear.lib fnt.lib pui.lib sg.lib sl.lib ssg.lib ul.lib mk4vc60_d.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept /libpath:"\usr\lib" /libpath:"\usr\lib\simgear" /libpath:"\usr\lib\plib"

!ENDIF 

# Begin Target

# Name "FlightGear - Win32 Release"
# Name "FlightGear - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
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
# End Group
# Begin Group "Lib_Airports"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\Airports\runways.cxx

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
# End Group
# Begin Group "Lib_Autopilot"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\Autopilot\auto_gui.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Autopilot"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Autopilot"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Autopilot\newauto.cxx

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

SOURCE=.\src\Cockpit\hud.cxx

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

SOURCE=.\src\Cockpit\hud_guag.cxx

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

SOURCE=.\src\Cockpit\hud_lat.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Cockpit"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Cockpit"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Cockpit\hud_lon.cxx

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

SOURCE=.\src\Cockpit\panel_io.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Cockpit"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Cockpit"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Cockpit\radiostack.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Cockpit"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Cockpit"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Cockpit\steam.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Cockpit"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Cockpit"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_Controls"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\Controls\controls.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Controls"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Controls"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_Balloon"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\FDM\Balloon\BalloonSim.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Balloon"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Balloon"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_JSBSim"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\FDM\JSBSim\FGAircraft.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_JSBSim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_JSBSim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\FGAtmosphere.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_JSBSim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_JSBSim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\FGAuxiliary.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_JSBSim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_JSBSim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\FGCoefficient.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_JSBSim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_JSBSim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\FGConfigFile.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_JSBSim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_JSBSim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\FGControls.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_JSBSim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_JSBSim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\FGFCS.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_JSBSim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_JSBSim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\FGFDMExec.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_JSBSim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_JSBSim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\FGInitialCondition.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_JSBSim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_JSBSim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\FGLGear.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_JSBSim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_JSBSim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\FGMatrix.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_JSBSim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_JSBSim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\FGModel.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_JSBSim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_JSBSim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\FGOutput.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_JSBSim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_JSBSim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\FGPosition.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_JSBSim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_JSBSim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\FGRotation.cpp

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

SOURCE=.\src\FDM\JSBSim\FGTranslation.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_JSBSim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_JSBSim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\FGTrim.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_JSBSim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_JSBSim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\FGTrimAxis.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_JSBSim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_JSBSim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\FGUtility.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_JSBSim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_JSBSim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\FGEngine.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_JSBSim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_JSBSim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\FGTank.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_JSBSim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_JSBSim"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\FGfdmSocket.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_JSBSim"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_JSBSim"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_filtersjb"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\FDM\JSBSim\filtersjb\FGDeadBand.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_filtersjb"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_filtersjb"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\filtersjb\FGFCSComponent.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_filtersjb"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_filtersjb"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\filtersjb\FGFilter.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_filtersjb"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_filtersjb"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\filtersjb\FGFlaps.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_filtersjb"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_filtersjb"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\filtersjb\FGGain.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_filtersjb"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_filtersjb"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\filtersjb\FGGradient.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_filtersjb"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_filtersjb"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\filtersjb\FGSummer.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_filtersjb"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_filtersjb"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim\filtersjb\FGSwitch.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_filtersjb"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_filtersjb"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_LaRCsim"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\FDM\LaRCsim\atmos_62.c

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

SOURCE=.\src\FDM\LaRCsim\ls_accel.c

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

SOURCE=.\src\FDM\LaRCsim\ls_geodesy.c

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

SOURCE=.\src\FDM\LaRCsim\ls_init.c

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

SOURCE=.\src\FDM\LaRCsim\ls_model.c

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

SOURCE=.\src\FDM\UIUCModel\uiuc_1Dinterpolation.cpp

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

SOURCE=.\src\FDM\UIUCModel\uiuc_2Dinterpolation.cpp

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

SOURCE=.\src\FDM\UIUCModel\uiuc_betaprobe.cpp

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

SOURCE=.\src\FDM\UIUCModel\uiuc_coef_lift.cpp

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

SOURCE=.\src\FDM\UIUCModel\uiuc_coef_roll.cpp

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

SOURCE=.\src\FDM\UIUCModel\uiuc_coef_yaw.cpp

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

SOURCE=.\src\FDM\UIUCModel\uiuc_controlInput.cpp

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

SOURCE=.\src\FDM\UIUCModel\uiuc_engine.cpp

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

SOURCE=.\src\FDM\UIUCModel\uiuc_initializemaps.cpp

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

SOURCE=.\src\FDM\UIUCModel\uiuc_map_CL.cpp

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

SOURCE=.\src\FDM\UIUCModel\uiuc_map_Cm.cpp

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

SOURCE=.\src\FDM\UIUCModel\uiuc_map_Croll.cpp

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

SOURCE=.\src\FDM\UIUCModel\uiuc_map_engine.cpp

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

SOURCE=.\src\FDM\UIUCModel\uiuc_map_ice.cpp

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

SOURCE=.\src\FDM\UIUCModel\uiuc_map_keyword.cpp

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

SOURCE=.\src\FDM\UIUCModel\uiuc_map_misc.cpp

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

SOURCE=.\src\FDM\UIUCModel\uiuc_map_record2.cpp

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

SOURCE=.\src\FDM\UIUCModel\uiuc_map_record4.cpp

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

SOURCE=.\src\FDM\UIUCModel\uiuc_menu.cpp

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

SOURCE=.\src\FDM\UIUCModel\uiuc_recorder.cpp

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

SOURCE=.\src\FDM\UIUCModel\uiuc_wrapper.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_UIUCModel"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_UIUCModel"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_Flight"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\FDM\ADA.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Flight"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Flight"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\Balloon.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Flight"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Flight"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\External.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Flight"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Flight"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\flight.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Flight"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Flight"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\IO360.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Flight"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Flight"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\JSBSim.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Flight"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Flight"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsim.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Flight"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Flight"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\LaRCsimIC.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Flight"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Flight"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\FDM\MagicCarpet.cxx

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

SOURCE=.\src\GUI\gui.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_GUI"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_GUI"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_Joystick"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\Joystick\joystick.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Joystick"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Joystick"

!ENDIF 

# End Source File
# End Group
# Begin Group "main"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\Main\main.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\bfi.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\fg_init.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\fg_io.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\globals.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\keyboard.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\options.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\save.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\splash.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\viewer.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\viewer_lookat.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\viewer_rph.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\main"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Main\viewmgr.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\main"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\main"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_Navaids"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\Navaids\fixlist.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Navaids"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Navaids"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Navaids\ilslist.cxx

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

SOURCE=.\src\Network\native.cxx

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

SOURCE=.\src\Network\nmea.cxx

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

SOURCE=.\src\Network\pve.cxx

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

SOURCE=.\src\Network\rul.cxx

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
# End Group
# Begin Group "Lib_NetworkOLK"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\NetworkOLK\net_send.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_NetworkOLK"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_NetworkOLK"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\NetworkOLK\net_hud.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_NetworkOLK"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_NetworkOLK"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\NetworkOLK\network.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_NetworkOLK"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_NetworkOLK"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\NetworkOLK\features.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_NetworkOLK"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_NetworkOLK"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_Objects"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\Objects\newmat.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Objects"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Objects"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Objects\matlib.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Objects"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Objects"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Objects\obj.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Objects"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Objects"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Objects\texload.c

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Objects"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Objects"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_Scenery"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\Scenery\hitlist.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Scenery"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Scenery"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Scenery\newcache.cxx

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

SOURCE=.\src\Scenery\tileentry.cxx

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
# End Group
# Begin Group "Lib_Time"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\Time\event.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Time"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Time"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Time\fg_timer.cxx

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

SOURCE=.\src\Time\moonpos.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Time"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Time"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Time\sunpos.cxx

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
# End Group
# Begin Group "Lib_Weather"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\Weather\weather.cxx

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_Weather"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_Weather"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_WeatherCM"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\WeatherCM\FGAirPressureItem.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_WeatherCM"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_WeatherCM"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\WeatherCM\FGCloudItem.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_WeatherCM"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_WeatherCM"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\WeatherCM\FGLocalWeatherDatabase.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_WeatherCM"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_WeatherCM"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\WeatherCM\FGPhysicalProperties.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_WeatherCM"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_WeatherCM"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\WeatherCM\FGPhysicalProperty.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_WeatherCM"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_WeatherCM"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\WeatherCM\FGTemperatureItem.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_WeatherCM"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_WeatherCM"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\WeatherCM\FGThunderstorm.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_WeatherCM"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_WeatherCM"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\WeatherCM\FGTurbulenceItem.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_WeatherCM"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_WeatherCM"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\WeatherCM\FGVaporPressureItem.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_WeatherCM"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_WeatherCM"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\WeatherCM\FGWeatherParse.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_WeatherCM"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_WeatherCM"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\WeatherCM\FGWindItem.cpp

!IF  "$(CFG)" == "FlightGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_WeatherCM"

!ELSEIF  "$(CFG)" == "FlightGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_WeatherCM"

!ENDIF 

# End Source File
# End Group
# End Target
# End Project
