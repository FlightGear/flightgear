/*
 * Copyright (c) 2007 Ivan Leben
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library in the file COPYING;
 * if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <vg/openvg.h>
#include "shImage.h"
#include "shContext.h"
#include <string.h>
#include <stdio.h>

#define _ITEM_T SHColor
#define _ARRAY_T SHColorArray
#define _FUNC_T shColorArray
#define _ARRAY_DEFINE
#define _COMPARE_T(c1,c2) 0
#include "shArrayBase.h"

#define _ITEM_T SHImage*
#define _ARRAY_T SHImageArray
#define _FUNC_T shImageArray
#define _ARRAY_DEFINE
#include "shArrayBase.h"


/*-----------------------------------------------------------
 * Prepares the proper pixel pack/unpack info for the given
 * OpenVG image format.
 *-----------------------------------------------------------*/

void shSetupImageFormat(VGImageFormat vg, SHImageFormatDesc *f)
{
  SHuint8 abits = 0;
  SHuint8 tshift = 0;
  SHuint8 tmask = 0;
  SHuint32 amsbBit = 0;
  SHuint32 bgrBit = 0;

  /* Store VG format name */
  f->vgformat = vg;

  /* Check if alpha on MSB or colors in BGR order */
  amsbBit = ((vg & (1 << 6)) >> 6);
  bgrBit = ((vg & (1 << 7)) >> 7);

  /* Find component ordering and size */
  switch(vg & 0x1F)
  {
  case 0: /* VG_sRGBX_8888 */
  case 7: /* VG_lRGBX_8888 */
    f->bytes = 4;
    f->rmask = 0xFF000000;
    f->rshift = 24;
    f->rmax = 255;
    f->gmask = 0x00FF0000;
    f->gshift = 16;
    f->gmax = 255;
    f->bmask = 0x0000FF00;
    f->bshift = 8;
    f->bmax = 255;
    f->amask = 0x0;
    f->ashift = 0;
    f->amax = 1;
    break;
  case 1: /* VG_sRGBA_8888 */
  case 2: /* VG_sRGBA_8888_PRE */
  case 8: /* VG_lRGBA_8888 */
  case 9: /* VG_lRGBA_8888_PRE */
    f->bytes = 4;
    f->rmask = 0xFF000000;
    f->rshift = 24;
    f->rmax = 255;
    f->gmask = 0x00FF0000;
    f->gshift = 16;
    f->gmax = 255;
    f->bmask = 0x0000FF00;
    f->bshift = 8;
    f->bmax = 255;
    f->amask = 0x000000FF;
    f->ashift = 0;
    f->amax = 255;
    break;
  case 3: /* VG_sRGB_565 */
    f->bytes = 2;
    f->rmask = 0xF800;
    f->rshift = 11;
    f->rmax = 31;
    f->gmask = 0x07E0;
    f->gshift = 5;
    f->gmax = 63;
    f->bmask = 0x001F;
    f->bshift = 0;
    f->bmax = 31;
    f->amask = 0x0;
    f->ashift = 0;
    f->amax = 1;
    break;
  case 4: /* VG_sRGBA_5551 */
    f->bytes = 2;
    f->rmask = 0xF800;
    f->rshift = 11;
    f->rmax = 31;
    f->gmask = 0x07C0;
    f->gshift = 6;
    f->gmax = 31;
    f->bmask = 0x003E;
    f->bshift = 1;
    f->bmax = 31;
    f->amask = 0x0001;
    f->ashift = 0;
    f->amax = 1;
    break;
  case 5: /* VG_sRGBA_4444 */
    f->bytes = 2;
    f->rmask = 0xF000;
    f->rshift = 12;
    f->rmax = 15;
    f->gmask = 0x0F00;
    f->gshift = 8;
    f->gmax = 15;
    f->bmask = 0x00F0;
    f->bshift = 4;
    f->bmax = 15;
    f->amask = 0x000F;
    f->ashift = 0;
    f->amax = 15;
    break;
  case 6: /* VG_sL_8 */
  case 10: /* VG_lL_8 */
    f->bytes = 1;
    f->rmask = 0xFF;
    f->rshift = 0;
    f->rmax = 255;
    f->gmask = 0xFF;
    f->gshift = 0;
    f->gmax = 255;
    f->bmask = 0xFF;
    f->bshift = 0;
    f->bmax = 255;
    f->amask = 0x0;
    f->ashift = 0;
    f->amax = 1;
    break;
  case 11: /* VG_A_8 */
    f->bytes = 1;
    f->rmask = 0x0;
    f->rshift = 0;
    f->rmax = 1;
    f->gmask = 0x0;
    f->gshift = 0;
    f->gmax = 1;
    f->bmask = 0x0;
    f->bshift = 0;
    f->bmax = 1;
    f->amask = 0xFF;
    f->ashift = 0;
    f->amax = 255;
    break;
  case 12: /* VG_BW_1 */
    f->bytes = 1;
    f->rmask = 0x0;
    f->rshift = 0;
    f->rmax = 1;
    f->gmask = 0x0;
    f->gshift = 0;
    f->gmax = 1;
    f->bmask = 0x0;
    f->bshift = 0;
    f->bmax = 1;
    f->amask = 0x0;
    f->ashift = 0;
    f->amax = 1;
    break;
  }

  /* Check for A,X at MSB */
  if (amsbBit) {

    abits = f->bshift;

    f->rshift -= abits;
    f->gshift -= abits;
    f->bshift -= abits;
    f->ashift = f->bytes * 8 - abits;

    f->rmask >>= abits;
    f->gmask >>= abits;
    f->bmask >>= abits;
    f->amask <<= f->bytes * 8 - abits;
  }

  /* Check for BGR ordering */
  if (bgrBit) {

    tshift = f->bshift;
    f->bshift = f->rshift;
    f->rshift = tshift;

    tmask = f->bmask;
    f->bmask = f->rmask;
    f->rmask = tmask;
  }

  /* Find proper mapping to OpenGL formats */
  switch(vg & 0x1F)
  {
  case VG_sRGBX_8888:
  case VG_lRGBX_8888:
  case VG_sRGBA_8888:
  case VG_sRGBA_8888_PRE:
  case VG_lRGBA_8888:
  case VG_lRGBA_8888_PRE:

    f->glintformat = GL_RGBA;

    if (amsbBit == 0 && bgrBit == 0) {
      f->glformat = GL_RGBA;
      f->gltype = GL_UNSIGNED_INT_8_8_8_8;

    }else if (amsbBit == 1 && bgrBit == 0) {
      f->glformat = GL_BGRA;
      f->gltype = GL_UNSIGNED_INT_8_8_8_8_REV;

    }else if (amsbBit == 0 && bgrBit == 1) {
      f->glformat = GL_BGRA;
      f->gltype = GL_UNSIGNED_INT_8_8_8_8;

    }else if (amsbBit == 1 && bgrBit == 1) {
      f->glformat = GL_RGBA;
      f->gltype = GL_UNSIGNED_INT_8_8_8_8_REV;
    }

    break;
  case VG_sRGBA_5551:

    f->glintformat = GL_RGBA;

    if (amsbBit == 0 && bgrBit == 0) {
      f->glformat = GL_RGBA;
      f->gltype = GL_UNSIGNED_SHORT_5_5_5_1;

    }else if (amsbBit == 1 && bgrBit == 0) {
      f->glformat = GL_BGRA;
      f->gltype = GL_UNSIGNED_SHORT_1_5_5_5_REV;

    }else if (amsbBit == 0 && bgrBit == 1) {
      f->glformat = GL_BGRA;
      f->gltype = GL_UNSIGNED_SHORT_5_5_5_1;

    }else if (amsbBit == 1 && bgrBit == 1) {
      f->glformat = GL_RGBA;
      f->gltype = GL_UNSIGNED_SHORT_1_5_5_5_REV;
    }

    break;
  case VG_sRGBA_4444:

    f->glintformat = GL_RGBA;

    if (amsbBit == 0 && bgrBit == 0) {
      f->glformat = GL_RGBA;
      f->gltype = GL_UNSIGNED_SHORT_4_4_4_4;

    }else if (amsbBit == 1 && bgrBit == 0) {
      f->glformat = GL_BGRA;
      f->gltype = GL_UNSIGNED_SHORT_4_4_4_4_REV;

    }else if (amsbBit == 0 && bgrBit == 1) {
      f->glformat = GL_BGRA;
      f->gltype = GL_UNSIGNED_SHORT_4_4_4_4;

    }else if (amsbBit == 1 && bgrBit == 1) {
      f->glformat = GL_RGBA;
      f->gltype = GL_UNSIGNED_SHORT_4_4_4_4_REV;
    }

    break;
  case VG_sRGB_565:

    f->glintformat = GL_RGB;

    if (bgrBit == 0) {
      f->glformat = GL_RGB;
      f->gltype = GL_UNSIGNED_SHORT_5_6_5;

    }else if (bgrBit == 1) {
      f->glformat = GL_RGB;
      f->gltype = GL_UNSIGNED_SHORT_5_6_5;
    }

    break;
  case VG_sL_8:
  case VG_lL_8:

    f->glintformat = GL_LUMINANCE;
    f->glformat = GL_LUMINANCE;
    f->gltype = GL_UNSIGNED_BYTE;

    break;
  case VG_A_8:

    f->glintformat = GL_ALPHA;
    f->glformat = GL_ALPHA;
    f->gltype = GL_UNSIGNED_BYTE;

    break;
  case VG_BW_1:

    f->glintformat = 0;
    f->glformat = 0;
    f->gltype = 0;
    break;
  }
}

