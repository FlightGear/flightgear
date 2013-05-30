# Autoconf etc. macros
AUTOMAKE_OPTIONS = subdir-objects

# Targets
lib_LTLIBRARIES = libiaxclient.la
libiaxclient_la_SOURCES = $(SRCS)
nodist_libiaxclient_la_SOURCES = $(NODIST_SRCS)

AM_CPPFLAGS =					\
	-I$(srcdir)/portmixer/px_common

AM_CFLAGS =					\
	$(PTHREAD_CFLAGS)			\
	$(SPEEX_CFLAGS)				\
	$(SPEEXDSP_CFLAGS)			\
	$(IAX2_CFLAGS)				\
	$(PORTAUDIO_CFLAGS)

AM_CFLAGS += -Wall
AM_CFLAGS += -Wextra
AM_CFLAGS += -Wno-unused-parameter
AM_CFLAGS += -Wpointer-arith
AM_CFLAGS += -DLIBVER='"$(PACKAGE_VERSION)"'

libiaxclient_la_LIBADD =			\
	$(PTHREAD_LIBS)				\
	$(IAX2_LIBS)				\
	$(PORTAUDIO_LIBS)			\
	$(SPEEX_LIBS)				\
	$(SPEEXDSP_LIBS)

AM_LDFLAGS =					\
	-no-undefined				\
	-version-info @IAXC_LT_CURRENT@:@IAXC_LT_REVISION@:@IAXC_LT_AGE@ \
	-export-symbols-regex "iaxc_.*"

include_HEADERS=iaxclient.h

SRCS = 
NODIST_SRCS =

if WIN32
SRCS += $(SRCS_WIN32)
libiaxclient_la_LIBADD += -lwinmm -lwsock32
endif

if LINUX
SRCS += $(SRCS_LINUX)
endif

if MACOSX
SRCS += $(SRCS_MACOSX)
AM_LDFLAGS += -framework QuickTime
endif

if VIDEO
SRCS += $(SRCS_VIDEO)
if VIDCAP
libiaxclient_la_LIBADD += $(VIDCAP_LIBS)
AM_CFLAGS += $(VIDCAP_CFLAGS)
endif
endif

if USE_LOCAL_GSM
SRCS += $(SRCS_LIBGSM)
endif

if USE_LOCAL_IAX2
SRCS += $(SRCS_IAX2)
if USE_DEBUG_IAX2
AM_CFLAGS += -DDEBUG_SUPPORT 
endif USE_DEBUG_IAX2
endif USE_LOCAL_IAX2

if USE_LOCAL_ILBC
SRCS += codec_ilbc.c
NODIST_SRCS += $(SRCS_ILBC)
endif USE_LOCAL_ILBC

if USE_CODEC_GSM
SRCS += codec_gsm.c codec_gsm.h
libiaxclient_la_LIBADD += $(GSM_LIBS)
AM_CFLAGS += $(GSM_CFLAGS)
endif

if OGG
libiaxclient_la_LIBADD += $(OGG_LIBS)
AM_CFLAGS += $(OGG_CFLAGS)
endif

if THEORA
SRCS += codec_theora.c codec_theora.h
libiaxclient_la_LIBADD += $(THEORA_LIBS)
AM_CFLAGS += $(THEORA_CFLAGS)
endif

if FFMPEG
SRCS += codec_ffmpeg.c codec_ffmpeg.h
libiaxclient_la_LIBADD += $(FFMPEG_LIBS)
AM_CFLAGS += $(FFMPEG_CFLAGS)
endif

SRCS_LIBGSM=					\
	gsm/src/add.c				\
	gsm/src/code.c				\
	gsm/src/debug.c				\
	gsm/src/decode.c			\
	gsm/src/gsm_create.c			\
	gsm/src/gsm_decode.c			\
	gsm/src/gsm_destroy.c			\
	gsm/src/gsm_encode.c			\
	gsm/src/gsm_explode.c			\
	gsm/src/gsm_implode.c			\
	gsm/src/gsm_option.c			\
	gsm/src/gsm_print.c			\
	gsm/src/long_term.c			\
	gsm/src/lpc.c				\
	gsm/src/preprocess.c			\
	gsm/src/rpe.c				\
	gsm/src/short_term.c			\
	gsm/src/table.c				\
	gsm/inc/private.h			\
	gsm/inc/gsm.h				\
	gsm/inc/unproto.h			\
	gsm/inc/config.h			\
	gsm/inc/proto.h

SRCS +=						\
	audio_encode.c				\
	audio_encode.h				\
	audio_portaudio.c			\
	audio_portaudio.h			\
	codec_alaw.c				\
	codec_alaw.h				\
	codec_ilbc.h				\
	codec_speex.c				\
	codec_speex.h				\
	codec_ulaw.c				\
	codec_ulaw.h				\
	iaxclient_lib.c				\
	iaxclient_lib.h				\
	ringbuffer.c				\
	ringbuffer.h				\
	portmixer/px_common/portmixer.h		\
	slice.c					\
	slice.h					\
	spandsp/plc.c				\
	spandsp/plc.h

SRCS_VIDEO=					\
	video.c					\
	video.h

SRCS_ILBC=					\
	iLBC/anaFilter.c			\
	iLBC/iCBSearch.c			\
	iLBC/packing.c				\
	iLBC/constants.c			\
	iLBC/gainquant.c			\
	iLBC/iLBC_decode.c			\
	iLBC/StateConstructW.c			\
	iLBC/createCB.c				\
	iLBC/getCBvec.c				\
	iLBC/iLBC_encode.c			\
	iLBC/StateSearchW.c			\
	iLBC/doCPLC.c				\
	iLBC/helpfun.c				\
	iLBC/syntFilter.c			\
	iLBC/enhancer.c				\
	iLBC/hpInput.c				\
	iLBC/LPCdecode.c			\
	iLBC/filter.c				\
	iLBC/hpOutput.c				\
	iLBC/LPCencode.c			\
	iLBC/FrameClassify.c			\
	iLBC/iCBConstruct.c			\
	iLBC/lsf.c

SRCS_WIN32=					\
	winfuncs.c				\
	portmixer/px_win_wmme/px_win_wmme.c

SRCS_LINUX=					\
	portmixer/px_unix_oss/px_unix_oss.c	\
	unixfuncs.c

SRCS_MACOSX=					\
	portmixer/px_mac_core/px_mac_core.c	\
	unixfuncs.c

SRCS_IAX2=					\
	libiax2/src/md5.c			\
	libiax2/src/iax.c			\
	libiax2/src/iax2-parser.c		\
	libiax2/src/jitterbuf.c			\
	libiax2/src/frame.h			\
	libiax2/src/iax2.h			\
	libiax2/src/iax2-parser.h		\
	libiax2/src/iax-client.h		\
	libiax2/src/jitterbuf.h			\
	libiax2/src/md5.h

EXTRA_DIST = \
	audio_alsa.c				\
	audio_alsa.h				\
	codec_ffmpeg.c				\
	codec_ffmpeg.h				\
	sound2c.pl				\
	gsm					\
	libiax2					\
	portmixer				\
	sox					\
	spandsp					\
	wince

