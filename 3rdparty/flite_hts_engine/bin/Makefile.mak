
CC		= cl
CL		= link

CFLAGS = /I C:\hts_engine_API\include /I ..\include /I ..\flite\include \
         /I ..\flite\lang\cmu_us_kal /I ..\flite\lang\cmulex /I ..\flite\lang\usenglish \
         /D "FLITE_PLUS_HTS_ENGINE" /D "NO_UNION_INITIALIZATION" \
         /O2 /Ob2 /Oi /Ot /Oy /GT /TC /GL
LFLAGS = /LTCG

LIBS = C:\hts_engine_API\lib\hts_engine_API.lib ..\lib\flite_hts_engine.lib winmm.lib
       
all: flite_hts_engine.exe

flite_hts_engine.exe : flite_hts_engine.obj
	$(CC) $(CFLAGS) /c $(@B).c
	$(CL) $(LFLAGS) /OUT:$@ $(LIBS) $(@B).obj

clean:		     
	del *.exe
	del *.obj