/*-----------------------------------------------------
 * Returns 1 if the given format is valid according to
 * the OpenVG specification, else 0.
 *-----------------------------------------------------*/

int shIsValidImageFormat(VGImageFormat format)
{
  SHint aOrderBit = (1 << 6);
  SHint rgbOrderBit = (1 << 7);
  SHint baseFormat = format & 0x1F;
  SHint unorderedRgba = format & (~(aOrderBit | rgbOrderBit));
  SHint isRgba = (baseFormat == VG_sRGBX_8888     ||
                  baseFormat == VG_sRGBA_8888     ||
                  baseFormat == VG_sRGBA_8888_PRE ||
                  baseFormat == VG_sRGBA_5551     ||
                  baseFormat == VG_sRGBA_4444     ||
                  baseFormat == VG_lRGBX_8888     ||
                  baseFormat == VG_lRGBA_8888     ||
                  baseFormat == VG_lRGBA_8888_PRE);
  
  SHint check = isRgba ? unorderedRgba : format;
  return check >= VG_sRGBX_8888 && check <= VG_BW_1;
}

/*-----------------------------------------------------
 * Returns 1 if the given format is supported by this
 * implementation
 *-----------------------------------------------------*/

int shIsSupportedImageFormat(VGImageFormat format)
{
  SHuint32 baseFormat = (format & 0x1F);
  if (baseFormat == VG_sRGBA_8888_PRE ||
      baseFormat == VG_lRGBA_8888_PRE ||
      baseFormat == VG_BW_1)
      return 0;

  return 1;
}

