# Microsoft Developer Studio Generated NMAKE File, Based on FGWin32.dsp
!IF "$(CFG)" == ""
CFG=FGWin32 - Win32 Debug
!MESSAGE No configuration specified. Defaulting to FGWin32 - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "FGWin32 - Win32 Release" && "$(CFG)" !=\
 "FGWin32 - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "FGWin32.mak" CFG="FGWin32 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "FGWin32 - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "FGWin32 - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "FGWin32 - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\FGWin32.exe"

!ELSE 

ALL : "$(OUTDIR)\FGWin32.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\aircraft.obj"
	-@erase "$(INTDIR)\atmos_62.obj"
	-@erase "$(INTDIR)\cockpit.obj"
	-@erase "$(INTDIR)\common.obj"
	-@erase "$(INTDIR)\controls.obj"
	-@erase "$(INTDIR)\default_model_routines.obj"
	-@erase "$(INTDIR)\event.obj"
	-@erase "$(INTDIR)\fg_geodesy.obj"
	-@erase "$(INTDIR)\fg_init.obj"
	-@erase "$(INTDIR)\fg_random.obj"
	-@erase "$(INTDIR)\fg_time.obj"
	-@erase "$(INTDIR)\fg_timer.obj"
	-@erase "$(INTDIR)\flight.obj"
	-@erase "$(INTDIR)\geometry.obj"
	-@erase "$(INTDIR)\GLUTkey.obj"
	-@erase "$(INTDIR)\GLUTmain.obj"
	-@erase "$(INTDIR)\hud.obj"
	-@erase "$(INTDIR)\joystick.obj"
	-@erase "$(INTDIR)\ls_accel.obj"
	-@erase "$(INTDIR)\ls_aux.obj"
	-@erase "$(INTDIR)\ls_geodesy.obj"
	-@erase "$(INTDIR)\ls_gravity.obj"
	-@erase "$(INTDIR)\ls_init.obj"
	-@erase "$(INTDIR)\ls_interface.obj"
	-@erase "$(INTDIR)\ls_model.obj"
	-@erase "$(INTDIR)\ls_step.obj"
	-@erase "$(INTDIR)\MAT3geom.obj"
	-@erase "$(INTDIR)\MAT3inv.obj"
	-@erase "$(INTDIR)\MAT3mat.obj"
	-@erase "$(INTDIR)\MAT3vec.obj"
	-@erase "$(INTDIR)\mesh.obj"
	-@erase "$(INTDIR)\moon.obj"
	-@erase "$(INTDIR)\navion_aero.obj"
	-@erase "$(INTDIR)\navion_engine.obj"
	-@erase "$(INTDIR)\navion_gear.obj"
	-@erase "$(INTDIR)\navion_init.obj"
	-@erase "$(INTDIR)\obj.obj"
	-@erase "$(INTDIR)\orbits.obj"
	-@erase "$(INTDIR)\planets.obj"
	-@erase "$(INTDIR)\polar.obj"
	-@erase "$(INTDIR)\scenery.obj"
	-@erase "$(INTDIR)\sky.obj"
	-@erase "$(INTDIR)\slew.obj"
	-@erase "$(INTDIR)\stars.obj"
	-@erase "$(INTDIR)\sun.obj"
	-@erase "$(INTDIR)\sunpos.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vector.obj"
	-@erase "$(INTDIR)\views.obj"
	-@erase "$(INTDIR)\weather.obj"
	-@erase "$(OUTDIR)\FGWin32.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D\
 "_MBCS" /D "GLUT" /D "USE_RAND" /D "USE_FTIME" /D "__WIN32__" /D "FX"\
 /Fp"$(INTDIR)\FGWin32.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Release/
CPP_SBRS=.

