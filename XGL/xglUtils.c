
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#if !defined( __CYGWIN__ ) && !defined( __CYGWIN32__ )
#  include <malloc.h>
#endif

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#include <string.h>

#include "xgl.h"

#ifndef TRUE
#define TRUE  1
#define FALSE 0
#endif

int   xglTraceOn = TRUE ;

#ifndef WIN32
    FILE *xglTraceFd = stdout ;
#else /* WIN32 */
    /* Bail for now, we just want it to compile I guess */
    FILE *xglTraceFd = NULL;
#endif /* WIN32 */

struct GLenumLookup
{
  GLenum val ;
  char *name ;
} ;

static struct GLenumLookup glenum_string [] =
{
/*
  Due to ambiguity - these are not in the table...

    GL_NONE = , GL_ZERO = GL_FALSE = GL_POINTS = 0
    GL_ONE  = , GL_TRUE = GL_LINES = 1
*/
  { GL_2D			,"GL_2D" },
  { GL_2_BYTES			,"GL_2_BYTES" },
  { GL_3D			,"GL_3D" },
  { GL_3D_COLOR			,"GL_3D_COLOR" },
  { GL_3D_COLOR_TEXTURE		,"GL_3D_COLOR_TEXTURE" },
  { GL_3_BYTES			,"GL_3_BYTES" },
  { GL_4D_COLOR_TEXTURE		,"GL_4D_COLOR_TEXTURE" },
  { GL_4_BYTES			,"GL_4_BYTES" },
  { GL_ACCUM			,"GL_ACCUM" },
  { GL_ACCUM_ALPHA_BITS		,"GL_ACCUM_ALPHA_BITS" },
  { GL_ACCUM_BLUE_BITS		,"GL_ACCUM_BLUE_BITS" },
  { GL_ACCUM_CLEAR_VALUE	,"GL_ACCUM_CLEAR_VALUE" },
  { GL_ACCUM_GREEN_BITS		,"GL_ACCUM_GREEN_BITS" },
  { GL_ACCUM_RED_BITS		,"GL_ACCUM_RED_BITS" },
  { GL_ADD			,"GL_ADD" },
  { GL_ALPHA			,"GL_ALPHA" },
  { GL_ALPHA_BIAS		,"GL_ALPHA_BIAS" },
  { GL_ALPHA_BITS		,"GL_ALPHA_BITS" },
  { GL_ALPHA_SCALE		,"GL_ALPHA_SCALE" },
  { GL_ALPHA_TEST		,"GL_ALPHA_TEST" },
  { GL_ALPHA_TEST_FUNC		,"GL_ALPHA_TEST_FUNC" },
  { GL_ALPHA_TEST_REF		,"GL_ALPHA_TEST_REF" },
  { GL_ALWAYS			,"GL_ALWAYS" },
  { GL_AMBIENT			,"GL_AMBIENT" },
  { GL_AMBIENT_AND_DIFFUSE	,"GL_AMBIENT_AND_DIFFUSE" },
  { GL_AND			,"GL_AND" },
  { GL_AND_INVERTED		,"GL_AND_INVERTED" },
  { GL_AND_REVERSE		,"GL_AND_REVERSE" },
  { GL_ATTRIB_STACK_DEPTH	,"GL_ATTRIB_STACK_DEPTH" },
  { GL_AUTO_NORMAL		,"GL_AUTO_NORMAL" },
  { GL_AUX0			,"GL_AUX0" },
  { GL_AUX1			,"GL_AUX1" },
  { GL_AUX2			,"GL_AUX2" },
  { GL_AUX3			,"GL_AUX3" },
  { GL_AUX_BUFFERS		,"GL_AUX_BUFFERS" },
  { GL_BACK			,"GL_BACK" },
  { GL_BACK_LEFT		,"GL_BACK_LEFT" },
  { GL_BACK_RIGHT		,"GL_BACK_RIGHT" },
  { GL_BITMAP			,"GL_BITMAP" },
  { GL_BITMAP_TOKEN		,"GL_BITMAP_TOKEN" },
  { GL_BLEND			,"GL_BLEND" },
  { GL_BLEND_DST		,"GL_BLEND_DST" },
#ifdef GL_BLEND_COLOR_EXT
  { GL_BLEND_COLOR_EXT		,"GL_BLEND_COLOR_EXT" },
#endif
#ifdef GL_BLEND_EQUATION_EXT
  { GL_BLEND_EQUATION_EXT	,"GL_BLEND_EQUATION_EXT" },
#endif
  { GL_BLEND_SRC		,"GL_BLEND_SRC" },
  { GL_BLUE			,"GL_BLUE" },
  { GL_BLUE_BIAS		,"GL_BLUE_BIAS" },
  { GL_BLUE_BITS		,"GL_BLUE_BITS" },
  { GL_BLUE_SCALE		,"GL_BLUE_SCALE" },
  { GL_BYTE			,"GL_BYTE" },
  { GL_CCW			,"GL_CCW" },
  { GL_CLAMP			,"GL_CLAMP" },
  { GL_CLEAR			,"GL_CLEAR" },
  { GL_CLIP_PLANE0		,"GL_CLIP_PLANE0" },
  { GL_CLIP_PLANE1		,"GL_CLIP_PLANE1" },
  { GL_CLIP_PLANE2		,"GL_CLIP_PLANE2" },
  { GL_CLIP_PLANE3		,"GL_CLIP_PLANE3" },
  { GL_CLIP_PLANE4		,"GL_CLIP_PLANE4" },
  { GL_CLIP_PLANE5		,"GL_CLIP_PLANE5" },
  { GL_COEFF			,"GL_COEFF" },
  { GL_COLOR			,"GL_COLOR" },
#ifdef GL_COLOR_ARRAY_EXT
  { GL_COLOR_ARRAY_COUNT_EXT	,"GL_COLOR_ARRAY_COUNT_EXT" },
  { GL_COLOR_ARRAY_EXT		,"GL_COLOR_ARRAY_EXT" },
  { GL_COLOR_ARRAY_POINTER_EXT	,"GL_COLOR_ARRAY_POINTER_EXT" },
  { GL_COLOR_ARRAY_SIZE_EXT	,"GL_COLOR_ARRAY_SIZE_EXT" },
  { GL_COLOR_ARRAY_STRIDE_EXT	,"GL_COLOR_ARRAY_STRIDE_EXT" },
  { GL_COLOR_ARRAY_TYPE_EXT	,"GL_COLOR_ARRAY_TYPE_EXT" },
#endif
  { GL_COLOR_CLEAR_VALUE	,"GL_COLOR_CLEAR_VALUE" },
  { GL_COLOR_INDEX		,"GL_COLOR_INDEX" },
  { GL_COLOR_INDEXES		,"GL_COLOR_INDEXES" },
  { GL_COLOR_MATERIAL		,"GL_COLOR_MATERIAL" },
  { GL_COLOR_MATERIAL_FACE	,"GL_COLOR_MATERIAL_FACE" },
  { GL_COLOR_MATERIAL_PARAMETER	,"GL_COLOR_MATERIAL_PARAMETER" },
  { GL_COLOR_WRITEMASK		,"GL_COLOR_WRITEMASK" },
  { GL_COMPILE			,"GL_COMPILE" },
  { GL_COMPILE_AND_EXECUTE	,"GL_COMPILE_AND_EXECUTE" },
#ifdef GL_CONSTANT_ALPHA_EXT
  { GL_CONSTANT_ALPHA_EXT	,"GL_CONSTANT_ALPHA_EXT" },
#endif
  { GL_CONSTANT_ATTENUATION	,"GL_CONSTANT_ATTENUATION" },
#ifdef GL_CONSTANT_COLOR_EXT
  { GL_CONSTANT_COLOR_EXT	,"GL_CONSTANT_COLOR_EXT" },
#endif
  { GL_COPY			,"GL_COPY" },
  { GL_COPY_INVERTED		,"GL_COPY_INVERTED" },
  { GL_COPY_PIXEL_TOKEN		,"GL_COPY_PIXEL_TOKEN" },
  { GL_CULL_FACE		,"GL_CULL_FACE" },
  { GL_CULL_FACE_MODE		,"GL_CULL_FACE_MODE" },
  { GL_CURRENT_COLOR		,"GL_CURRENT_COLOR" },
  { GL_CURRENT_INDEX		,"GL_CURRENT_INDEX" },
  { GL_CURRENT_NORMAL		,"GL_CURRENT_NORMAL" },
  { GL_CURRENT_RASTER_COLOR	,"GL_CURRENT_RASTER_COLOR" },
  { GL_CURRENT_RASTER_DISTANCE	,"GL_CURRENT_RASTER_DISTANCE" },
  { GL_CURRENT_RASTER_INDEX	,"GL_CURRENT_RASTER_INDEX" },
  { GL_CURRENT_RASTER_POSITION	,"GL_CURRENT_RASTER_POSITION" },
  { GL_CURRENT_RASTER_POSITION_VALID,"GL_CURRENT_RASTER_POSITION_VALID" },
  { GL_CURRENT_RASTER_TEXTURE_COORDS,"GL_CURRENT_RASTER_TEXTURE_COORDS" },
  { GL_CURRENT_TEXTURE_COORDS	,"GL_CURRENT_TEXTURE_COORDS" },
  { GL_CW			,"GL_CW" },
  { GL_DECAL			,"GL_DECAL" },
  { GL_DECR			,"GL_DECR" },
  { GL_DEPTH			,"GL_DEPTH" },
  { GL_DEPTH_BIAS		,"GL_DEPTH_BIAS" },
  { GL_DEPTH_BITS		,"GL_DEPTH_BITS" },
  { GL_DEPTH_CLEAR_VALUE	,"GL_DEPTH_CLEAR_VALUE" },
  { GL_DEPTH_COMPONENT		,"GL_DEPTH_COMPONENT" },
  { GL_DEPTH_FUNC		,"GL_DEPTH_FUNC" },
  { GL_DEPTH_RANGE		,"GL_DEPTH_RANGE" },
  { GL_DEPTH_SCALE		,"GL_DEPTH_SCALE" },
  { GL_DEPTH_TEST		,"GL_DEPTH_TEST" },
  { GL_DEPTH_WRITEMASK		,"GL_DEPTH_WRITEMASK" },
  { GL_DIFFUSE			,"GL_DIFFUSE" },
  { GL_DITHER			,"GL_DITHER" },
  { GL_DOMAIN			,"GL_DOMAIN" },
  { GL_DONT_CARE		,"GL_DONT_CARE" },
  { GL_DOUBLEBUFFER		,"GL_DOUBLEBUFFER" },
#ifdef GL_DOUBLE_EXT
  { GL_DOUBLE_EXT		,"GL_DOUBLE_EXT" },
#endif
  { GL_DRAW_BUFFER		,"GL_DRAW_BUFFER" },
  { GL_DRAW_PIXEL_TOKEN		,"GL_DRAW_PIXEL_TOKEN" },
  { GL_DST_ALPHA		,"GL_DST_ALPHA" },
  { GL_DST_COLOR		,"GL_DST_COLOR" },
  { GL_EDGE_FLAG		,"GL_EDGE_FLAG" },
#ifdef GL_EDGE_FLAG_ARRAY_EXT
  { GL_EDGE_FLAG_ARRAY_COUNT_EXT,"GL_EDGE_FLAG_ARRAY_COUNT_EXT" },
  { GL_EDGE_FLAG_ARRAY_EXT	,"GL_EDGE_FLAG_ARRAY_EXT" },
  { GL_EDGE_FLAG_ARRAY_POINTER_EXT,"GL_EDGE_FLAG_ARRAY_POINTER_EXT" },
  { GL_EDGE_FLAG_ARRAY_STRIDE_EXT,"GL_EDGE_FLAG_ARRAY_STRIDE_EXT" },
#endif
  { GL_EMISSION			,"GL_EMISSION" },
  { GL_EQUAL			,"GL_EQUAL" },
  { GL_EQUIV			,"GL_EQUIV" },
  { GL_EXP			,"GL_EXP" },
  { GL_EXP2			,"GL_EXP2" },
  { GL_EXTENSIONS		,"GL_EXTENSIONS" },
  { GL_EYE_LINEAR		,"GL_EYE_LINEAR" },
  { GL_EYE_PLANE		,"GL_EYE_PLANE" },
  { GL_FASTEST			,"GL_FASTEST" },
  { GL_FEEDBACK			,"GL_FEEDBACK" },
  { GL_FILL			,"GL_FILL" },
  { GL_FLAT			,"GL_FLAT" },
  { GL_FLOAT			,"GL_FLOAT" },
  { GL_FOG			,"GL_FOG" },
  { GL_FOG_COLOR		,"GL_FOG_COLOR" },
  { GL_FOG_DENSITY		,"GL_FOG_DENSITY" },
  { GL_FOG_END			,"GL_FOG_END" },
  { GL_FOG_HINT			,"GL_FOG_HINT" },
  { GL_FOG_INDEX		,"GL_FOG_INDEX" },
  { GL_FOG_MODE			,"GL_FOG_MODE" },
  { GL_FOG_START		,"GL_FOG_START" },
  { GL_FRONT			,"GL_FRONT" },
  { GL_FRONT_AND_BACK		,"GL_FRONT_AND_BACK" },
  { GL_FRONT_FACE		,"GL_FRONT_FACE" },
  { GL_FRONT_LEFT		,"GL_FRONT_LEFT" },
  { GL_FRONT_RIGHT		,"GL_FRONT_RIGHT" },
#ifdef GL_FUNC_ADD_EXT
  { GL_FUNC_ADD_EXT		,"GL_FUNC_ADD_EXT" },
  { GL_FUNC_REVERSE_SUBTRACT_EXT,"GL_FUNC_REVERSE_SUBTRACT_EXT" },
  { GL_FUNC_SUBTRACT_EXT	,"GL_FUNC_SUBTRACT_EXT" },
#endif
  { GL_GEQUAL			,"GL_GEQUAL" },
  { GL_GREATER			,"GL_GREATER" },
  { GL_GREEN			,"GL_GREEN" },
  { GL_GREEN_BIAS		,"GL_GREEN_BIAS" },
  { GL_GREEN_BITS		,"GL_GREEN_BITS" },
  { GL_GREEN_SCALE		,"GL_GREEN_SCALE" },
  { GL_INCR			,"GL_INCR" },
#ifdef GL_INDEX_ARRAY_EXT
  { GL_INDEX_ARRAY_COUNT_EXT	,"GL_INDEX_ARRAY_COUNT_EXT" },
  { GL_INDEX_ARRAY_EXT		,"GL_INDEX_ARRAY_EXT" },
  { GL_INDEX_ARRAY_POINTER_EXT	,"GL_INDEX_ARRAY_POINTER_EXT" },
  { GL_INDEX_ARRAY_STRIDE_EXT	,"GL_INDEX_ARRAY_STRIDE_EXT" },
  { GL_INDEX_ARRAY_TYPE_EXT	,"GL_INDEX_ARRAY_TYPE_EXT" },
#endif
  { GL_INDEX_BITS		,"GL_INDEX_BITS" },
  { GL_INDEX_CLEAR_VALUE	,"GL_INDEX_CLEAR_VALUE" },
  { GL_INDEX_MODE		,"GL_INDEX_MODE" },
  { GL_INDEX_OFFSET		,"GL_INDEX_OFFSET" },
  { GL_INDEX_SHIFT		,"GL_INDEX_SHIFT" },
  { GL_INDEX_WRITEMASK		,"GL_INDEX_WRITEMASK" },
  { GL_INT			,"GL_INT" },
  { GL_INVALID_ENUM		,"GL_INVALID_ENUM" },
  { GL_INVALID_OPERATION	,"GL_INVALID_OPERATION" },
  { GL_INVALID_VALUE		,"GL_INVALID_VALUE" },
  { GL_INVERT			,"GL_INVERT" },
  { GL_KEEP			,"GL_KEEP" },
  { GL_LEFT			,"GL_LEFT" },
  { GL_LEQUAL			,"GL_LEQUAL" },
  { GL_LESS			,"GL_LESS" },
  { GL_LIGHT0			,"GL_LIGHT0" },
  { GL_LIGHT1			,"GL_LIGHT1" },
  { GL_LIGHT2			,"GL_LIGHT2" },
  { GL_LIGHT3			,"GL_LIGHT3" },
  { GL_LIGHT4			,"GL_LIGHT4" },
  { GL_LIGHT5			,"GL_LIGHT5" },
  { GL_LIGHT6			,"GL_LIGHT6" },
  { GL_LIGHT7			,"GL_LIGHT7" },
  { GL_LIGHTING			,"GL_LIGHTING" },
  { GL_LIGHT_MODEL_AMBIENT	,"GL_LIGHT_MODEL_AMBIENT" },
  { GL_LIGHT_MODEL_LOCAL_VIEWER	,"GL_LIGHT_MODEL_LOCAL_VIEWER" },
  { GL_LIGHT_MODEL_TWO_SIDE	,"GL_LIGHT_MODEL_TWO_SIDE" },
  { GL_LINE			,"GL_LINE" },
  { GL_LINEAR			,"GL_LINEAR" },
  { GL_LINEAR_ATTENUATION	,"GL_LINEAR_ATTENUATION" },
  { GL_LINEAR_MIPMAP_LINEAR	,"GL_LINEAR_MIPMAP_LINEAR" },
  { GL_LINEAR_MIPMAP_NEAREST	,"GL_LINEAR_MIPMAP_NEAREST" },
  { GL_LINE_LOOP		,"GL_LINE_LOOP" },
  { GL_LINE_RESET_TOKEN		,"GL_LINE_RESET_TOKEN" },
  { GL_LINE_SMOOTH		,"GL_LINE_SMOOTH" },
  { GL_LINE_SMOOTH_HINT		,"GL_LINE_SMOOTH_HINT" },
  { GL_LINE_STIPPLE		,"GL_LINE_STIPPLE" },
  { GL_LINE_STIPPLE_PATTERN	,"GL_LINE_STIPPLE_PATTERN" },
  { GL_LINE_STIPPLE_REPEAT	,"GL_LINE_STIPPLE_REPEAT" },
  { GL_LINE_STRIP		,"GL_LINE_STRIP" },
  { GL_LINE_TOKEN		,"GL_LINE_TOKEN" },
  { GL_LINE_WIDTH		,"GL_LINE_WIDTH" },
  { GL_LINE_WIDTH_GRANULARITY	,"GL_LINE_WIDTH_GRANULARITY" },
  { GL_LINE_WIDTH_RANGE		,"GL_LINE_WIDTH_RANGE" },
  { GL_LIST_BASE		,"GL_LIST_BASE" },
  { GL_LIST_INDEX		,"GL_LIST_INDEX" },
  { GL_LIST_MODE		,"GL_LIST_MODE" },
  { GL_LOAD			,"GL_LOAD" },
  { GL_LOGIC_OP			,"GL_LOGIC_OP" },
  { GL_LOGIC_OP_MODE		,"GL_LOGIC_OP_MODE" },
  { GL_LUMINANCE		,"GL_LUMINANCE" },
  { GL_LUMINANCE_ALPHA		,"GL_LUMINANCE_ALPHA" },
  { GL_MAP1_COLOR_4		,"GL_MAP1_COLOR_4" },
  { GL_MAP1_GRID_DOMAIN		,"GL_MAP1_GRID_DOMAIN" },
  { GL_MAP1_GRID_SEGMENTS	,"GL_MAP1_GRID_SEGMENTS" },
  { GL_MAP1_INDEX		,"GL_MAP1_INDEX" },
  { GL_MAP1_NORMAL		,"GL_MAP1_NORMAL" },
  { GL_MAP1_TEXTURE_COORD_1	,"GL_MAP1_TEXTURE_COORD_1" },
  { GL_MAP1_TEXTURE_COORD_2	,"GL_MAP1_TEXTURE_COORD_2" },
  { GL_MAP1_TEXTURE_COORD_3	,"GL_MAP1_TEXTURE_COORD_3" },
  { GL_MAP1_TEXTURE_COORD_4	,"GL_MAP1_TEXTURE_COORD_4" },
  { GL_MAP1_VERTEX_3		,"GL_MAP1_VERTEX_3" },
  { GL_MAP1_VERTEX_4		,"GL_MAP1_VERTEX_4" },
  { GL_MAP2_COLOR_4		,"GL_MAP2_COLOR_4" },
  { GL_MAP2_GRID_DOMAIN		,"GL_MAP2_GRID_DOMAIN" },
  { GL_MAP2_GRID_SEGMENTS	,"GL_MAP2_GRID_SEGMENTS" },
  { GL_MAP2_INDEX		,"GL_MAP2_INDEX" },
  { GL_MAP2_NORMAL		,"GL_MAP2_NORMAL" },
  { GL_MAP2_TEXTURE_COORD_1	,"GL_MAP2_TEXTURE_COORD_1" },
  { GL_MAP2_TEXTURE_COORD_2	,"GL_MAP2_TEXTURE_COORD_2" },
  { GL_MAP2_TEXTURE_COORD_3	,"GL_MAP2_TEXTURE_COORD_3" },
  { GL_MAP2_TEXTURE_COORD_4	,"GL_MAP2_TEXTURE_COORD_4" },
  { GL_MAP2_VERTEX_3		,"GL_MAP2_VERTEX_3" },
  { GL_MAP2_VERTEX_4		,"GL_MAP2_VERTEX_4" },
  { GL_MAP_COLOR		,"GL_MAP_COLOR" },
  { GL_MAP_STENCIL		,"GL_MAP_STENCIL" },
  { GL_MATRIX_MODE		,"GL_MATRIX_MODE" },
  { GL_MAX_ATTRIB_STACK_DEPTH	,"GL_MAX_ATTRIB_STACK_DEPTH" },
  { GL_MAX_CLIP_PLANES		,"GL_MAX_CLIP_PLANES" },
  { GL_MAX_EVAL_ORDER		,"GL_MAX_EVAL_ORDER" },
#ifdef GL_MAX_EXT
  { GL_MAX_EXT			,"GL_MAX_EXT" },
#endif
  { GL_MAX_LIGHTS		,"GL_MAX_LIGHTS" },
  { GL_MAX_LIST_NESTING		,"GL_MAX_LIST_NESTING" },
  { GL_MAX_MODELVIEW_STACK_DEPTH,"GL_MAX_MODELVIEW_STACK_DEPTH" },
  { GL_MAX_NAME_STACK_DEPTH	,"GL_MAX_NAME_STACK_DEPTH" },
  { GL_MAX_PIXEL_MAP_TABLE	,"GL_MAX_PIXEL_MAP_TABLE" },
  { GL_MAX_PROJECTION_STACK_DEPTH,"GL_MAX_PROJECTION_STACK_DEPTH" },
  { GL_MAX_TEXTURE_SIZE		,"GL_MAX_TEXTURE_SIZE" },
  { GL_MAX_TEXTURE_STACK_DEPTH	,"GL_MAX_TEXTURE_STACK_DEPTH" },
  { GL_MAX_VIEWPORT_DIMS	,"GL_MAX_VIEWPORT_DIMS" },
#ifdef GL_MIN_EXT
  { GL_MIN_EXT			,"GL_MIN_EXT" },
#endif
  { GL_MODELVIEW		,"GL_MODELVIEW" },
  { GL_MODELVIEW_MATRIX		,"GL_MODELVIEW_MATRIX" },
  { GL_MODELVIEW_STACK_DEPTH	,"GL_MODELVIEW_STACK_DEPTH" },
  { GL_MODULATE			,"GL_MODULATE" },
  { GL_MULT			,"GL_MULT" },
  { GL_NAME_STACK_DEPTH		,"GL_NAME_STACK_DEPTH" },
  { GL_NAND			,"GL_NAND" },
  { GL_NEAREST			,"GL_NEAREST" },
  { GL_NEAREST_MIPMAP_LINEAR	,"GL_NEAREST_MIPMAP_LINEAR" },
  { GL_NEAREST_MIPMAP_NEAREST	,"GL_NEAREST_MIPMAP_NEAREST" },
  { GL_NEVER			,"GL_NEVER" },
  { GL_NICEST			,"GL_NICEST" },
  { GL_NOOP			,"GL_NOOP" },
  { GL_NOR			,"GL_NOR" },
  { GL_NORMALIZE		,"GL_NORMALIZE" },
#ifdef GL_NORMAL_ARRAY_EXT
  { GL_NORMAL_ARRAY_COUNT_EXT	,"GL_NORMAL_ARRAY_COUNT_EXT" },
  { GL_NORMAL_ARRAY_EXT		,"GL_NORMAL_ARRAY_EXT" },
  { GL_NORMAL_ARRAY_POINTER_EXT	,"GL_NORMAL_ARRAY_POINTER_EXT" },
  { GL_NORMAL_ARRAY_STRIDE_EXT	,"GL_NORMAL_ARRAY_STRIDE_EXT" },
  { GL_NORMAL_ARRAY_TYPE_EXT	,"GL_NORMAL_ARRAY_TYPE_EXT" },
#endif
  { GL_NOTEQUAL			,"GL_NOTEQUAL" },
  { GL_OBJECT_LINEAR		,"GL_OBJECT_LINEAR" },
  { GL_OBJECT_PLANE		,"GL_OBJECT_PLANE" },
#ifdef GL_ONE_MINUS_CONSTANT_ALPHA_EXT
  { GL_ONE_MINUS_CONSTANT_ALPHA_EXT,"GL_ONE_MINUS_CONSTANT_ALPHA_EXT" },
  { GL_ONE_MINUS_CONSTANT_COLOR_EXT,"GL_ONE_MINUS_CONSTANT_COLOR_EXT" },
#endif
  { GL_ONE_MINUS_DST_ALPHA	,"GL_ONE_MINUS_DST_ALPHA" },
  { GL_ONE_MINUS_DST_COLOR	,"GL_ONE_MINUS_DST_COLOR" },
  { GL_ONE_MINUS_SRC_ALPHA	,"GL_ONE_MINUS_SRC_ALPHA" },
  { GL_ONE_MINUS_SRC_COLOR	,"GL_ONE_MINUS_SRC_COLOR" },
  { GL_OR			,"GL_OR" },
  { GL_ORDER			,"GL_ORDER" },
  { GL_OR_INVERTED		,"GL_OR_INVERTED" },
  { GL_OR_REVERSE		,"GL_OR_REVERSE" },
  { GL_OUT_OF_MEMORY		,"GL_OUT_OF_MEMORY" },
  { GL_PACK_ALIGNMENT		,"GL_PACK_ALIGNMENT" },
  { GL_PACK_LSB_FIRST		,"GL_PACK_LSB_FIRST" },
  { GL_PACK_ROW_LENGTH		,"GL_PACK_ROW_LENGTH" },
  { GL_PACK_SKIP_PIXELS		,"GL_PACK_SKIP_PIXELS" },
  { GL_PACK_SKIP_ROWS		,"GL_PACK_SKIP_ROWS" },
  { GL_PACK_SWAP_BYTES		,"GL_PACK_SWAP_BYTES" },
  { GL_PASS_THROUGH_TOKEN	,"GL_PASS_THROUGH_TOKEN" },
  { GL_PERSPECTIVE_CORRECTION_HINT,"GL_PERSPECTIVE_CORRECTION_HINT" },
  { GL_PIXEL_MAP_A_TO_A		,"GL_PIXEL_MAP_A_TO_A" },
  { GL_PIXEL_MAP_A_TO_A_SIZE	,"GL_PIXEL_MAP_A_TO_A_SIZE" },
  { GL_PIXEL_MAP_B_TO_B		,"GL_PIXEL_MAP_B_TO_B" },
  { GL_PIXEL_MAP_B_TO_B_SIZE	,"GL_PIXEL_MAP_B_TO_B_SIZE" },
  { GL_PIXEL_MAP_G_TO_G		,"GL_PIXEL_MAP_G_TO_G" },
  { GL_PIXEL_MAP_G_TO_G_SIZE	,"GL_PIXEL_MAP_G_TO_G_SIZE" },
  { GL_PIXEL_MAP_I_TO_A		,"GL_PIXEL_MAP_I_TO_A" },
  { GL_PIXEL_MAP_I_TO_A_SIZE	,"GL_PIXEL_MAP_I_TO_A_SIZE" },
  { GL_PIXEL_MAP_I_TO_B		,"GL_PIXEL_MAP_I_TO_B" },
  { GL_PIXEL_MAP_I_TO_B_SIZE	,"GL_PIXEL_MAP_I_TO_B_SIZE" },
  { GL_PIXEL_MAP_I_TO_G		,"GL_PIXEL_MAP_I_TO_G" },
  { GL_PIXEL_MAP_I_TO_G_SIZE	,"GL_PIXEL_MAP_I_TO_G_SIZE" },
  { GL_PIXEL_MAP_I_TO_I		,"GL_PIXEL_MAP_I_TO_I" },
  { GL_PIXEL_MAP_I_TO_I_SIZE	,"GL_PIXEL_MAP_I_TO_I_SIZE" },
  { GL_PIXEL_MAP_I_TO_R		,"GL_PIXEL_MAP_I_TO_R" },
  { GL_PIXEL_MAP_I_TO_R_SIZE	,"GL_PIXEL_MAP_I_TO_R_SIZE" },
  { GL_PIXEL_MAP_R_TO_R		,"GL_PIXEL_MAP_R_TO_R" },
  { GL_PIXEL_MAP_R_TO_R_SIZE	,"GL_PIXEL_MAP_R_TO_R_SIZE" },
  { GL_PIXEL_MAP_S_TO_S		,"GL_PIXEL_MAP_S_TO_S" },
  { GL_PIXEL_MAP_S_TO_S_SIZE	,"GL_PIXEL_MAP_S_TO_S_SIZE" },
  { GL_POINT			,"GL_POINT" },
  { GL_POINT_SIZE		,"GL_POINT_SIZE" },
  { GL_POINT_SIZE_GRANULARITY 	,"GL_POINT_SIZE_GRANULARITY" },
  { GL_POINT_SIZE_RANGE		,"GL_POINT_SIZE_RANGE" },
  { GL_POINT_SMOOTH		,"GL_POINT_SMOOTH" },
  { GL_POINT_SMOOTH_HINT	,"GL_POINT_SMOOTH_HINT" },
  { GL_POINT_TOKEN		,"GL_POINT_TOKEN" },
  { GL_POLYGON			,"GL_POLYGON" },
  { GL_POLYGON_MODE		,"GL_POLYGON_MODE" },
  { GL_POLYGON_SMOOTH		,"GL_POLYGON_SMOOTH" },
  { GL_POLYGON_SMOOTH_HINT	,"GL_POLYGON_SMOOTH_HINT" },
  { GL_POLYGON_STIPPLE		,"GL_POLYGON_STIPPLE" },
#ifdef GL_POLYGON_OFFSET_EXT
  { GL_POLYGON_OFFSET_BIAS_EXT  ,"GL_POLYGON_OFFSET_BIAS_EXT" },
  { GL_POLYGON_OFFSET_EXT       ,"GL_POLYGON_OFFSET_EXT" },
  { GL_POLYGON_OFFSET_FACTOR_EXT,"GL_POLYGON_OFFSET_FACTOR_EXT" },
#endif
  { GL_POLYGON_TOKEN		,"GL_POLYGON_TOKEN" },
  { GL_POSITION			,"GL_POSITION" },
  { GL_PROJECTION		,"GL_PROJECTION" },
  { GL_PROJECTION_MATRIX	,"GL_PROJECTION_MATRIX" },
  { GL_PROJECTION_STACK_DEPTH	,"GL_PROJECTION_STACK_DEPTH" },
  { GL_Q			,"GL_Q" },
  { GL_QUADRATIC_ATTENUATION	,"GL_QUADRATIC_ATTENUATION" },
  { GL_QUADS			,"GL_QUADS" },
  { GL_QUAD_STRIP		,"GL_QUAD_STRIP" },
  { GL_R			,"GL_R" },
  { GL_READ_BUFFER		,"GL_READ_BUFFER" },
  { GL_RED			,"GL_RED" },
  { GL_RED_BIAS			,"GL_RED_BIAS" },
  { GL_RED_BITS			,"GL_RED_BITS" },
  { GL_RED_SCALE		,"GL_RED_SCALE" },
  { GL_RENDER			,"GL_RENDER" },
  { GL_RENDERER			,"GL_RENDERER" },
  { GL_RENDER_MODE		,"GL_RENDER_MODE" },
  { GL_REPEAT			,"GL_REPEAT" },
  { GL_REPLACE			,"GL_REPLACE" },
#ifdef GL_REPLACE_EXT
  { GL_REPLACE_EXT		,"GL_REPLACE_EXT" },
#endif
  { GL_RETURN			,"GL_RETURN" },
  { GL_RGB			,"GL_RGB" },
  { GL_RGBA			,"GL_RGBA" },
  { GL_RGBA_MODE		,"GL_RGBA_MODE" },
  { GL_RIGHT			,"GL_RIGHT" },
  { GL_S			,"GL_S" },
  { GL_SCISSOR_BOX		,"GL_SCISSOR_BOX" },
  { GL_SCISSOR_TEST		,"GL_SCISSOR_TEST" },
  { GL_SELECT			,"GL_SELECT" },
  { GL_SET			,"GL_SET" },
  { GL_SHADE_MODEL		,"GL_SHADE_MODEL" },
  { GL_SHININESS		,"GL_SHININESS" },
  { GL_SHORT			,"GL_SHORT" },
  { GL_SMOOTH			,"GL_SMOOTH" },
  { GL_SPECULAR			,"GL_SPECULAR" },
  { GL_SPHERE_MAP		,"GL_SPHERE_MAP" },
  { GL_SPOT_CUTOFF		,"GL_SPOT_CUTOFF" },
  { GL_SPOT_DIRECTION		,"GL_SPOT_DIRECTION" },
  { GL_SPOT_EXPONENT		,"GL_SPOT_EXPONENT" },
  { GL_SRC_ALPHA		,"GL_SRC_ALPHA" },
  { GL_SRC_ALPHA_SATURATE	,"GL_SRC_ALPHA_SATURATE" },
  { GL_SRC_COLOR		,"GL_SRC_COLOR" },
  { GL_STACK_OVERFLOW		,"GL_STACK_OVERFLOW" },
  { GL_STACK_UNDERFLOW		,"GL_STACK_UNDERFLOW" },
  { GL_STENCIL			,"GL_STENCIL" },
  { GL_STENCIL_BITS		,"GL_STENCIL_BITS" },
  { GL_STENCIL_CLEAR_VALUE	,"GL_STENCIL_CLEAR_VALUE" },
  { GL_STENCIL_FAIL		,"GL_STENCIL_FAIL" },
  { GL_STENCIL_FUNC		,"GL_STENCIL_FUNC" },
  { GL_STENCIL_INDEX		,"GL_STENCIL_INDEX" },
  { GL_STENCIL_PASS_DEPTH_FAIL	,"GL_STENCIL_PASS_DEPTH_FAIL" },
  { GL_STENCIL_PASS_DEPTH_PASS	,"GL_STENCIL_PASS_DEPTH_PASS" },
  { GL_STENCIL_REF		,"GL_STENCIL_REF" },
  { GL_STENCIL_TEST		,"GL_STENCIL_TEST" },
  { GL_STENCIL_VALUE_MASK	,"GL_STENCIL_VALUE_MASK" },
  { GL_STENCIL_WRITEMASK	,"GL_STENCIL_WRITEMASK" },
  { GL_STEREO			,"GL_STEREO" },
  { GL_SUBPIXEL_BITS		,"GL_SUBPIXEL_BITS" },
  { GL_T			,"GL_T" },
  { GL_TEXTURE			,"GL_TEXTURE" },
  { GL_TEXTURE_1D		,"GL_TEXTURE_1D" },
  { GL_TEXTURE_2D		,"GL_TEXTURE_2D" },
  { GL_TEXTURE_BORDER		,"GL_TEXTURE_BORDER" },
  { GL_TEXTURE_BORDER_COLOR	,"GL_TEXTURE_BORDER_COLOR" },
  { GL_TEXTURE_COMPONENTS	,"GL_TEXTURE_COMPONENTS" },
#ifdef GL_TEXTURE_COORD_ARRAY_EXT
  { GL_TEXTURE_COORD_ARRAY_COUNT_EXT,"GL_TEXTURE_COORD_ARRAY_COUNT_EXT" },
  { GL_TEXTURE_COORD_ARRAY_EXT	,"GL_TEXTURE_COORD_ARRAY_EXT" },
  { GL_TEXTURE_COORD_ARRAY_POINTER_EXT,"GL_TEXTURE_COORD_ARRAY_POINTER_EXT" },
  { GL_TEXTURE_COORD_ARRAY_SIZE_EXT,"GL_TEXTURE_COORD_ARRAY_SIZE_EXT" },
  { GL_TEXTURE_COORD_ARRAY_STRIDE_EXT,"GL_TEXTURE_COORD_ARRAY_STRIDE_EXT" },
  { GL_TEXTURE_COORD_ARRAY_TYPE_EXT,"GL_TEXTURE_COORD_ARRAY_TYPE_EXT" },
#endif
  { GL_TEXTURE_ENV		,"GL_TEXTURE_ENV" },
  { GL_TEXTURE_ENV_COLOR	,"GL_TEXTURE_ENV_COLOR" },
  { GL_TEXTURE_ENV_MODE		,"GL_TEXTURE_ENV_MODE" },
  { GL_TEXTURE_GEN_MODE		,"GL_TEXTURE_GEN_MODE" },
  { GL_TEXTURE_GEN_Q		,"GL_TEXTURE_GEN_Q" },
  { GL_TEXTURE_GEN_R		,"GL_TEXTURE_GEN_R" },
  { GL_TEXTURE_GEN_S		,"GL_TEXTURE_GEN_S" },
  { GL_TEXTURE_GEN_T		,"GL_TEXTURE_GEN_T" },
  { GL_TEXTURE_HEIGHT		,"GL_TEXTURE_HEIGHT" },
  { GL_TEXTURE_MAG_FILTER	,"GL_TEXTURE_MAG_FILTER" },
  { GL_TEXTURE_MATRIX		,"GL_TEXTURE_MATRIX" },
  { GL_TEXTURE_MIN_FILTER	,"GL_TEXTURE_MIN_FILTER" },
  { GL_TEXTURE_STACK_DEPTH	,"GL_TEXTURE_STACK_DEPTH" },
  { GL_TEXTURE_WIDTH		,"GL_TEXTURE_WIDTH" },
  { GL_TEXTURE_WRAP_S		,"GL_TEXTURE_WRAP_S" },
  { GL_TEXTURE_WRAP_T		,"GL_TEXTURE_WRAP_T" },
  { GL_TRIANGLES		,"GL_TRIANGLES" },
  { GL_TRIANGLE_FAN		,"GL_TRIANGLE_FAN" },
  { GL_TRIANGLE_STRIP		,"GL_TRIANGLE_STRIP" },
  { GL_UNPACK_ALIGNMENT		,"GL_UNPACK_ALIGNMENT" },
  { GL_UNPACK_LSB_FIRST		,"GL_UNPACK_LSB_FIRST" },
  { GL_UNPACK_ROW_LENGTH	,"GL_UNPACK_ROW_LENGTH" },
  { GL_UNPACK_SKIP_PIXELS	,"GL_UNPACK_SKIP_PIXELS" },
  { GL_UNPACK_SKIP_ROWS		,"GL_UNPACK_SKIP_ROWS" },
  { GL_UNPACK_SWAP_BYTES	,"GL_UNPACK_SWAP_BYTES" },
  { GL_UNSIGNED_BYTE		,"GL_UNSIGNED_BYTE" },
  { GL_UNSIGNED_INT		,"GL_UNSIGNED_INT" },
  { GL_UNSIGNED_SHORT		,"GL_UNSIGNED_SHORT" },
  { GL_VENDOR			,"GL_VENDOR" },
  { GL_VERSION			,"GL_VERSION" },
#ifdef GL_VERTEX_ARRAY_EXT
  { GL_VERTEX_ARRAY_COUNT_EXT	,"GL_VERTEX_ARRAY_COUNT_EXT" },
  { GL_VERTEX_ARRAY_EXT		,"GL_VERTEX_ARRAY_EXT" },
  { GL_VERTEX_ARRAY_POINTER_EXT	,"GL_VERTEX_ARRAY_POINTER_EXT" },
  { GL_VERTEX_ARRAY_SIZE_EXT	,"GL_VERTEX_ARRAY_SIZE_EXT" },
  { GL_VERTEX_ARRAY_STRIDE_EXT	,"GL_VERTEX_ARRAY_STRIDE_EXT" },
  { GL_VERTEX_ARRAY_TYPE_EXT	,"GL_VERTEX_ARRAY_TYPE_EXT" },
#endif
  { GL_VIEWPORT			,"GL_VIEWPORT" },
  { GL_XOR			,"GL_XOR" },
  { GL_ZOOM_X			,"GL_ZOOM_X" },
  { GL_ZOOM_Y			,"GL_ZOOM_Y" },