/*--------------------------------------------------------
 * Packs the pixel color components into memory at given
 * address according to given format
 *--------------------------------------------------------*/

void shStoreColor(SHColor *c, void *data, SHImageFormatDesc *f)
{
  /*
  TODO: unsupported formats:
  - s and l both behave linearly
  - 1-bit black & white (BW_1)
  */

  SHfloat l = 0.0f;
  SHuint32 out = 0x0;

  if (f->vgformat == VG_lL_8 || f->vgformat == VG_sL_8) {

    /* Grayscale (luminosity) conversion as defined by the spec */
    l = 0.2126f * c->r + 0.7152f * c->g + 0.0722f * c->r;
    out = (SHuint32)(l * (SHfloat)f->rmax + 0.5f);

  }else{

    /* Pack color components */
    out += ( ((SHuint32)(c->r * (SHfloat)f->rmax + 0.5f)) << f->rshift ) & f->rmask;
    out += ( ((SHuint32)(c->g * (SHfloat)f->gmax + 0.5f)) << f->gshift ) & f->gmask;
    out += ( ((SHuint32)(c->b * (SHfloat)f->bmax + 0.5f)) << f->bshift ) & f->bmask;
    out += ( ((SHuint32)(c->a * (SHfloat)f->amax + 0.5f)) << f->ashift ) & f->amask;
  }
  
  /* Store to buffer */
  switch (f->bytes) {
  case 4: *((SHuint32*)data) = (SHuint32)(out & 0xFFFFFFFF); break;
  case 2: *((SHuint16*)data) = (SHuint16)(out & 0x0000FFFF); break;
  case 1: *((SHuint8*)data)  = (SHuint8) (out & 0x000000FF); break;
  }
}

/*---------------------------------------------------------
 * Unpacks the pixel color components from memory at given
 * address according to the given format
 *---------------------------------------------------------*/

void shLoadColor(SHColor *c, const void *data, SHImageFormatDesc *f)
{
  /*
  TODO: unsupported formats:
  - s and l both behave linearly
  - 1-bit black & white (BW_1)
  */

  SHuint32 in = 0x0;
  
  /* Load from buffer */
  switch (f->bytes) {
  case 4: in = (SHuint32) *((SHuint32*)data); break;
  case 2: in = (SHuint32) *((SHuint16*)data); break;
  case 1: in = (SHuint32) *((SHuint8*)data); break;
  }

  /* Unpack color components */
  c->r = (SHfloat)((in & f->rmask) >> f->rshift) / (SHfloat) f->rmax;
  c->g = (SHfloat)((in & f->gmask) >> f->gshift) / (SHfloat) f->gmax;
  c->b = (SHfloat)((in & f->bmask) >> f->bshift) / (SHfloat) f->bmax;
  c->a = (SHfloat)((in & f->amask) >> f->ashift) / (SHfloat) f->amax;
  
  /* Initialize unused components to 1 */
  if (f->amask == 0x0) { c->a = 1.0f; }
  if (f->rmask == 0x0) { c->r = 1.0f; c->g = 1.0f; c->b = 1.0f; }
}


/*----------------------------------------------
 * Color and Image constructors and destructors
 *----------------------------------------------*/

void SHColor_ctor(SHColor *c)
{
  c->r = 0.0f;
  c->g = 0.0f;
  c->b = 0.0f;
  c->a = 0.0f;
}

void SHColor_dtor(SHColor *c) {
}

void SHImage_ctor(SHImage *i)
{
  i->data = NULL;
  i->width = 0;
  i->height = 0;
  glGenTextures(1, &i->texture);
}

void SHImage_dtor(SHImage *i)
{
  if (i->data != NULL)
    free(i->data);
  
  if (glIsTexture(i->texture))
    glDeleteTextures(1, &i->texture);
}

/*--------------------------------------------------------
 * Finds appropriate OpenGL texture size for the size of
 * the given image
 *--------------------------------------------------------*/

void shUpdateImageTextureSize(SHImage *i)
{
  i->texwidth = i->width;
  i->texheight = i->height;
  i->texwidthK = 1.0f;
  i->texheightK = 1.0f;
  
  /* Round size to nearest power of 2 */
  /* TODO: might be dropped out if it works without  */
  
  /*i->texwidth = 1;
  while (i->texwidth < i->width)
    i->texwidth *= 2;
  
  i->texheight = 1;
  while (i->texheight < i->height)
    i->texheight *= 2;
  
  i->texwidthK  = (SHfloat)i->width  / i->texwidth;
  i->texheightK = (SHfloat)i->height / i->texheight; */
}

/*--------------------------------------------------
 * Downloads the image data from OpenVG into 
 * an OpenGL texture
 *--------------------------------------------------*/