.c{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\FGWin32.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=user32.lib gdi32.lib winmm.lib glut32.lib glu32.lib opengl32.lib\
 /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\FGWin32.pdb"\
 /machine:I386 /out:"$(OUTDIR)\FGWin32.exe" 
LINK32_OBJS= \
	"$(INTDIR)\aircraft.obj" \
	"$(INTDIR)\atmos_62.obj" \
	"$(INTDIR)\cockpit.obj" \
	"$(INTDIR)\common.obj" \
	"$(INTDIR)\controls.obj" \
	"$(INTDIR)\default_model_routines.obj" \
	"$(INTDIR)\event.obj" \
	"$(INTDIR)\fg_geodesy.obj" \
	"$(INTDIR)\fg_init.obj" \
	"$(INTDIR)\fg_random.obj" \
	"$(INTDIR)\fg_time.obj" \
	"$(INTDIR)\fg_timer.obj" \
	"$(INTDIR)\flight.obj" \
	"$(INTDIR)\geometry.obj" \
	"$(INTDIR)\GLUTkey.obj" \
	"$(INTDIR)\GLUTmain.obj" \
	"$(INTDIR)\hud.obj" \
	"$(INTDIR)\joystick.obj" \
	"$(INTDIR)\ls_accel.obj" \
	"$(INTDIR)\ls_aux.obj" \
	"$(INTDIR)\ls_geodesy.obj" \
	"$(INTDIR)\ls_gravity.obj" \
	"$(INTDIR)\ls_init.obj" \
	"$(INTDIR)\ls_interface.obj" \
	"$(INTDIR)\ls_model.obj" \
	"$(INTDIR)\ls_step.obj" \
	"$(INTDIR)\MAT3geom.obj" \
	"$(INTDIR)\MAT3inv.obj" \
	"$(INTDIR)\MAT3mat.obj" \
	"$(INTDIR)\MAT3vec.obj" \
	"$(INTDIR)\mesh.obj" \
	"$(INTDIR)\moon.obj" \
	"$(INTDIR)\navion_aero.obj" \
	"$(INTDIR)\navion_engine.obj" \
	"$(INTDIR)\navion_gear.obj" \
	"$(INTDIR)\navion_init.obj" \
	"$(INTDIR)\obj.obj" \
	"$(INTDIR)\orbits.obj" \
	"$(INTDIR)\planets.obj" \
	"$(INTDIR)\polar.obj" \
	"$(INTDIR)\scenery.obj" \
	"$(INTDIR)\sky.obj" \
	"$(INTDIR)\slew.obj" \
	"$(INTDIR)\stars.obj" \
	"$(INTDIR)\sun.obj" \
	"$(INTDIR)\sunpos.obj" \
	"$(INTDIR)\vector.obj" \
	"$(INTDIR)\views.obj" \
	"$(INTDIR)\weather.obj"

"$(OUTDIR)\FGWin32.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "FGWin32 - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\FGWin32.exe"

!ELSE 

ALL : "$(OUTDIR)\FGWin32.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\aircraft.obj"
	-@erase "$(INTDIR)\atmos_62.obj"
	-@erase "$(INTDIR)\cockpit.obj"
	-@erase "$(INTDIR)\common.obj"
	-@erase "$(INTDIR)\controls.obj"
	-@erase "$(INTDIR)\default_model_routines.obj"
	-@erase "$(INTDIR)\event.obj"
	-@erase "$(INTDIR)\fg_geodesy.obj"
	-@erase "$(INTDIR)\fg_init.obj"
	-@erase "$(INTDIR)\fg_random.obj"
	-@erase "$(INTDIR)\fg_time.obj"
	-@erase "$(INTDIR)\fg_timer.obj"
	-@erase "$(INTDIR)\flight.obj"
	-@erase "$(INTDIR)\geometry.obj"
	-@erase "$(INTDIR)\GLUTkey.obj"
	-@erase "$(INTDIR)\GLUTmain.obj"
	-@erase "$(INTDIR)\hud.obj"
	-@erase "$(INTDIR)\joystick.obj"
	-@erase "$(INTDIR)\ls_accel.obj"
	-@erase "$(INTDIR)\ls_aux.obj"
	-@erase "$(INTDIR)\ls_geodesy.obj"
	-@erase "$(INTDIR)\ls_gravity.obj"
	-@erase "$(INTDIR)\ls_init.obj"
	-@erase "$(INTDIR)\ls_interface.obj"
	-@erase "$(INTDIR)\ls_model.obj"
	-@erase "$(INTDIR)\ls_step.obj"
	-@erase "$(INTDIR)\MAT3geom.obj"
	-@erase "$(INTDIR)\MAT3inv.obj"
	-@erase "$(INTDIR)\MAT3mat.obj"
	-@erase "$(INTDIR)\MAT3vec.obj"
	-@erase "$(INTDIR)\mesh.obj"
	-@erase "$(INTDIR)\moon.obj"
	-@erase "$(INTDIR)\navion_aero.obj"
	-@erase "$(INTDIR)\navion_engine.obj"
	-@erase "$(INTDIR)\navion_gear.obj"
	-@erase "$(INTDIR)\navion_init.obj"
	-@erase "$(INTDIR)\obj.obj"
	-@erase "$(INTDIR)\orbits.obj"
	-@erase "$(INTDIR)\planets.obj"
	-@erase "$(INTDIR)\polar.obj"
	-@erase "$(INTDIR)\scenery.obj"
	-@erase "$(INTDIR)\sky.obj"
	-@erase "$(INTDIR)\slew.obj"
	-@erase "$(INTDIR)\stars.obj"
	-@erase "$(INTDIR)\sun.obj"
	-@erase "$(INTDIR)\sunpos.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(INTDIR)\vector.obj"
	-@erase "$(INTDIR)\views.obj"
	-@erase "$(INTDIR)\weather.obj"
	-@erase "$(OUTDIR)\FGWin32.exe"
	-@erase "$(OUTDIR)\FGWin32.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_CONSOLE"\
 /D "_MBCS" /D "GLUT" /D "USE_RAND" /D "USE_FTIME" /D "__WIN32__" /D "FX" /D\
 "NO_PRINTF" /Fp"$(INTDIR)\FGWin32.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"\
 /FD /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.

.c{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\FGWin32.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=opengl32.lib glu32.lib glut32.lib user32.lib gdi32.lib\
 kernel32.lib glide2x.lib texus.lib winmm.lib /nologo /subsystem:console\
 /verbose /incremental:no /pdb:"$(OUTDIR)\FGWin32.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)\FGWin32.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\aircraft.obj" \
	"$(INTDIR)\atmos_62.obj" \
	"$(INTDIR)\cockpit.obj" \
	"$(INTDIR)\common.obj" \
	"$(INTDIR)\controls.obj" \
	"$(INTDIR)\default_model_routines.obj" \
	"$(INTDIR)\event.obj" \
	"$(INTDIR)\fg_geodesy.obj" \
	"$(INTDIR)\fg_init.obj" \
	"$(INTDIR)\fg_random.obj" \
	"$(INTDIR)\fg_time.obj" \
	"$(INTDIR)\fg_timer.obj" \
	"$(INTDIR)\flight.obj" \
	"$(INTDIR)\geometry.obj" \
	"$(INTDIR)\GLUTkey.obj" \
	"$(INTDIR)\GLUTmain.obj" \
	"$(INTDIR)\hud.obj" \
	"$(INTDIR)\joystick.obj" \
	"$(INTDIR)\ls_accel.obj" \
	"$(INTDIR)\ls_aux.obj" \
	"$(INTDIR)\ls_geodesy.obj" \
	"$(INTDIR)\ls_gravity.obj" \
	"$(INTDIR)\ls_init.obj" \
	"$(INTDIR)\ls_interface.obj" \
	"$(INTDIR)\ls_model.obj" \
	"$(INTDIR)\ls_step.obj" \
	"$(INTDIR)\MAT3geom.obj" \
	"$(INTDIR)\MAT3inv.obj" \
	"$(INTDIR)\MAT3mat.obj" \
	"$(INTDIR)\MAT3vec.obj" \
	"$(INTDIR)\mesh.obj" \
	"$(INTDIR)\moon.obj" \
	"$(INTDIR)\navion_aero.obj" \
	"$(INTDIR)\navion_engine.obj" \
	"$(INTDIR)\navion_gear.obj" \
	"$(INTDIR)\navion_init.obj" \
	"$(INTDIR)\obj.obj" \
	"$(INTDIR)\orbits.obj" \
	"$(INTDIR)\planets.obj" \
	"$(INTDIR)\polar.obj" \
	"$(INTDIR)\scenery.obj" \
	"$(INTDIR)\sky.obj" \
	"$(INTDIR)\slew.obj" \
	"$(INTDIR)\stars.obj" \
	"$(INTDIR)\sun.obj" \
	"$(INTDIR)\sunpos.obj" \
	"$(INTDIR)\vector.obj" \
	"$(INTDIR)\views.obj" \
	"$(INTDIR)\weather.obj"

"$(OUTDIR)\FGWin32.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(CFG)" == "FGWin32 - Win32 Release" || "$(CFG)" ==\
 "FGWin32 - Win32 Debug"
SOURCE=..\Src\Aircraft\aircraft.c

!IF  "$(CFG)" == "FGWin32 - Win32 Release"

DEP_CPP_AIRCR=\
	"..\src\aircraft\aircraft.h"\
	"..\src\controls\controls.h"\
	"..\src\flight\flight.h"\
	"..\src\flight\larcsim\ls_interface.h"\
	"..\src\flight\slew\slew.h"\
	"..\src\include\constants.h"\
	

"$(INTDIR)\aircraft.obj" : $(SOURCE) $(DEP_CPP_AIRCR) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "FGWin32 - Win32 Debug"

DEP_CPP_AIRCR=\
	"..\src\aircraft\aircraft.h"\
	"..\src\controls\controls.h"\
	"..\src\flight\flight.h"\
	"..\src\flight\larcsim\ls_interface.h"\
	"..\src\flight\slew\slew.h"\
	"..\src\include\constants.h"\
	

"$(INTDIR)\aircraft.obj" : $(SOURCE) $(DEP_CPP_AIRCR) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\Src\Flight\LaRCsim\atmos_62.c

!IF  "$(CFG)" == "FGWin32 - Win32 Release"

DEP_CPP_ATMOS=\
	"..\src\flight\larcsim\ls_constants.h"\
	"..\src\flight\larcsim\ls_types.h"\
	

"$(INTDIR)\atmos_62.obj" : $(SOURCE) $(DEP_CPP_ATMOS) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "FGWin32 - Win32 Debug"

DEP_CPP_ATMOS=\
	"..\src\flight\larcsim\ls_constants.h"\
	"..\src\flight\larcsim\ls_types.h"\
	

"$(INTDIR)\atmos_62.obj" : $(SOURCE) $(DEP_CPP_ATMOS) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\Src\Cockpit\cockpit.c

!IF  "$(CFG)" == "FGWin32 - Win32 Release"

DEP_CPP_COCKP=\
	"..\..\devstudio\vc\include\gl\glut.h"\
	"..\src\aircraft\aircraft.h"\
	"..\src\cockpit\cockpit.h"\
	"..\src\cockpit\hud.h"\
	"..\src\controls\controls.h"\
	"..\src\flight\flight.h"\
	"..\src\flight\larcsim\ls_interface.h"\
	"..\src\flight\slew\slew.h"\
	"..\src\include\constants.h"\
	"..\src\math\fg_random.h"\
	"..\src\math\mat3.h"\
	"..\src\math\polar.h"\
	"..\src\scenery\mesh.h"\
	"..\src\scenery\scenery.h"\
	"..\src\time\fg_timer.h"\
	"..\src\weather\weather.h"\
	

"$(INTDIR)\cockpit.obj" : $(SOURCE) $(DEP_CPP_COCKP) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "FGWin32 - Win32 Debug"

DEP_CPP_COCKP=\
	"..\..\devstudio\vc\include\gl\glut.h"\
	"..\src\aircraft\aircraft.h"\
	"..\src\cockpit\cockpit.h"\
	"..\src\cockpit\hud.h"\
	"..\src\controls\controls.h"\
	"..\src\flight\flight.h"\
	"..\src\flight\larcsim\ls_interface.h"\
	"..\src\flight\slew\slew.h"\
	"..\src\include\constants.h"\
	"..\src\math\fg_random.h"\
	"..\src\math\mat3.h"\
	"..\src\math\polar.h"\
	"..\src\scenery\mesh.h"\
	"..\src\scenery\scenery.h"\
	"..\src\time\fg_timer.h"\
	"..\src\weather\weather.h"\
	

"$(INTDIR)\cockpit.obj" : $(SOURCE) $(DEP_CPP_COCKP) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\Src\Scenery\common.c
DEP_CPP_COMMO=\
	"..\src\scenery\common.h"\
	

"$(INTDIR)\common.obj" : $(SOURCE) $(DEP_CPP_COMMO) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\Src\Controls\controls.c
DEP_CPP_CONTR=\
	"..\src\aircraft\aircraft.h"\
	"..\src\controls\controls.h"\
	"..\src\flight\flight.h"\
	"..\src\flight\larcsim\ls_interface.h"\
	"..\src\flight\slew\slew.h"\
	

"$(INTDIR)\controls.obj" : $(SOURCE) $(DEP_CPP_CONTR) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\Src\Flight\LaRCsim\default_model_routines.c

"$(INTDIR)\default_model_routines.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\Src\Time\event.c
DEP_CPP_EVENT=\
	"..\src\time\event.h"\
	

"$(INTDIR)\event.obj" : $(SOURCE) $(DEP_CPP_EVENT) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\Src\Math\fg_geodesy.c

!IF  "$(CFG)" == "FGWin32 - Win32 Release"

DEP_CPP_FG_GE=\
	"..\src\include\constants.h"\
	"..\src\math\fg_geodesy.h"\
	

"$(INTDIR)\fg_geodesy.obj" : $(SOURCE) $(DEP_CPP_FG_GE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "FGWin32 - Win32 Debug"

DEP_CPP_FG_GE=\
	"..\src\include\constants.h"\
	"..\src\math\fg_geodesy.h"\
	

"$(INTDIR)\fg_geodesy.obj" : $(SOURCE) $(DEP_CPP_FG_GE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\Src\Main\fg_init.c

!IF  "$(CFG)" == "FGWin32 - Win32 Release"

DEP_CPP_FG_IN=\
	"..\..\devstudio\vc\include\gl\glut.h"\
	"..\src\aircraft\aircraft.h"\
	"..\src\cockpit\cockpit.h"\
	"..\src\cockpit\hud.h"\
	"..\src\controls\controls.h"\
	"..\src\flight\flight.h"\
	"..\src\flight\larcsim\ls_interface.h"\
	"..\src\flight\slew\slew.h"\
	"..\src\include\constants.h"\
	"..\src\include\general.h"\
	"..\src\joystick\joystick.h"\
	"..\src\main\fg_init.h"\
	"..\src\main\views.h"\
	"..\src\math\fg_random.h"\
	"..\src\math\mat3.h"\
	"..\src\scenery\mesh.h"\
	"..\src\scenery\moon.h"\
	"..\src\scenery\orbits.h"\
	"..\src\scenery\scenery.h"\
	"..\src\scenery\sky.h"\
	"..\src\scenery\stars.h"\
	"..\src\scenery\sun.h"\
	"..\src\time\event.h"\
	"..\src\time\fg_time.h"\
	"..\src\time\sunpos.h"\
	"..\src\weather\weather.h"\
	

"$(INTDIR)\fg_init.obj" : $(SOURCE) $(DEP_CPP_FG_IN) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "FGWin32 - Win32 Debug"

DEP_CPP_FG_IN=\
	"..\..\devstudio\vc\include\gl\glut.h"\
	"..\src\aircraft\aircraft.h"\
	"..\src\cockpit\cockpit.h"\
	"..\src\cockpit\hud.h"\
	"..\src\controls\controls.h"\
	"..\src\flight\flight.h"\
	"..\src\flight\larcsim\ls_interface.h"\
	"..\src\flight\slew\slew.h"\
	"..\src\include\constants.h"\
	"..\src\include\general.h"\
	"..\src\joystick\joystick.h"\
	"..\src\main\fg_init.h"\
	"..\src\main\views.h"\
	"..\src\math\fg_random.h"\
	"..\src\math\mat3.h"\
	"..\src\scenery\mesh.h"\
	"..\src\scenery\moon.h"\
	"..\src\scenery\orbits.h"\
	"..\src\scenery\scenery.h"\
	"..\src\scenery\sky.h"\
	"..\src\scenery\stars.h"\
	"..\src\scenery\sun.h"\
	"..\src\time\event.h"\
	"..\src\time\fg_time.h"\
	"..\src\time\sunpos.h"\
	"..\src\weather\weather.h"\
	

"$(INTDIR)\fg_init.obj" : $(SOURCE) $(DEP_CPP_FG_IN) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\Src\Math\fg_random.c
DEP_CPP_FG_RA=\
	"..\src\math\fg_random.h"\
	

"$(INTDIR)\fg_random.obj" : $(SOURCE) $(DEP_CPP_FG_RA) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\Src\Time\fg_time.c

!IF  "$(CFG)" == "FGWin32 - Win32 Release"

DEP_CPP_FG_TI=\
	"..\..\devstudio\vc\include\gl\glut.h"\
	"..\src\flight\flight.h"\
	"..\src\flight\larcsim\ls_interface.h"\
	"..\src\flight\slew\slew.h"\
	"..\src\include\constants.h"\
	"..\src\time\fg_time.h"\
	

"$(INTDIR)\fg_time.obj" : $(SOURCE) $(DEP_CPP_FG_TI) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "FGWin32 - Win32 Debug"

DEP_CPP_FG_TI=\
	"..\..\devstudio\vc\include\gl\glut.h"\
	"..\src\flight\flight.h"\
	"..\src\flight\larcsim\ls_interface.h"\
	"..\src\flight\slew\slew.h"\
	"..\src\include\constants.h"\
	"..\src\time\fg_time.h"\
	

"$(INTDIR)\fg_time.obj" : $(SOURCE) $(DEP_CPP_FG_TI) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\Src\Time\fg_timer.c
DEP_CPP_FG_TIM=\
	"..\src\time\fg_timer.h"\
	

"$(INTDIR)\fg_timer.obj" : $(SOURCE) $(DEP_CPP_FG_TIM) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\Src\Flight\flight.c

!IF  "$(CFG)" == "FGWin32 - Win32 Release"

DEP_CPP_FLIGH=\
	"..\src\flight\flight.h"\
	"..\src\flight\larcsim\ls_interface.h"\
	"..\src\flight\slew\slew.h"\
	

"$(INTDIR)\flight.obj" : $(SOURCE) $(DEP_CPP_FLIGH) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "FGWin32 - Win32 Debug"

DEP_CPP_FLIGH=\
	"..\src\flight\flight.h"\
	"..\src\flight\larcsim\ls_interface.h"\
	"..\src\flight\slew\slew.h"\
	

"$(INTDIR)\flight.obj" : $(SOURCE) $(DEP_CPP_FLIGH) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\Src\Scenery\geometry.c
DEP_CPP_GEOME=\
	"..\..\devstudio\vc\include\gl\glut.h"\
	"..\src\scenery\geometry.h"\
	"..\src\scenery\mesh.h"\
	

"$(INTDIR)\geometry.obj" : $(SOURCE) $(DEP_CPP_GEOME) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\Src\Main\GLUTkey.c

!IF  "$(CFG)" == "FGWin32 - Win32 Release"

DEP_CPP_GLUTK=\
	"..\..\devstudio\vc\include\gl\glut.h"\
	"..\src\aircraft\aircraft.h"\
	"..\src\controls\controls.h"\
	"..\src\flight\flight.h"\
	"..\src\flight\larcsim\ls_interface.h"\
	"..\src\flight\slew\slew.h"\
	"..\src\include\constants.h"\
	"..\src\main\glutkey.h"\
	"..\src\main\views.h"\
	"..\src\math\mat3.h"\
	"..\src\time\fg_time.h"\
	"..\src\weather\weather.h"\
	"..\src\xgl\xgl.h"\
	

"$(INTDIR)\GLUTkey.obj" : $(SOURCE) $(DEP_CPP_GLUTK) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "FGWin32 - Win32 Debug"

DEP_CPP_GLUTK=\
	"..\..\devstudio\vc\include\gl\glut.h"\
	"..\src\aircraft\aircraft.h"\
	"..\src\controls\controls.h"\
	"..\src\flight\flight.h"\
	"..\src\flight\larcsim\ls_interface.h"\
	"..\src\flight\slew\slew.h"\
	"..\src\include\constants.h"\
	"..\src\main\glutkey.h"\
	"..\src\main\views.h"\
	"..\src\math\mat3.h"\
	"..\src\time\fg_time.h"\
	"..\src\weather\weather.h"\
	"..\src\xgl\xgl.h"\
	

"$(INTDIR)\GLUTkey.obj" : $(SOURCE) $(DEP_CPP_GLUTK) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\Src\Main\GLUTmain.c

!IF  "$(CFG)" == "FGWin32 - Win32 Release"

DEP_CPP_GLUTM=\
	"..\..\devstudio\vc\include\gl\glut.h"\
	"..\src\aircraft\aircraft.h"\
	"..\src\cockpit\cockpit.h"\
	"..\src\cockpit\hud.h"\
	"..\src\controls\controls.h"\
	"..\src\flight\flight.h"\
	"..\src\flight\larcsim\ls_interface.h"\
	"..\src\flight\slew\slew.h"\
	"..\src\include\constants.h"\
	"..\src\include\general.h"\
	"..\src\joystick\joystick.h"\
	"..\src\main\fg_init.h"\
	"..\src\main\glutkey.h"\
	"..\src\main\views.h"\
	"..\src\math\fg_geodesy.h"\
	"..\src\math\mat3.h"\
	"..\src\math\polar.h"\
	"..\src\scenery\mesh.h"\
	"..\src\scenery\moon.h"\
	"..\src\scenery\orbits.h"\
	"..\src\scenery\scenery.h"\
	"..\src\scenery\sky.h"\
	"..\src\scenery\stars.h"\
	"..\src\scenery\sun.h"\
	"..\src\time\event.h"\
	"..\src\time\fg_time.h"\
	"..\src\time\fg_timer.h"\
	"..\src\time\sunpos.h"\
	"..\src\weather\weather.h"\
	"..\src\xgl\xgl.h"\
	

"$(INTDIR)\GLUTmain.obj" : $(SOURCE) $(DEP_CPP_GLUTM) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "FGWin32 - Win32 Debug"

DEP_CPP_GLUTM=\
	"..\..\devstudio\vc\include\gl\glut.h"\
	"..\src\aircraft\aircraft.h"\
	"..\src\cockpit\cockpit.h"\
	"..\src\cockpit\hud.h"\
	"..\src\controls\controls.h"\
	"..\src\flight\flight.h"\
	"..\src\flight\larcsim\ls_interface.h"\
	"..\src\flight\slew\slew.h"\
	"..\src\include\constants.h"\
	"..\src\include\general.h"\
	"..\src\joystick\joystick.h"\
	"..\src\main\fg_init.h"\
	"..\src\main\glutkey.h"\
	"..\src\main\views.h"\
	"..\src\math\fg_geodesy.h"\
	"..\src\math\mat3.h"\
	"..\src\math\polar.h"\
	"..\src\scenery\mesh.h"\
	"..\src\scenery\moon.h"\
	"..\src\scenery\orbits.h"\
	"..\src\scenery\scenery.h"\
	"..\src\scenery\sky.h"\
	"..\src\scenery\stars.h"\
	"..\src\scenery\sun.h"\
	"..\src\time\event.h"\
	"..\src\time\fg_time.h"\
	"..\src\time\fg_timer.h"\
	"..\src\time\sunpos.h"\
	"..\src\weather\weather.h"\
	"..\src\xgl\xgl.h"\
	

"$(INTDIR)\GLUTmain.obj" : $(SOURCE) $(DEP_CPP_GLUTM) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\Src\Cockpit\hud.c

!IF  "$(CFG)" == "FGWin32 - Win32 Release"

DEP_CPP_HUD_C=\
	"..\..\devstudio\vc\include\gl\glut.h"\
	"..\src\aircraft\aircraft.h"\
	"..\src\cockpit\hud.h"\
	"..\src\controls\controls.h"\
	"..\src\flight\flight.h"\
	"..\src\flight\larcsim\ls_interface.h"\
	"..\src\flight\slew\slew.h"\
	"..\src\include\constants.h"\
	"..\src\math\fg_random.h"\
	"..\src\math\mat3.h"\
	"..\src\math\polar.h"\
	"..\src\scenery\mesh.h"\
	"..\src\scenery\scenery.h"\
	"..\src\time\fg_timer.h"\
	"..\src\weather\weather.h"\
	

"$(INTDIR)\hud.obj" : $(SOURCE) $(DEP_CPP_HUD_C) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "FGWin32 - Win32 Debug"

DEP_CPP_HUD_C=\
	"..\..\devstudio\vc\include\gl\glut.h"\
	"..\src\aircraft\aircraft.h"\
	"..\src\cockpit\hud.h"\
	"..\src\controls\controls.h"\
	"..\src\flight\flight.h"\
	"..\src\flight\larcsim\ls_interface.h"\
	"..\src\flight\slew\slew.h"\
	"..\src\include\constants.h"\
	"..\src\math\fg_random.h"\
	"..\src\math\mat3.h"\
	"..\src\math\polar.h"\
	"..\src\scenery\mesh.h"\
	"..\src\scenery\scenery.h"\
	"..\src\time\fg_timer.h"\
	"..\src\weather\weather.h"\
	

"$(INTDIR)\hud.obj" : $(SOURCE) $(DEP_CPP_HUD_C) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\Src\Joystick\joystick.c

"$(INTDIR)\joystick.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\Src\Flight\LaRCsim\ls_accel.c

!IF  "$(CFG)" == "FGWin32 - Win32 Release"

DEP_CPP_LS_AC=\
	"..\src\flight\larcsim\ls_constants.h"\
	"..\src\flight\larcsim\ls_generic.h"\
	"..\src\flight\larcsim\ls_types.h"\
	

"$(INTDIR)\ls_accel.obj" : $(SOURCE) $(DEP_CPP_LS_AC) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "FGWin32 - Win32 Debug"

DEP_CPP_LS_AC=\
	"..\src\flight\larcsim\ls_constants.h"\
	"..\src\flight\larcsim\ls_generic.h"\
	"..\src\flight\larcsim\ls_types.h"\
	

"$(INTDIR)\ls_accel.obj" : $(SOURCE) $(DEP_CPP_LS_AC) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\Src\Flight\LaRCsim\ls_aux.c

!IF  "$(CFG)" == "FGWin32 - Win32 Release"

DEP_CPP_LS_AU=\
	"..\src\flight\larcsim\ls_constants.h"\
	"..\src\flight\larcsim\ls_generic.h"\
	"..\src\flight\larcsim\ls_types.h"\
	

"$(INTDIR)\ls_aux.obj" : $(SOURCE) $(DEP_CPP_LS_AU) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "FGWin32 - Win32 Debug"

DEP_CPP_LS_AU=\
	"..\src\flight\larcsim\ls_constants.h"\
	"..\src\flight\larcsim\ls_generic.h"\
	"..\src\flight\larcsim\ls_types.h"\
	

"$(INTDIR)\ls_aux.obj" : $(SOURCE) $(DEP_CPP_LS_AU) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\Src\Flight\LaRCsim\ls_geodesy.c

!IF  "$(CFG)" == "FGWin32 - Win32 Release"

DEP_CPP_LS_GE=\
	"..\src\flight\larcsim\ls_constants.h"\
	"..\src\flight\larcsim\ls_types.h"\
	

"$(INTDIR)\ls_geodesy.obj" : $(SOURCE) $(DEP_CPP_LS_GE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "FGWin32 - Win32 Debug"

DEP_CPP_LS_GE=\
	"..\src\flight\larcsim\ls_constants.h"\
	"..\src\flight\larcsim\ls_types.h"\
	

"$(INTDIR)\ls_geodesy.obj" : $(SOURCE) $(DEP_CPP_LS_GE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\Src\Flight\LaRCsim\ls_gravity.c

!IF  "$(CFG)" == "FGWin32 - Win32 Release"

DEP_CPP_LS_GR=\
	"..\src\flight\larcsim\ls_constants.h"\
	"..\src\flight\larcsim\ls_types.h"\
	

"$(INTDIR)\ls_gravity.obj" : $(SOURCE) $(DEP_CPP_LS_GR) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "FGWin32 - Win32 Debug"

DEP_CPP_LS_GR=\
	"..\src\flight\larcsim\ls_constants.h"\
	"..\src\flight\larcsim\ls_types.h"\
	

"$(INTDIR)\ls_gravity.obj" : $(SOURCE) $(DEP_CPP_LS_GR) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\Src\Flight\LaRCsim\ls_init.c

!IF  "$(CFG)" == "FGWin32 - Win32 Release"

DEP_CPP_LS_IN=\
	"..\src\flight\larcsim\ls_generic.h"\
	"..\src\flight\larcsim\ls_sym.h"\
	"..\src\flight\larcsim\ls_types.h"\
	

"$(INTDIR)\ls_init.obj" : $(SOURCE) $(DEP_CPP_LS_IN) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "FGWin32 - Win32 Debug"

DEP_CPP_LS_IN=\
	"..\src\flight\larcsim\ls_generic.h"\
	"..\src\flight\larcsim\ls_sym.h"\
	"..\src\flight\larcsim\ls_types.h"\
	

"$(INTDIR)\ls_init.obj" : $(SOURCE) $(DEP_CPP_LS_IN) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\Src\Flight\LaRCsim\ls_interface.c

!IF  "$(CFG)" == "FGWin32 - Win32 Release"

DEP_CPP_LS_INT=\
	"..\src\aircraft\aircraft.h"\
	"..\src\controls\controls.h"\
	"..\src\flight\flight.h"\
	"..\src\flight\larcsim\ls_cockpit.h"\
	"..\src\flight\larcsim\ls_constants.h"\
	"..\src\flight\larcsim\ls_generic.h"\
	"..\src\flight\larcsim\ls_interface.h"\
	"..\src\flight\larcsim\ls_sim_control.h"\
	"..\src\flight\larcsim\ls_types.h"\
	"..\src\flight\slew\slew.h"\
	

"$(INTDIR)\ls_interface.obj" : $(SOURCE) $(DEP_CPP_LS_INT) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "FGWin32 - Win32 Debug"

DEP_CPP_LS_INT=\
	"..\src\aircraft\aircraft.h"\
	"..\src\controls\controls.h"\
	"..\src\flight\flight.h"\
	"..\src\flight\larcsim\ls_cockpit.h"\
	"..\src\flight\larcsim\ls_constants.h"\
	"..\src\flight\larcsim\ls_generic.h"\
	"..\src\flight\larcsim\ls_interface.h"\
	"..\src\flight\larcsim\ls_sim_control.h"\
	"..\src\flight\larcsim\ls_types.h"\
	"..\src\flight\slew\slew.h"\
	

"$(INTDIR)\ls_interface.obj" : $(SOURCE) $(DEP_CPP_LS_INT) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\Src\Flight\LaRCsim\ls_model.c
DEP_CPP_LS_MO=\
	"..\src\flight\larcsim\ls_types.h"\
	

"$(INTDIR)\ls_model.obj" : $(SOURCE) $(DEP_CPP_LS_MO) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\Src\Flight\LaRCsim\ls_step.c

!IF  "$(CFG)" == "FGWin32 - Win32 Release"

DEP_CPP_LS_ST=\
	"..\src\flight\larcsim\ls_constants.h"\
	"..\src\flight\larcsim\ls_generic.h"\
	"..\src\flight\larcsim\ls_types.h"\
	

"$(INTDIR)\ls_step.obj" : $(SOURCE) $(DEP_CPP_LS_ST) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "FGWin32 - Win32 Debug"

DEP_CPP_LS_ST=\
	"..\src\flight\larcsim\ls_constants.h"\
	"..\src\flight\larcsim\ls_generic.h"\
	"..\src\flight\larcsim\ls_types.h"\
	

"$(INTDIR)\ls_step.obj" : $(SOURCE) $(DEP_CPP_LS_ST) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\Src\Math\MAT3geom.c

!IF  "$(CFG)" == "FGWin32 - Win32 Release"

DEP_CPP_MAT3G=\
	"..\src\math\mat3.h"\
	"..\src\math\mat3defs.h"\
	

"$(INTDIR)\MAT3geom.obj" : $(SOURCE) $(DEP_CPP_MAT3G) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "FGWin32 - Win32 Debug"

DEP_CPP_MAT3G=\
	"..\src\math\mat3.h"\
	"..\src\math\mat3defs.h"\
	

"$(INTDIR)\MAT3geom.obj" : $(SOURCE) $(DEP_CPP_MAT3G) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\Src\Math\MAT3inv.c

!IF  "$(CFG)" == "FGWin32 - Win32 Release"

DEP_CPP_MAT3I=\
	"..\src\math\mat3.h"\
	"..\src\math\mat3defs.h"\
	

"$(INTDIR)\MAT3inv.obj" : $(SOURCE) $(DEP_CPP_MAT3I) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "FGWin32 - Win32 Debug"

DEP_CPP_MAT3I=\
	"..\src\math\mat3.h"\
	"..\src\math\mat3defs.h"\
	

"$(INTDIR)\MAT3inv.obj" : $(SOURCE) $(DEP_CPP_MAT3I) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\Src\Math\MAT3mat.c

!IF  "$(CFG)" == "FGWin32 - Win32 Release"

DEP_CPP_MAT3M=\
	"..\src\math\mat3.h"\
	"..\src\math\mat3defs.h"\
	

"$(INTDIR)\MAT3mat.obj" : $(SOURCE) $(DEP_CPP_MAT3M) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "FGWin32 - Win32 Debug"

DEP_CPP_MAT3M=\
	"..\src\math\mat3.h"\
	"..\src\math\mat3defs.h"\
	

"$(INTDIR)\MAT3mat.obj" : $(SOURCE) $(DEP_CPP_MAT3M) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\Src\Math\MAT3vec.c
DEP_CPP_MAT3V=\
	"..\src\math\mat3.h"\
	

"$(INTDIR)\MAT3vec.obj" : $(SOURCE) $(DEP_CPP_MAT3V) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\Src\Scenery\mesh.c

!IF  "$(CFG)" == "FGWin32 - Win32 Release"

DEP_CPP_MESH_=\
	"..\..\devstudio\vc\include\gl\glut.h"\
	"..\src\include\constants.h"\
	"..\src\math\fg_geodesy.h"\
	"..\src\math\fg_random.h"\
	"..\src\math\mat3.h"\
	"..\src\math\polar.h"\
	"..\src\scenery\common.h"\
	"..\src\scenery\mesh.h"\
	"..\src\scenery\scenery.h"\
	

"$(INTDIR)\mesh.obj" : $(SOURCE) $(DEP_CPP_MESH_) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "FGWin32 - Win32 Debug"

DEP_CPP_MESH_=\
	"..\..\devstudio\vc\include\gl\glut.h"\
	"..\src\include\constants.h"\
	"..\src\math\fg_geodesy.h"\
	"..\src\math\fg_random.h"\
	"..\src\math\mat3.h"\
	"..\src\math\polar.h"\
	"..\src\scenery\common.h"\
	"..\src\scenery\mesh.h"\
	"..\src\scenery\scenery.h"\
	

"$(INTDIR)\mesh.obj" : $(SOURCE) $(DEP_CPP_MESH_) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\Src\Scenery\moon.c

!IF  "$(CFG)" == "FGWin32 - Win32 Release"

DEP_CPP_MOON_=\
	"..\..\devstudio\vc\include\gl\glut.h"\
	"..\src\aircraft\aircraft.h"\
	"..\src\controls\controls.h"\
	"..\src\flight\flight.h"\
	"..\src\flight\larcsim\ls_interface.h"\
	"..\src\flight\slew\slew.h"\
	"..\src\include\constants.h"\
	"..\src\include\general.h"\
	"..\src\main\views.h"\
	"..\src\math\mat3.h"\
	"..\src\scenery\moon.h"\
	"..\src\scenery\orbits.h"\
	"..\src\time\fg_time.h"\
	"..\src\xgl\xgl.h"\
	

"$(INTDIR)\moon.obj" : $(SOURCE) $(DEP_CPP_MOON_) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "FGWin32 - Win32 Debug"

DEP_CPP_MOON_=\
	"..\..\devstudio\vc\include\gl\glut.h"\
	"..\src\aircraft\aircraft.h"\
	"..\src\controls\controls.h"\
	"..\src\flight\flight.h"\
	"..\src\flight\larcsim\ls_interface.h"\
	"..\src\flight\slew\slew.h"\
	"..\src\include\constants.h"\
	"..\src\include\general.h"\
	"..\src\main\views.h"\
	"..\src\math\mat3.h"\
	"..\src\scenery\moon.h"\
	"..\src\scenery\orbits.h"\
	"..\src\time\fg_time.h"\
	"..\src\xgl\xgl.h"\
	

"$(INTDIR)\moon.obj" : $(SOURCE) $(DEP_CPP_MOON_) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\Src\Flight\LaRCsim\navion_aero.c

!IF  "$(CFG)" == "FGWin32 - Win32 Release"

DEP_CPP_NAVIO=\
	"..\src\flight\larcsim\ls_cockpit.h"\
	"..\src\flight\larcsim\ls_generic.h"\
	"..\src\flight\larcsim\ls_types.h"\
	

"$(INTDIR)\navion_aero.obj" : $(SOURCE) $(DEP_CPP_NAVIO) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "FGWin32 - Win32 Debug"

DEP_CPP_NAVIO=\
	"..\src\flight\larcsim\ls_cockpit.h"\
	"..\src\flight\larcsim\ls_generic.h"\
	"..\src\flight\larcsim\ls_types.h"\
	

"$(INTDIR)\navion_aero.obj" : $(SOURCE) $(DEP_CPP_NAVIO) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\Src\Flight\LaRCsim\navion_engine.c

!IF  "$(CFG)" == "FGWin32 - Win32 Release"

DEP_CPP_NAVION=\
	"..\src\flight\larcsim\ls_cockpit.h"\
	"..\src\flight\larcsim\ls_constants.h"\
	"..\src\flight\larcsim\ls_generic.h"\
	"..\src\flight\larcsim\ls_sim_control.h"\
	"..\src\flight\larcsim\ls_types.h"\
	

"$(INTDIR)\navion_engine.obj" : $(SOURCE) $(DEP_CPP_NAVION) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "FGWin32 - Win32 Debug"

DEP_CPP_NAVION=\
	"..\src\flight\larcsim\ls_cockpit.h"\
	"..\src\flight\larcsim\ls_constants.h"\
	"..\src\flight\larcsim\ls_generic.h"\
	"..\src\flight\larcsim\ls_sim_control.h"\
	"..\src\flight\larcsim\ls_types.h"\
	

"$(INTDIR)\navion_engine.obj" : $(SOURCE) $(DEP_CPP_NAVION) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\Src\Flight\LaRCsim\navion_gear.c

!IF  "$(CFG)" == "FGWin32 - Win32 Release"

DEP_CPP_NAVION_=\
	"..\src\flight\larcsim\ls_cockpit.h"\
	"..\src\flight\larcsim\ls_constants.h"\
	"..\src\flight\larcsim\ls_generic.h"\
	"..\src\flight\larcsim\ls_types.h"\
	

"$(INTDIR)\navion_gear.obj" : $(SOURCE) $(DEP_CPP_NAVION_) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "FGWin32 - Win32 Debug"

DEP_CPP_NAVION_=\
	"..\src\flight\larcsim\ls_cockpit.h"\
	"..\src\flight\larcsim\ls_constants.h"\
	"..\src\flight\larcsim\ls_generic.h"\
	"..\src\flight\larcsim\ls_types.h"\
	

"$(INTDIR)\navion_gear.obj" : $(SOURCE) $(DEP_CPP_NAVION_) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\Src\Flight\LaRCsim\navion_init.c

!IF  "$(CFG)" == "FGWin32 - Win32 Release"

DEP_CPP_NAVION_I=\
	"..\src\flight\larcsim\ls_cockpit.h"\
	"..\src\flight\larcsim\ls_generic.h"\
	"..\src\flight\larcsim\ls_types.h"\
	

"$(INTDIR)\navion_init.obj" : $(SOURCE) $(DEP_CPP_NAVION_I) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "FGWin32 - Win32 Debug"

DEP_CPP_NAVION_I=\
	"..\src\flight\larcsim\ls_cockpit.h"\
	"..\src\flight\larcsim\ls_generic.h"\
	"..\src\flight\larcsim\ls_types.h"\
	

"$(INTDIR)\navion_init.obj" : $(SOURCE) $(DEP_CPP_NAVION_I) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\Src\Scenery\obj.c

!IF  "$(CFG)" == "FGWin32 - Win32 Release"

DEP_CPP_OBJ_C=\
	"..\..\devstudio\vc\include\gl\glut.h"\
	"..\src\math\mat3.h"\
	"..\src\scenery\obj.h"\
	"..\src\scenery\scenery.h"\
	"..\src\xgl\xgl.h"\
	

"$(INTDIR)\obj.obj" : $(SOURCE) $(DEP_CPP_OBJ_C) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "FGWin32 - Win32 Debug"

DEP_CPP_OBJ_C=\
	"..\..\devstudio\vc\include\gl\glut.h"\
	"..\src\math\mat3.h"\
	"..\src\scenery\obj.h"\
	"..\src\scenery\scenery.h"\
	"..\src\xgl\xgl.h"\
	

"$(INTDIR)\obj.obj" : $(SOURCE) $(DEP_CPP_OBJ_C) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\Src\Scenery\orbits.c

!IF  "$(CFG)" == "FGWin32 - Win32 Release"

DEP_CPP_ORBIT=\
	"..\..\devstudio\vc\include\gl\glut.h"\
	"..\src\flight\flight.h"\
	"..\src\flight\larcsim\ls_interface.h"\
	"..\src\flight\slew\slew.h"\
	"..\src\include\general.h"\
	"..\src\scenery\orbits.h"\
	"..\src\time\fg_time.h"\
	

"$(INTDIR)\orbits.obj" : $(SOURCE) $(DEP_CPP_ORBIT) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "FGWin32 - Win32 Debug"

DEP_CPP_ORBIT=\
	"..\..\devstudio\vc\include\gl\glut.h"\
	"..\src\flight\flight.h"\
	"..\src\flight\larcsim\ls_interface.h"\
	"..\src\flight\slew\slew.h"\
	"..\src\include\general.h"\
	"..\src\scenery\orbits.h"\
	"..\src\time\fg_time.h"\
	

"$(INTDIR)\orbits.obj" : $(SOURCE) $(DEP_CPP_ORBIT) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\Src\Scenery\planets.c

!IF  "$(CFG)" == "FGWin32 - Win32 Release"

DEP_CPP_PLANE=\
	"..\..\devstudio\vc\include\gl\glut.h"\
	"..\src\flight\flight.h"\
	"..\src\flight\larcsim\ls_interface.h"\
	"..\src\flight\slew\slew.h"\
	"..\src\scenery\orbits.h"\
	"..\src\scenery\planets.h"\
	"..\src\scenery\sun.h"\
	"..\src\time\fg_time.h"\
	

"$(INTDIR)\planets.obj" : $(SOURCE) $(DEP_CPP_PLANE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "FGWin32 - Win32 Debug"

DEP_CPP_PLANE=\
	"..\..\devstudio\vc\include\gl\glut.h"\
	"..\src\flight\flight.h"\
	"..\src\flight\larcsim\ls_interface.h"\
	"..\src\flight\slew\slew.h"\
	"..\src\scenery\orbits.h"\
	"..\src\scenery\planets.h"\
	"..\src\scenery\sun.h"\
	"..\src\time\fg_time.h"\
	

"$(INTDIR)\planets.obj" : $(SOURCE) $(DEP_CPP_PLANE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\Src\Math\polar.c

!IF  "$(CFG)" == "FGWin32 - Win32 Release"

DEP_CPP_POLAR=\
	"..\src\include\constants.h"\
	"..\src\math\polar.h"\
	

"$(INTDIR)\polar.obj" : $(SOURCE) $(DEP_CPP_POLAR) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "FGWin32 - Win32 Debug"

DEP_CPP_POLAR=\
	"..\src\include\constants.h"\
	"..\src\math\polar.h"\
	

"$(INTDIR)\polar.obj" : $(SOURCE) $(DEP_CPP_POLAR) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\Src\Scenery\scenery.c

!IF  "$(CFG)" == "FGWin32 - Win32 Release"

DEP_CPP_SCENE=\
	"..\..\devstudio\vc\include\gl\glut.h"\
	"..\src\include\general.h"\
	"..\src\scenery\astro.h"\
	"..\src\scenery\obj.h"\
	"..\src\scenery\scenery.h"\
	"..\src\scenery\stars.h"\
	"..\src\xgl\xgl.h"\
	

"$(INTDIR)\scenery.obj" : $(SOURCE) $(DEP_CPP_SCENE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "FGWin32 - Win32 Debug"

DEP_CPP_SCENE=\
	"..\..\devstudio\vc\include\gl\glut.h"\
	"..\src\include\general.h"\
	"..\src\scenery\astro.h"\
	"..\src\scenery\obj.h"\
	"..\src\scenery\scenery.h"\
	"..\src\scenery\stars.h"\
	"..\src\xgl\xgl.h"\
	

"$(INTDIR)\scenery.obj" : $(SOURCE) $(DEP_CPP_SCENE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\Src\Scenery\sky.c

!IF  "$(CFG)" == "FGWin32 - Win32 Release"

DEP_CPP_SKY_C=\
	"..\..\devstudio\vc\include\gl\glut.h"\
	"..\src\aircraft\aircraft.h"\
	"..\src\controls\controls.h"\
	"..\src\flight\flight.h"\
	"..\src\flight\larcsim\ls_interface.h"\
	"..\src\flight\slew\slew.h"\
	"..\src\include\constants.h"\
	"..\src\main\views.h"\
	"..\src\math\fg_random.h"\
	"..\src\math\mat3.h"\
	"..\src\scenery\sky.h"\
	"..\src\time\event.h"\
	"..\src\time\fg_time.h"\
	"..\src\xgl\xgl.h"\
	

"$(INTDIR)\sky.obj" : $(SOURCE) $(DEP_CPP_SKY_C) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "FGWin32 - Win32 Debug"

DEP_CPP_SKY_C=\
	"..\..\devstudio\vc\include\gl\glut.h"\
	"..\src\aircraft\aircraft.h"\
	"..\src\controls\controls.h"\
	"..\src\flight\flight.h"\
	"..\src\flight\larcsim\ls_interface.h"\
	"..\src\flight\slew\slew.h"\
	"..\src\include\constants.h"\
	"..\src\main\views.h"\
	"..\src\math\fg_random.h"\
	"..\src\math\mat3.h"\
	"..\src\scenery\sky.h"\
	"..\src\time\event.h"\
	"..\src\time\fg_time.h"\
	"..\src\xgl\xgl.h"\
	

"$(INTDIR)\sky.obj" : $(SOURCE) $(DEP_CPP_SKY_C) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\Src\Flight\Slew\slew.c

!IF  "$(CFG)" == "FGWin32 - Win32 Release"

DEP_CPP_SLEW_=\
	"..\src\aircraft\aircraft.h"\
	"..\src\controls\controls.h"\
	"..\src\flight\flight.h"\
	"..\src\flight\larcsim\ls_interface.h"\
	"..\src\flight\slew\slew.h"\
	"..\src\include\constants.h"\
	

"$(INTDIR)\slew.obj" : $(SOURCE) $(DEP_CPP_SLEW_) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "FGWin32 - Win32 Debug"

DEP_CPP_SLEW_=\
	"..\src\aircraft\aircraft.h"\
	"..\src\controls\controls.h"\
	"..\src\flight\flight.h"\
	"..\src\flight\larcsim\ls_interface.h"\
	"..\src\flight\slew\slew.h"\
	"..\src\include\constants.h"\
	

"$(INTDIR)\slew.obj" : $(SOURCE) $(DEP_CPP_SLEW_) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\Src\Scenery\stars.c

!IF  "$(CFG)" == "FGWin32 - Win32 Release"

DEP_CPP_STARS=\
	"..\..\devstudio\vc\include\gl\glut.h"\
	"..\src\aircraft\aircraft.h"\
	"..\src\controls\controls.h"\
	"..\src\flight\flight.h"\
	"..\src\flight\larcsim\ls_interface.h"\
	"..\src\flight\slew\slew.h"\
	"..\src\include\constants.h"\
	"..\src\include\general.h"\
	"..\src\main\views.h"\
	"..\src\math\mat3.h"\
	"..\src\scenery\orbits.h"\
	"..\src\scenery\planets.h"\
	"..\src\scenery\stars.h"\
	"..\src\time\fg_time.h"\
	"..\src\xgl\xgl.h"\
	

"$(INTDIR)\stars.obj" : $(SOURCE) $(DEP_CPP_STARS) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "FGWin32 - Win32 Debug"

DEP_CPP_STARS=\
	"..\..\devstudio\vc\include\gl\glut.h"\
	"..\src\aircraft\aircraft.h"\
	"..\src\controls\controls.h"\
	"..\src\flight\flight.h"\
	"..\src\flight\larcsim\ls_interface.h"\
	"..\src\flight\slew\slew.h"\
	"..\src\include\constants.h"\
	"..\src\include\general.h"\
	"..\src\main\views.h"\
	"..\src\math\mat3.h"\
	"..\src\scenery\orbits.h"\
	"..\src\scenery\planets.h"\
	"..\src\scenery\stars.h"\
	"..\src\time\fg_time.h"\
	"..\src\xgl\xgl.h"\
	

"$(INTDIR)\stars.obj" : $(SOURCE) $(DEP_CPP_STARS) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\Src\Scenery\sun.c

!IF  "$(CFG)" == "FGWin32 - Win32 Release"

DEP_CPP_SUN_C=\
	"..\..\devstudio\vc\include\gl\glut.h"\
	"..\src\flight\flight.h"\
	"..\src\flight\larcsim\ls_interface.h"\
	"..\src\flight\slew\slew.h"\
	"..\src\main\views.h"\
	"..\src\math\mat3.h"\
	"..\src\scenery\orbits.h"\
	"..\src\scenery\sun.h"\
	"..\src\time\fg_time.h"\
	"..\src\xgl\xgl.h"\
	

"$(INTDIR)\sun.obj" : $(SOURCE) $(DEP_CPP_SUN_C) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "FGWin32 - Win32 Debug"

DEP_CPP_SUN_C=\
	"..\..\devstudio\vc\include\gl\glut.h"\
	"..\src\flight\flight.h"\
	"..\src\flight\larcsim\ls_interface.h"\
	"..\src\flight\slew\slew.h"\
	"..\src\main\views.h"\
	"..\src\math\mat3.h"\
	"..\src\scenery\orbits.h"\
	"..\src\scenery\sun.h"\
	"..\src\time\fg_time.h"\
	"..\src\xgl\xgl.h"\
	

"$(INTDIR)\sun.obj" : $(SOURCE) $(DEP_CPP_SUN_C) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\Src\Time\sunpos.c

!IF  "$(CFG)" == "FGWin32 - Win32 Release"

DEP_CPP_SUNPO=\
	"..\..\devstudio\vc\include\gl\glut.h"\
	"..\src\flight\flight.h"\
	"..\src\flight\larcsim\ls_interface.h"\
	"..\src\flight\slew\slew.h"\
	"..\src\include\constants.h"\
	"..\src\main\views.h"\
	"..\src\math\fg_geodesy.h"\
	"..\src\math\mat3.h"\
	"..\src\math\polar.h"\
	"..\src\time\fg_time.h"\
	"..\src\time\sunpos.h"\
	

"$(INTDIR)\sunpos.obj" : $(SOURCE) $(DEP_CPP_SUNPO) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "FGWin32 - Win32 Debug"

DEP_CPP_SUNPO=\
	"..\..\devstudio\vc\include\gl\glut.h"\
	"..\src\flight\flight.h"\
	"..\src\flight\larcsim\ls_interface.h"\
	"..\src\flight\slew\slew.h"\
	"..\src\include\constants.h"\
	"..\src\main\views.h"\
	"..\src\math\fg_geodesy.h"\
	"..\src\math\mat3.h"\
	"..\src\math\polar.h"\
	"..\src\time\fg_time.h"\
	"..\src\time\sunpos.h"\
	

"$(INTDIR)\sunpos.obj" : $(SOURCE) $(DEP_CPP_SUNPO) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\Src\Math\vector.c
DEP_CPP_VECTO=\
	"..\src\math\mat3.h"\
	"..\src\math\vector.h"\
	

"$(INTDIR)\vector.obj" : $(SOURCE) $(DEP_CPP_VECTO) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\Src\Main\views.c

!IF  "$(CFG)" == "FGWin32 - Win32 Release"

DEP_CPP_VIEWS=\
	"..\..\devstudio\vc\include\gl\glut.h"\
	"..\src\flight\flight.h"\
	"..\src\flight\larcsim\ls_interface.h"\
	"..\src\flight\slew\slew.h"\
	"..\src\include\constants.h"\
	"..\src\main\views.h"\
	"..\src\math\mat3.h"\
	"..\src\math\polar.h"\
	"..\src\math\vector.h"\
	"..\src\scenery\scenery.h"\
	"..\src\time\fg_time.h"\
	

"$(INTDIR)\views.obj" : $(SOURCE) $(DEP_CPP_VIEWS) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "FGWin32 - Win32 Debug"

DEP_CPP_VIEWS=\
	"..\..\devstudio\vc\include\gl\glut.h"\
	"..\src\flight\flight.h"\
	"..\src\flight\larcsim\ls_interface.h"\
	"..\src\flight\slew\slew.h"\
	"..\src\include\constants.h"\
	"..\src\main\views.h"\
	"..\src\math\mat3.h"\
	"..\src\math\polar.h"\
	"..\src\math\vector.h"\
	"..\src\scenery\scenery.h"\
	"..\src\time\fg_time.h"\
	

"$(INTDIR)\views.obj" : $(SOURCE) $(DEP_CPP_VIEWS) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\Src\Weather\weather.c

!IF  "$(CFG)" == "FGWin32 - Win32 Release"

DEP_CPP_WEATH=\
	"..\src\aircraft\aircraft.h"\
	"..\src\controls\controls.h"\
	"..\src\flight\flight.h"\
	"..\src\flight\larcsim\ls_interface.h"\
	"..\src\flight\slew\slew.h"\
	"..\src\math\fg_random.h"\
	"..\src\weather\weather.h"\
	

"$(INTDIR)\weather.obj" : $(SOURCE) $(DEP_CPP_WEATH) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "FGWin32 - Win32 Debug"

DEP_CPP_WEATH=\
	"..\src\aircraft\aircraft.h"\
	"..\src\controls\controls.h"\
	"..\src\flight\flight.h"\
	"..\src\flight\larcsim\ls_interface.h"\
	"..\src\flight\slew\slew.h"\
	"..\src\math\fg_random.h"\
	"..\src\weather\weather.h"\
	

"$(INTDIR)\weather.obj" : $(SOURCE) $(DEP_CPP_WEATH) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


!ENDIF 