  /* Magic end-marker - do not remove! */
  { GL_ZERO, NULL }
} ;


int xglTraceIsEnabled ( char *gl_function_name )
{
  static int frameno   = 0 ;
  static int countdown = 0 ;

  if ( strcmp ( gl_function_name, "glutSwapBuffers" ) == 0 )
  {
    if ( countdown == 0 )
    {
      char s [ 100 ] ;

      fprintf ( stderr, "\nContinue Tracing after frame %d [Yes,No,Countdown,eXit] ?", ++frameno ) ;
      gets ( s ) ;

      if ( s[0] == 'x' )
        exit ( 1 ) ;

      xglTraceOn = ( s[0] != 'n' && s[0] != 'c' ) ;

      if ( s[0] == 'c' )
      {
	fprintf ( stderr, "\nHow many frames should I wait until I ask again?" ) ;
	gets ( s ) ;
	countdown = atoi(s) ;
      }

      fprintf ( xglTraceFd, "/* Frame %d - tracing %s */\n", frameno, xglTraceOn ? "ON" : "OFF" ) ;
    }
    else
      countdown-- ;
  }

  return xglTraceOn ;
}


int xglExecuteIsEnabled ( char *gl_function_name )
{
  return TRUE ;
}

char *xglExpandGLenum ( GLenum x )
{
  static GLenum last_val = GL_NONE ;
  static char  *last_str = NULL ;
  char *error_message;
  int i;

  /* Due to ambiguity - these are output as numbers...

      GL_NONE = , GL_ZERO = GL_FALSE = GL_POINTS = 0
      GL_ONE  = , GL_TRUE = GL_LINES = 1
  */

  if ( (int) x == 0 ) return "(GLenum) 0" ;
  if ( (int) x == 1 ) return "(GLenum) 1" ;

  if ( last_val == x ) return last_str ;

  for ( i = 0 ; glenum_string [i].name != NULL ; i++ )
    if ( glenum_string [i].val == x )
      return  glenum_string [i].name ;

  /*
    WARNING - this will leak memory - but it is an error condition,
              so I suppose it's acceptable.
              You can't declare the 'error_message' string as a
              static - or else double errors will go mis-reported.
  */

  error_message = (char *)malloc( 100 * sizeof(char) ) ;

  sprintf ( error_message, "(GLenum) 0x%04x /* Illegal? */", (int) x ) ;

  return error_message ;
}