void shUpdateImageTexture(SHImage *i, VGContext *c)
{
  SHint potwidth;
  SHint potheight;
  SHint8 *potdata;

  /* Find nearest power of two size */

  potwidth = 1;
  while (potwidth < i->width)
    potwidth *= 2;
  
  potheight = 1;
  while (potheight < i->height)
    potheight *= 2;
  
  
  /* Scale into a temp buffer if image not a power-of-two size (pot)
     and non-power-of-two textures are not supported by OpenGL */  
  
  if ((i->width < potwidth || i->height < potheight) &&
      !c->isGLAvailable_TextureNonPowerOfTwo) {
    
    potdata = (SHint8*)malloc( potwidth * potheight * i->fd.bytes );
    if (!potdata) return;
    
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glBindTexture(GL_TEXTURE_2D, i->texture);
    
    
    gluScaleImage(i->fd.glformat, i->width, i->height, i->fd.gltype, i->data,
                  potwidth, potheight, i->fd.gltype, potdata);
    
    glTexImage2D(GL_TEXTURE_2D, 0, i->fd.glintformat, potwidth, potheight, 0,
                 i->fd.glformat, i->fd.gltype, potdata);
    
    free(potdata);
    return;
  }
  
  /* Store pixels to texture */
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glBindTexture(GL_TEXTURE_2D, i->texture);
  glTexImage2D(GL_TEXTURE_2D, 0, i->fd.glintformat,
               i->texwidth, i->texheight, 0,
               i->fd.glformat, i->fd.gltype, i->data);
}

/*----------------------------------------------------------
 * Creates a new image object and returns the handle to it
 *----------------------------------------------------------*/

VG_API_CALL VGImage vgCreateImage(VGImageFormat format,
                                  VGint width, VGint height,
                                  VGbitfield allowedQuality)
{
  SHImage *i = NULL;
  SHImageFormatDesc fd;
  VG_GETCONTEXT(VG_INVALID_HANDLE);
  
  /* Reject invalid formats */
  VG_RETURN_ERR_IF(!shIsValidImageFormat(format),
                   VG_UNSUPPORTED_IMAGE_FORMAT_ERROR,
                   VG_INVALID_HANDLE);
  
  /* Reject unsupported formats */
  VG_RETURN_ERR_IF(!shIsSupportedImageFormat(format),
                   VG_UNSUPPORTED_IMAGE_FORMAT_ERROR,
                   VG_INVALID_HANDLE);
  
  /* Reject invalid sizes */
  VG_RETURN_ERR_IF(width  <= 0 || width > SH_MAX_IMAGE_WIDTH ||
                   height <= 0 || height > SH_MAX_IMAGE_HEIGHT ||
                   width * height > SH_MAX_IMAGE_PIXELS,
                   VG_ILLEGAL_ARGUMENT_ERROR, VG_INVALID_HANDLE);
  
  /* Check if byte size exceeds SH_MAX_IMAGE_BYTES */
  shSetupImageFormat(format, &fd);
  VG_RETURN_ERR_IF(width * height * fd.bytes > SH_MAX_IMAGE_BYTES,
                   VG_ILLEGAL_ARGUMENT_ERROR, VG_INVALID_HANDLE);
  
  /* Reject invalid quality bits */
  VG_RETURN_ERR_IF(allowedQuality &
                   ~(VG_IMAGE_QUALITY_NONANTIALIASED |
                     VG_IMAGE_QUALITY_FASTER | VG_IMAGE_QUALITY_BETTER),
                   VG_ILLEGAL_ARGUMENT_ERROR, VG_INVALID_HANDLE);
  
  /* Create new image object */
  SH_NEWOBJ(SHImage, i);
  VG_RETURN_ERR_IF(!i, VG_OUT_OF_MEMORY_ERROR, VG_INVALID_HANDLE);
  i->width = width;
  i->height = height;
  i->fd = fd;
  
  /* Allocate data memory */
  shUpdateImageTextureSize(i);
  i->data = (SHuint8*)malloc( i->texwidth * i->texheight * fd.bytes );
  
  if (i->data == NULL) {
    SH_DELETEOBJ(SHImage, i);
    VG_RETURN_ERR(VG_OUT_OF_MEMORY_ERROR, VG_INVALID_HANDLE); }
  
  /* Initialize data by zeroing-out */
  memset(i->data, 1, width * height * fd.bytes);
  shUpdateImageTexture(i, context);
  
  /* Add to resource list */
  shImageArrayPushBack(&context->images, i);
  
  VG_RETURN((VGImage)i);
}

VG_API_CALL void vgDestroyImage(VGImage image)
{
  SHint index;
  VG_GETCONTEXT(VG_NO_RETVAL);
  
  /* Check if valid resource */
  index = shImageArrayFind(&context->images, (SHImage*)image);
  VG_RETURN_ERR_IF(index == -1, VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);
  
  /* Delete object and remove resource */
  SH_DELETEOBJ(SHImage, (SHImage*)image);
  shImageArrayRemoveAt(&context->images, index);
  
  VG_RETURN(VG_NO_RETVAL);
}

/*---------------------------------------------------
 * Clear given rectangle area in the image data with
 * color set via vgSetfv(VG_CLEAR_COLOR, ...)
 *---------------------------------------------------*/

