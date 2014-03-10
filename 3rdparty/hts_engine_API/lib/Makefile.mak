
CC = cl

CFLAGS = /O2 /Ob2 /Oi /Ot /Oy /GT /GL /TC /I ..\include
LFLAGS = /LTCG

CORES = HTS_audio.obj HTS_engine.obj HTS_gstream.obj HTS_label.obj HTS_misc.obj HTS_model.obj HTS_pstream.obj HTS_sstream.obj HTS_vocoder.obj

all: hts_engine_API.lib

hts_engine_API.lib: $(CORES)
	lib $(LFLAGS) /OUT:$@ $(CORES)

.c.obj:
	$(CC) $(CFLAGS) /c $<

clean:
	del *.lib
	del *.obj