static GLbyte    b1 [ 1 ],  b2 [ 2 ],  b3 [ 3 ],  b4 [ 4 ] ;
static GLdouble  d1 [ 1 ],  d2 [ 2 ],  d3 [ 3 ],  d4 [ 4 ] ;
static GLfloat   f1 [ 1 ],  f2 [ 2 ],  f3 [ 3 ],  f4 [ 4 ] ;
static GLint     i1 [ 1 ],  i2 [ 2 ],  i3 [ 3 ],  i4 [ 4 ] ;
static GLshort   s1 [ 1 ],  s2 [ 2 ],  s3 [ 3 ],  s4 [ 4 ] ;
static GLubyte  ub1 [ 1 ], ub2 [ 2 ], ub3 [ 3 ], ub4 [ 4 ] ;
static GLuint   ui1 [ 1 ], ui2 [ 2 ], ui3 [ 3 ], ui4 [ 4 ] ;
static GLushort us1 [ 1 ], us2 [ 2 ], us3 [ 3 ], us4 [ 4 ] ;

static GLdouble md [ 16 ] ;
static GLfloat  mf [ 16 ] ;

GLdouble *xglBuild1dv  ( GLdouble v ) {  d1[0] = v ; return  d1 ; }
GLfloat  *xglBuild1fv  ( GLfloat  v ) {  f1[0] = v ; return  f1 ; }
GLbyte   *xglBuild1bv  ( GLbyte   v ) {  b1[0] = v ; return  b1 ; }
GLint    *xglBuild1iv  ( GLint    v ) {  i1[0] = v ; return  i1 ; }
GLshort  *xglBuild1sv  ( GLshort  v ) {  s1[0] = v ; return  s1 ; }
GLubyte  *xglBuild1ubv ( GLubyte  v ) { ub1[0] = v ; return ub1 ; }
GLuint   *xglBuild1uiv ( GLuint   v ) { ui1[0] = v ; return ui1 ; }
GLushort *xglBuild1usv ( GLushort v ) { us1[0] = v ; return us1 ; }