VG_API_CALL void vgClearImage(VGImage image,
                              VGint x, VGint y, VGint width, VGint height)
{
  SHImage *i;
  SHColor clear;
  SHuint8 *data;
  SHint X,Y, ix, iy, dx, dy, stride;
  VG_GETCONTEXT(VG_NO_RETVAL);
  
  VG_RETURN_ERR_IF(!shIsValidImage(context, image),
                   VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);
  
  /* TODO: check if image current render target */
  
  i = (SHImage*)image;
  VG_RETURN_ERR_IF(width <= 0 || height <= 0,
                   VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);
  
  /* Nothing to do if target rectangle out of bounds */
  if (x >= i->width || y >= i->height)
    VG_RETURN(VG_NO_RETVAL);
  if (x + width < 0 || y + height < 0)
    VG_RETURN(VG_NO_RETVAL);

  /* Clamp to image bounds */
  ix = SH_MAX(x, 0); dx = ix - x;
  iy = SH_MAX(y, 0); dy = iy - y;
  width  = SH_MIN( width  - dx, i->width  - ix);
  height = SH_MIN( height - dy, i->height - iy);
  stride = i->texwidth * i->fd.bytes;
  
  /* Walk pixels and clear*/
  clear = context->clearColor;
  
  for (Y=iy; Y<iy+height; ++Y) {
    data = i->data + ( Y*stride + ix * i->fd.bytes );
    
    for (X=ix; X<ix+width; ++X) {
      shStoreColor(&clear, data, &i->fd);
      data += i->fd.bytes;
    }}
  
  shUpdateImageTexture(i, context);
  VG_RETURN(VG_NO_RETVAL);
}

/*------------------------------------------------------------
 * Generic function for copying a rectangle area of pixels
 * of size (width,height) among two data buffers. The size of
 * source (swidth,sheight) and destination (dwidth,dheight)
 * images may vary as well as the source coordinates (sx,sy)
 * and destination coordinates(dx, dy).
 *------------------------------------------------------------*/

void shCopyPixels(SHuint8 *dst, VGImageFormat dstFormat, SHint dstStride,
                  const SHuint8 *src, VGImageFormat srcFormat, SHint srcStride,
                  SHint dwidth, SHint dheight, SHint swidth, SHint sheight,
                  SHint dx, SHint dy, SHint sx, SHint sy,
                  SHint width, SHint height)
{
  SHint dxold, dyold;
  SHint SX, SY, DX, DY;
  const SHuint8 *SD;
  SHuint8 *DD;
  SHColor c;

  SHImageFormatDesc dfd;
  SHImageFormatDesc sfd;

  /* Setup image format descriptors */
  SH_ASSERT(shIsSupportedImageFormat(dstFormat));
  SH_ASSERT(shIsSupportedImageFormat(srcFormat));
  shSetupImageFormat(dstFormat, &dfd);
  shSetupImageFormat(srcFormat, &sfd);

  /*
    In order to optimize the copying loop and remove the
    if statements from it to check whether target pixel
    is in the source and destination surface, we clamp
    copy rectangle in advance. This is quite a tedious
    task though. Here is a picture of the scene. Note that
    (dx,dy) is actually an offset of the copy rectangle
    (clamped to src surface) from the (0,0) point on dst
    surface. A negative (dx,dy) (as in this picture) also
    affects src coords of the copy rectangle which have
    to be readjusted again (sx,sy,width,height).

                          src
    *----------------------*
    | (sx,sy)  copy rect   |
    | *-----------*        |
    | |\(dx, dy)  |        |          dst
    | | *------------------------------*
    | | |xxxxxxxxx|        |           |
    | | |xxxxxxxxx|        |           |
    | *-----------*        |           |
    |   |   (width,height) |           |
    *----------------------*           |
        |           (swidth,sheight)   |
        *------------------------------*
                                (dwidth,dheight)
  */

  /* Cancel if copy rect out of src bounds */
  if (sx >= swidth || sy >= sheight) return;
  if (sx + width < 0 || sy + height < 0) return;
  
  /* Clamp copy rectangle to src bounds */
  sx = SH_MAX(sx, 0);
  sy = SH_MAX(sy, 0);
  width = SH_MIN(width, swidth - sx);
  height = SH_MIN(height, sheight - sy);
  
  /* Cancel if copy rect out of dst bounds */
  if (dx >= dwidth || dy >= dheight) return;
  if (dx + width < 0 || dy + height < 0) return;
  
  /* Clamp copy rectangle to dst bounds */
  dxold = dx; dyold = dy;
  dx = SH_MAX(dx, 0);
  dy = SH_MAX(dy, 0);
  sx += dx - dxold;
  sy += dy - dyold;
  width -= dx - dxold;
  height -= dy - dyold;
  width = SH_MIN(width, dwidth  - dx);
  height = SH_MIN(height, dheight - dy);
  
  /* Calculate stride from format if not given */
  if (dstStride == -1) dstStride = dwidth * dfd.bytes;
  if (srcStride == -1) srcStride = swidth * sfd.bytes;
  
  if (srcFormat == dstFormat) {
    
    /* Walk pixels and copy */
    for (SY=sy, DY=dy; SY < sy+height; ++SY, ++DY) {
      SD = src + SY * srcStride + sx * sfd.bytes;
      DD = dst + DY * dstStride + dx * dfd.bytes;
      memcpy(DD, SD, width * sfd.bytes);
    }
    
  }else{
  
    /* Walk pixels and copy */
    for (SY=sy, DY=dy; SY < sy+height; ++SY, ++DY) {
      SD = src + SY * srcStride + sx * sfd.bytes;
      DD = dst + DY * dstStride + dx * dfd.bytes;
      
      for (SX=sx, DX=dx; SX < sx+width; ++SX, ++DX) {
        shLoadColor(&c, SD, &sfd);
        shStoreColor(&c, DD, &dfd);
        SD += sfd.bytes; DD += dfd.bytes;
      }}
  }
}

