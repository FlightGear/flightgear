REM @ECHO OFF

REM Skip ahead to CONT1 if FG_ROOT has a value
IF NOT %FG_ROOT%.==. GOTO CONT1

SET FG_ROOT=.

:CONT1

REM Check for the existance of the executable
IF NOT EXIST %FG_ROOT%\BIN\FGFS.EXE GOTO ERROR1

REM Now that FG_ROOT has been set, run the program
ECHO FG_ROOT = %FG_ROOT%
%FG_ROOT%\BIN\FGFS.EXE %1 %2 %3 %4 %5 %6 %7 %8 %9

GOTO END

:ERROR1
ECHO Cannot find %FG_ROOT%\BIN\FGFS.EXE
GOTO END

:END