GLdouble *xglBuild2dv  ( GLdouble v0, GLdouble v1 ) {  d2[0] = v0 ;  d2[1] = v1 ; return  d2 ; }
GLfloat  *xglBuild2fv  ( GLfloat  v0, GLfloat  v1 ) {  f2[0] = v0 ;  f2[1] = v1 ; return  f2 ; }
GLbyte   *xglBuild2bv  ( GLbyte   v0, GLbyte   v1 ) {  b2[0] = v0 ;  b2[1] = v1 ; return  b2 ; }
GLint    *xglBuild2iv  ( GLint    v0, GLint    v1 ) {  i2[0] = v0 ;  i2[1] = v1 ; return  i2 ; }
GLshort  *xglBuild2sv  ( GLshort  v0, GLshort  v1 ) {  s2[0] = v0 ;  s2[1] = v1 ; return  s2 ; }
GLubyte  *xglBuild2ubv ( GLubyte  v0, GLubyte  v1 ) { ub2[0] = v0 ; ub2[1] = v1 ; return ub2 ; }
GLuint   *xglBuild2uiv ( GLuint   v0, GLuint   v1 ) { ui2[0] = v0 ; ui2[1] = v1 ; return ui2 ; }
GLushort *xglBuild2usv ( GLushort v0, GLushort v1 ) { us2[0] = v0 ; us2[1] = v1 ; return us2 ; }