/*---------------------------------------------------------
 * Copies a rectangle area of pixels of size (width,height)
 * from given data buffer to image surface at destination
 * coordinates (x,y)
 *---------------------------------------------------------*/

VG_API_CALL void vgImageSubData(VGImage image,
                                const void * data, VGint dataStride,
                                VGImageFormat dataFormat,
                                VGint x, VGint y, VGint width, VGint height)
{
  SHImage *i;
  VG_GETCONTEXT(VG_NO_RETVAL);
  
  VG_RETURN_ERR_IF(!shIsValidImage(context, image),
                   VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);
  
  /* TODO: check if image current render target */
  i = (SHImage*)image;
  
  /* Reject invalid formats */
  VG_RETURN_ERR_IF(!shIsValidImageFormat(dataFormat),
                   VG_UNSUPPORTED_IMAGE_FORMAT_ERROR,
                   VG_NO_RETVAL);
  
  /* Reject unsupported image formats */
  VG_RETURN_ERR_IF(!shIsSupportedImageFormat(dataFormat),
                   VG_UNSUPPORTED_IMAGE_FORMAT_ERROR,
                   VG_NO_RETVAL);
  
  VG_RETURN_ERR_IF(width <= 0 || height <= 0 || !data,
                   VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);
  
  /* TODO: check data array alignment */
  
  shCopyPixels(i->data, i->fd.vgformat, i->texwidth * i->fd.bytes,
               data, dataFormat,dataStride,
               i->width, i->height, width, height,
               x, y, 0, 0, width, height);
  
  shUpdateImageTexture(i, context);
  VG_RETURN(VG_NO_RETVAL);
}

/*---------------------------------------------------------
 * Copies a rectangle area of pixels of size (width,height)
 * from image surface at source coordinates (x,y) to given
 * data buffer
 *---------------------------------------------------------*/

VG_API_CALL void vgGetImageSubData(VGImage image,
                                   void * data, VGint dataStride,
                                   VGImageFormat dataFormat,
                                   VGint x, VGint y,
                                   VGint width, VGint height)
{
  SHImage *i;
  VG_GETCONTEXT(VG_NO_RETVAL);
  
  VG_RETURN_ERR_IF(!shIsValidImage(context, image),
                   VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);
  
  /* TODO: check if image current render target */
  i = (SHImage*)image;
  
  /* Reject invalid formats */
  VG_RETURN_ERR_IF(!shIsValidImageFormat(dataFormat),
                   VG_UNSUPPORTED_IMAGE_FORMAT_ERROR,
                   VG_NO_RETVAL);
  
  /* Reject unsupported formats */
  VG_RETURN_ERR_IF(!shIsSupportedImageFormat(dataFormat),
                   VG_UNSUPPORTED_IMAGE_FORMAT_ERROR,
                   VG_NO_RETVAL);
  
  VG_RETURN_ERR_IF(width <= 0 || height <= 0 || !data,
                   VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);
  
  /* TODO: check data array alignment */
  
  shCopyPixels(data, dataFormat, dataStride,
               i->data, i->fd.vgformat, i->texwidth * i->fd.bytes,
               width, height, i->width, i->height,
               0,0,x,x,width,height);
  
  VG_RETURN(VG_NO_RETVAL);
}

/*----------------------------------------------------------
 * Copies a rectangle area of pixels of size (width,height)
 * from src image surface at source coordinates (sx,sy) to
 * dst image surface at destination cordinates (dx,dy)
 *---------------------------------------------------------*/

VG_API_CALL void vgCopyImage(VGImage dst, VGint dx, VGint dy,
                             VGImage src, VGint sx, VGint sy,
                             VGint width, VGint height,
                             VGboolean dither)
{
  SHImage *s, *d;
  SHuint8 *pixels;

  VG_GETCONTEXT(VG_NO_RETVAL);
  
  VG_RETURN_ERR_IF(!shIsValidImage(context, src) ||
                   !shIsValidImage(context, dst),
                   VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);

  /* TODO: check if images current render target */

  s = (SHImage*)src; d = (SHImage*)dst;
  VG_RETURN_ERR_IF(width <= 0 || height <= 0,
                   VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);

  /* In order to perform copying in a cosistent fashion
     we first copy to a temporary buffer and only then to
     destination image */
  
  /* TODO: rather check first if the image is really
     the same and whether the regions overlap. if not
     we can copy directly */

  pixels = (SHuint8*)malloc(width * height * s->fd.bytes);
  SH_RETURN_ERR_IF(!pixels, VG_OUT_OF_MEMORY_ERROR, SH_NO_RETVAL);

  shCopyPixels(pixels, s->fd.vgformat, s->texwidth * s->fd.bytes,
               s->data, s->fd.vgformat, s->texwidth * s->fd.bytes,
               width, height, s->width, s->height,
               0, 0, sx, sy, width, height);

  shCopyPixels(d->data, d->fd.vgformat, d->texwidth * d->fd.bytes,
               pixels, s->fd.vgformat, s->texwidth * s->fd.bytes,
               d->width, d->height, width, height,
               dx, dy, 0, 0, width, height);
  
  free(pixels);
  
  shUpdateImageTexture(d, context);
  VG_RETURN(VG_NO_RETVAL);
}

