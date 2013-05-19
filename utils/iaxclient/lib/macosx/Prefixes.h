
#define MACOSX 
#define LIBIAX 
//#define IAXC_VIDEO


//#define CODEC_ILBC 0
#define SPEEX_PREPROCESS 1
//#define SPAN_EC 0
#define SPEEX_EC 1
//#define MEC2_EC 0
#define USE_NEWJB 1
//#define USE_VIDEO 0



// ***************************************************************************
//		BEGIN CONFIGURABLE VARIABLES SECTION
// ***************************************************************************

// Choose wheter to compile iaxclient with ffmpeg library support or not
#define USE_FFMPEG 0
// Please fill in the path to ffmpeg's libavcodec source dir
//FFMPEG_SRCDIR=

// Warning: The use of AMR codec can change iaxclient library licensing 
//
// Please fill in the path to 3GPP TS26.104 source code ONLY IF you 
// don't wants to use ffmpeg library. If you wants to use AMR narrow
// band and wide band please instal it under the same subdirectory
//AMR_CODEC_SRCDIR=
#define USE_AMR 0
#define USE_AMR_WB 0

// Warning: The use of AMR codec included in ffmpeg library can change ffmpeg 
// library licensing and iaxclient library licensing. Use only if you know 
// what are you doing
//
// Use this only if you have installed 3GPP TS26.104 source codes following
// ffmpeg inscrutions
#define USE_FF_AMR 0
#define USE_FF_AMR_WB 0


// Mats hack for MacOSX 10.2.8; see jitterbuf.c
#define HAVE_NOT_VA_ARGS
#define HAVE_NOT_RESTRICT_KEYWORD