GLdouble *xglBuild3dv  ( GLdouble v0, GLdouble v1, GLdouble v2 ) {  d3[0] = v0 ;  d3[1] = v1 ;  d3[2] = v2 ; return  d3 ; }
GLfloat  *xglBuild3fv  ( GLfloat  v0, GLfloat  v1, GLfloat  v2 ) {  f3[0] = v0 ;  f3[1] = v1 ;  f3[2] = v2 ; return  f3 ; }
GLbyte   *xglBuild3bv  ( GLbyte   v0, GLbyte   v1, GLbyte   v2 ) {  b3[0] = v0 ;  b3[1] = v1 ;  b3[2] = v2 ; return  b3 ; }
GLint    *xglBuild3iv  ( GLint    v0, GLint    v1, GLint    v2 ) {  i3[0] = v0 ;  i3[1] = v1 ;  i3[2] = v2 ; return  i3 ; }
GLshort  *xglBuild3sv  ( GLshort  v0, GLshort  v1, GLshort  v2 ) {  s3[0] = v0 ;  s3[1] = v1 ;  s3[2] = v2 ; return  s3 ; }
GLubyte  *xglBuild3ubv ( GLubyte  v0, GLubyte  v1, GLubyte  v2 ) { ub3[0] = v0 ; ub3[1] = v1 ; ub3[2] = v2 ; return ub3 ; }
GLuint   *xglBuild3uiv ( GLuint   v0, GLuint   v1, GLuint   v2 ) { ui3[0] = v0 ; ui3[1] = v1 ; ui3[2] = v2 ; return ui3 ; }
GLushort *xglBuild3usv ( GLushort v0, GLushort v1, GLushort v2 ) { us3[0] = v0 ; us3[1] = v1 ; us3[2] = v2 ; return us3 ; }