/*---------------------------------------------------------
 * Copies a rectangle area of pixels of size (width,height)
 * from src image surface at source coordinates (sx,sy) to
 * window surface at destination coordinates (dx,dy)
 *---------------------------------------------------------*/

VG_API_CALL void vgSetPixels(VGint dx, VGint dy,
                             VGImage src, VGint sx, VGint sy,
                             VGint width, VGint height)
{
  SHImage *i;
  SHuint8 *pixels;
  SHImageFormatDesc winfd;

  VG_GETCONTEXT(VG_NO_RETVAL);
  
  VG_RETURN_ERR_IF(!shIsValidImage(context, src),
                   VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);
  
  /* TODO: check if image current render target (requires EGL) */

  i = (SHImage*)src;
  VG_RETURN_ERR_IF(width <= 0 || height <= 0,
                   VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);

  /* Setup window image format descriptor */
  /* TODO: this actually depends on the target framebuffer type
     if we really want the copy to be optimized */
  shSetupImageFormat(VG_sRGBA_8888, &winfd);

  /* OpenGL doesn't allow us to use random stride. We have to
     manually copy the image data and write from a copy with
     normal row length (without power-of-two roundup pixels) */

  pixels = (SHuint8*)malloc(width * height * winfd.bytes);
  SH_RETURN_ERR_IF(!pixels, VG_OUT_OF_MEMORY_ERROR, SH_NO_RETVAL);

  shCopyPixels(pixels, winfd.vgformat, -1,
               i->data, i->fd.vgformat, i->texwidth * i->fd.bytes,
               width, height, i->width, i->height,
               0,0,sx,sy, width, height);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glRasterPos2i(dx, dy);
  glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
  glRasterPos2i(0,0);
  
  free(pixels);

  VG_RETURN(VG_NO_RETVAL);
}

/*---------------------------------------------------------
 * Copies a rectangle area of pixels of size (width,height)
 * from given data buffer at source coordinates (sx,sy) to
 * window surface at destination coordinates (dx,dy)
 *---------------------------------------------------------*/

VG_API_CALL void vgWritePixels(const void * data, VGint dataStride,
                               VGImageFormat dataFormat,
                               VGint dx, VGint dy,
                               VGint width, VGint height)
{
  SHuint8 *pixels;
  SHImageFormatDesc winfd;

  VG_GETCONTEXT(VG_NO_RETVAL);

  /* Reject invalid formats */
  VG_RETURN_ERR_IF(!shIsValidImageFormat(dataFormat),
                   VG_UNSUPPORTED_IMAGE_FORMAT_ERROR,
                   VG_NO_RETVAL);
  
  /* Reject unsupported formats */
  VG_RETURN_ERR_IF(!shIsSupportedImageFormat(dataFormat),
                   VG_UNSUPPORTED_IMAGE_FORMAT_ERROR,
                   VG_NO_RETVAL);

  /* TODO: check output data array alignment */
  
  VG_RETURN_ERR_IF(width <= 0 || height <= 0 || !data,
                   VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);

  /* Setup window image format descriptor */
  /* TODO: this actually depends on the target framebuffer type
     if we really want the copy to be optimized */
  shSetupImageFormat(VG_sRGBA_8888, &winfd);

  /* OpenGL doesn't allow us to use random stride. We have to
     manually copy the image data and write from a copy with
     normal row length */

  pixels = (SHuint8*)malloc(width * height * winfd.bytes);
  SH_RETURN_ERR_IF(!pixels, VG_OUT_OF_MEMORY_ERROR, SH_NO_RETVAL);
  
  shCopyPixels(pixels, winfd.vgformat, -1,
               (SHuint8*)data, dataFormat, dataStride,
               width, height, width, height,
               0,0,0,0, width, height);
  
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glRasterPos2i(dx, dy);
  glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
  glRasterPos2i(0,0);
  
  free(pixels);

  VG_RETURN(VG_NO_RETVAL); 
}

/*-----------------------------------------------------------
 * Copies a rectangle area of pixels of size (width, height)
 * from window surface at source coordinates (sx, sy) to
 * image surface at destination coordinates (dx, dy)
 *-----------------------------------------------------------*/

