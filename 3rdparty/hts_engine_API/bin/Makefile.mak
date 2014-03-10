
CC = cl
CL = link

CFLAGS = /O2 /Ob2 /Oi /Ot /Oy /GT /GL /TC /I ..\include
LFLAGS = /LTCG

LIBS = ..\lib\hts_engine_API.lib winmm.lib

all: hts_engine.exe

hts_engine.exe : hts_engine.obj
	$(CC) $(CFLAGS) /c $(@B).c
	$(CL) $(LFLAGS) /OUT:$@ $(LIBS) $(@B).obj

clean:	
	del *.exe
	del *.obj