GLdouble *xglBuild4dv  ( GLdouble v0, GLdouble v1, GLdouble v2, GLdouble v3 ) {  d4[0] = v0 ;  d4[1] = v1 ;  d4[2] = v2 ;  d4[3] = v3 ; return  d4 ; }
GLfloat  *xglBuild4fv  ( GLfloat  v0, GLfloat  v1, GLfloat  v2, GLfloat  v3 ) {  f4[0] = v0 ;  f4[1] = v1 ;  f4[2] = v2 ;  f4[3] = v3 ; return  f4 ; }
GLbyte   *xglBuild4bv  ( GLbyte   v0, GLbyte   v1, GLbyte   v2, GLbyte   v3 ) {  b4[0] = v0 ;  b4[1] = v1 ;  b4[2] = v2 ;  b4[3] = v3 ; return  b4 ; }
GLint    *xglBuild4iv  ( GLint    v0, GLint    v1, GLint    v2, GLint    v3 ) {  i4[0] = v0 ;  i4[1] = v1 ;  i4[2] = v2 ;  i4[3] = v3 ; return  i4 ; }
GLshort  *xglBuild4sv  ( GLshort  v0, GLshort  v1, GLshort  v2, GLshort  v3 ) {  s4[0] = v0 ;  s4[1] = v1 ;  s4[2] = v2 ;  s4[3] = v3 ; return  s4 ; }
GLubyte  *xglBuild4ubv ( GLubyte  v0, GLubyte  v1, GLubyte  v2, GLubyte  v3 ) { ub4[0] = v0 ; ub4[1] = v1 ; ub4[2] = v2 ; ub4[3] = v3 ; return ub4 ; }
GLuint   *xglBuild4uiv ( GLuint   v0, GLuint   v1, GLuint   v2, GLuint   v3 ) { ui4[0] = v0 ; ui4[1] = v1 ; ui4[2] = v2 ; ui4[3] = v3 ; return ui4 ; }
GLushort *xglBuild4usv ( GLushort v0, GLushort v1, GLushort v2, GLushort v3 ) { us4[0] = v0 ; us4[1] = v1 ; us4[2] = v2 ; us4[3] = v3 ; return us4 ; }