VG_API_CALL void vgGetPixels(VGImage dst, VGint dx, VGint dy,
                             VGint sx, VGint sy,
                             VGint width, VGint height)
{
  SHImage *i;
  SHuint8 *pixels;
  SHImageFormatDesc winfd;
  VG_GETCONTEXT(VG_NO_RETVAL);
  
  VG_RETURN_ERR_IF(!shIsValidImage(context, dst),
                   VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);
  
   /* TODO: check if image current render target */

  i = (SHImage*)dst;
  VG_RETURN_ERR_IF(width <= 0 || height <= 0,
                   VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);

  /* Setup window image format descriptor */
  /* TODO: this actually depends on the target framebuffer type
     if we really want the copy to be optimized */
  shSetupImageFormat(VG_sRGBA_8888, &winfd);
  
  /* OpenGL doesn't allow us to read to random destination
     coordinates nor using random stride. We have to
     read first and then manually copy to the image data */

  pixels = (SHuint8*)malloc(width * height * winfd.bytes);
  SH_RETURN_ERR_IF(!pixels, VG_OUT_OF_MEMORY_ERROR, SH_NO_RETVAL);

  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(sx, sy, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
  
  shCopyPixels(i->data, i->fd.vgformat, i->texwidth * i->fd.bytes,
               pixels, winfd.vgformat, -1,
               i->width, i->height, width, height,
               dx, dy, 0, 0, width, height);

  free(pixels);
  
  shUpdateImageTexture(i, context);
  VG_RETURN(VG_NO_RETVAL);
}

/*-----------------------------------------------------------
 * Copies a rectangle area of pixels of size (width, height)
 * from window surface at source coordinates (sx, sy) to
 * to given output data buffer.
 *-----------------------------------------------------------*/

VG_API_CALL void vgReadPixels(void * data, VGint dataStride,
                              VGImageFormat dataFormat,
                              VGint sx, VGint sy,
                              VGint width, VGint height)
{
  SHuint8 *pixels;
  SHImageFormatDesc winfd;
  VG_GETCONTEXT(VG_NO_RETVAL);

  /* Reject invalid formats */
  VG_RETURN_ERR_IF(!shIsValidImageFormat(dataFormat),
                   VG_UNSUPPORTED_IMAGE_FORMAT_ERROR,
                   VG_NO_RETVAL);
  
  /* Reject unsupported image formats */
  VG_RETURN_ERR_IF(!shIsSupportedImageFormat(dataFormat),
                   VG_UNSUPPORTED_IMAGE_FORMAT_ERROR,
                   VG_NO_RETVAL);

  /* TODO: check output data array alignment */
  
  VG_RETURN_ERR_IF(width <= 0 || height <= 0 || !data,
                   VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);

  /* Setup window image format descriptor */
  /* TODO: this actually depends on the target framebuffer type
     if we really want the copy to be optimized */
  shSetupImageFormat(VG_sRGBA_8888, &winfd);

  /* OpenGL doesn't allow random data stride. We have to
     read first and then manually copy to the output buffer */

  pixels = (SHuint8*)malloc(width * height * winfd.bytes);
  SH_RETURN_ERR_IF(!pixels, VG_OUT_OF_MEMORY_ERROR, SH_NO_RETVAL);

  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(sx, sy, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
  
  shCopyPixels(data, dataFormat, dataStride,
               pixels, winfd.vgformat, -1,
               width, height, width, height,
               0, 0, 0, 0, width, height);

  free(pixels);
  
  VG_RETURN(VG_NO_RETVAL);
}

/*----------------------------------------------------------
 * Copies a rectangle area of pixels of size (width,height)
 * from window surface at source coordinates (sx,sy) to
 * windows surface at destination cordinates (dx,dy)
 *---------------------------------------------------------*/

VG_API_CALL void vgCopyPixels(VGint dx, VGint dy,
                              VGint sx, VGint sy,
                              VGint width, VGint height)
{
  VG_GETCONTEXT(VG_NO_RETVAL);
  
  VG_RETURN_ERR_IF(width <= 0 || height <= 0,
                   VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);
  
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glRasterPos2i(dx, dy);
  glCopyPixels(sx, sy, width, height, GL_COLOR);
  glRasterPos2i(0, 0);
  
  VG_RETURN(VG_NO_RETVAL);
}

VG_API_CALL VGImage vgChildImage(VGImage parent,
                                 VGint x, VGint y, VGint width, VGint height)
{
  return VG_INVALID_HANDLE;
}

VG_API_CALL VGImage vgGetParent(VGImage image)
{
  return VG_INVALID_HANDLE;
}

VG_API_CALL void vgColorMatrix(VGImage dst, VGImage src,
                               const VGfloat * matrix)
{
}

VG_API_CALL void vgConvolve(VGImage dst, VGImage src,
                            VGint kernelWidth, VGint kernelHeight,
                            VGint shiftX, VGint shiftY,
                            const VGshort * kernel,
                            VGfloat scale,
                            VGfloat bias,
                            VGTilingMode tilingMode)
{
}

VG_API_CALL void vgSeparableConvolve(VGImage dst, VGImage src,
                                     VGint kernelWidth,
                                     VGint kernelHeight,
                                     VGint shiftX, VGint shiftY,
                                     const VGshort * kernelX,
                                     const VGshort * kernelY,
                                     VGfloat scale,
                                     VGfloat bias,
                                     VGTilingMode tilingMode)
{
}

VG_API_CALL void vgGaussianBlur(VGImage dst, VGImage src,
                                VGfloat stdDeviationX,
                                VGfloat stdDeviationY,
                                VGTilingMode tilingMode)
{
}

VG_API_CALL void vgLookup(VGImage dst, VGImage src,
                          const VGubyte * redLUT,
                          const VGubyte * greenLUT,
                          const VGubyte * blueLUT,
                          const VGubyte * alphaLUT,
                          VGboolean outputLinear,
                          VGboolean outputPremultiplied)
{
}

VG_API_CALL void vgLookupSingle(VGImage dst, VGImage src,
                                const VGuint * lookupTable,
                                VGImageChannel sourceChannel,
                                VGboolean outputLinear,
                                VGboolean outputPremultiplied)
{
}