GLdouble *xglBuildMatrixd ( GLdouble m0 , GLdouble m1 , GLdouble m2 , GLdouble m3 ,
                            GLdouble m4 , GLdouble m5 , GLdouble m6 , GLdouble m7 ,
                            GLdouble m8 , GLdouble m9 , GLdouble m10, GLdouble m11,
                            GLdouble m12, GLdouble m13, GLdouble m14, GLdouble m15 )
{
  md[ 0] = m0  ; md[ 1] = m1  ; md[ 2] = m2  ; md[ 3] = m3  ;
  md[ 4] = m4  ; md[ 5] = m5  ; md[ 6] = m6  ; md[ 7] = m7  ;
  md[ 8] = m8  ; md[ 9] = m9  ; md[10] = m10 ; md[11] = m11 ;
  md[12] = m12 ; md[13] = m13 ; md[14] = m14 ; md[15] = m15 ;

  return md ;
}


GLfloat *xglBuildMatrixf ( GLfloat m0 , GLfloat m1 , GLfloat m2 , GLfloat m3 ,
                           GLfloat m4 , GLfloat m5 , GLfloat m6 , GLfloat m7 ,
                           GLfloat m8 , GLfloat m9 , GLfloat m10, GLfloat m11,
                           GLfloat m12, GLfloat m13, GLfloat m14, GLfloat m15 )
{
  mf[ 0] = m0  ; mf[ 1] = m1  ; mf[ 2] = m2  ; mf[ 3] = m3  ;
  mf[ 4] = m4  ; mf[ 5] = m5  ; mf[ 6] = m6  ; mf[ 7] = m7  ;
  mf[ 8] = m8  ; mf[ 9] = m9  ; mf[10] = m10 ; mf[11] = m11 ;
  mf[12] = m12 ; mf[13] = m13 ; mf[14] = m14 ; mf[15] = m15 ;

  return mf ;
}

