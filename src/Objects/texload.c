
/* texture.c - by David Blythe, SGI */

/* texload is a simplistic routine for reading an SGI .rgb image file. */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h> 
#include <string.h>

#include <simgear/fg_zlib.h>

#include "texload.h"
#include "colours.h"

typedef struct _ImageRec {
    unsigned short imagic;
    unsigned short type;
    unsigned short dim;
    unsigned short xsize, ysize, zsize;
    unsigned int min, max;
    unsigned int wasteBytes;
    char name[80];
    unsigned long colorMap;
    fgFile file;
    unsigned char *tmp;
    unsigned long rleEnd;
    unsigned int *rowStart;
    int *rowSize;
} ImageRec;

void
rgbtorgb(unsigned char *r,unsigned char *g,unsigned char *b,unsigned char *l,int n) {
    while(n--) {
        l[0] = r[0];
        l[1] = g[0];
        l[2] = b[0];
        l += 3; r++; g++; b++;
    }
}

static void
ConvertShort(unsigned short *array, unsigned int length) {
    unsigned short b1, b2;
    unsigned char *ptr;

    ptr = (unsigned char *)array;
    while (length--) {
        b1 = *ptr++;
        b2 = *ptr++;
        *array++ = (b1 << 8) | (b2);
    }
}

static void
ConvertUint(unsigned *array, unsigned int length) {
    unsigned int b1, b2, b3, b4;
    unsigned char *ptr;

    ptr = (unsigned char *)array;
    while (length--) {
        b1 = *ptr++;
        b2 = *ptr++;
        b3 = *ptr++;
        b4 = *ptr++;
        *array++ = (b1 << 24) | (b2 << 16) | (b3 << 8) | (b4);
    }
}

static ImageRec *ImageOpen(const char *fileName)
{
     union {
       int testWord;
       char testByte[4];
     } endianTest;

    ImageRec *image;
    int swapFlag;
    int x;

    endianTest.testWord = 1;
    if (endianTest.testByte[0] == 1) {
        swapFlag = 1;
    } else {
        swapFlag = 0;
    }

    image = (ImageRec *)malloc(sizeof(ImageRec));
    if (image == NULL) {
        fprintf(stderr, "Out of memory!\n");
        exit(1);
    }
    if ((image->file = fgopen(fileName, "rb")) == NULL) {
      return NULL;
    }

    // fread(image, 1, 12, image->file);
    fgread(image->file, image, 12);

    if (swapFlag) {
        ConvertShort(&image->imagic, 6);
    }

    image->tmp = (unsigned char *)malloc(image->xsize*256);
    if (image->tmp == NULL) {
        fprintf(stderr, "\nOut of memory!\n");
        exit(1);
    }

    if ((image->type & 0xFF00) == 0x0100) {
        x = image->ysize * image->zsize * (int) sizeof(unsigned);
        image->rowStart = (unsigned *)malloc(x);
        image->rowSize = (int *)malloc(x);
        if (image->rowStart == NULL || image->rowSize == NULL) {
            fprintf(stderr, "\nOut of memory!\n");
            exit(1);
        }
        image->rleEnd = 512 + (2 * x);
        fgseek(image->file, 512, SEEK_SET);
        // fread(image->rowStart, 1, x, image->file);
	fgread(image->file, image->rowStart, x);
        // fread(image->rowSize, 1, x, image->file);
	fgread(image->file, image->rowSize, x);
        if (swapFlag) {
            ConvertUint(image->rowStart, x/(int) sizeof(unsigned));
            ConvertUint((unsigned *)image->rowSize, x/(int) sizeof(int));
        }
    }
    return image;
}

static void
ImageClose(ImageRec *image) {
    fgclose(image->file);
    free(image->tmp);
    free(image);
}

static void
ImageGetRow(ImageRec *image, unsigned char *buf, int y, int z) {
    unsigned char *iPtr, *oPtr, pixel;
    int count;

    if ((image->type & 0xFF00) == 0x0100) {
        fgseek(image->file, (long) image->rowStart[y+z*image->ysize], SEEK_SET);
        // fread(image->tmp, 1, (unsigned int)image->rowSize[y+z*image->ysize],
        //      image->file);
	fgread(image->file, image->tmp, 
	       (unsigned int)image->rowSize[y+z*image->ysize]);

        iPtr = image->tmp;
        oPtr = buf;
        for (;;) {
            pixel = *iPtr++;
            count = (int)(pixel & 0x7F);
            if (!count) {
                return;
            }
            if (pixel & 0x80) {
                while (count--) {
                    *oPtr++ = *iPtr++;
                }
            } else {
                pixel = *iPtr++;
                while (count--) {
                    *oPtr++ = pixel;
                }
            }
        }
    } else {
        fgseek(image->file, 512+(y*image->xsize)+(z*image->xsize*image->ysize),
              SEEK_SET);
        // fread(buf, 1, image->xsize, image->file);
	fgread(image->file, buf, image->xsize);
    }
}

GLubyte *
read_alpha_texture(const char *name, int *width, int *height)
{
    unsigned char *base, *lptr;
    ImageRec *image;
    int y;

    image = ImageOpen(name);
    if(!image) {
        return NULL;
    }

    (*width)=image->xsize;
    (*height)=image->ysize;

    printf("image->zsize = %d\n", image->zsize);

    if (image->zsize != 1) {
      ImageClose(image);
      return NULL;
    }

    base = (unsigned char *)malloc(image->xsize*image->ysize*sizeof(unsigned char));
    lptr = base;
    for(y=0; y<image->ysize; y++) {
        ImageGetRow(image,lptr,y,0);
        lptr += image->xsize;
    }
    ImageClose(image);

    return (unsigned char *) base;
}

GLubyte *
read_rgb_texture(const char *name, int *width, int *height)
{
    unsigned char *base, *ptr;
    unsigned char *rbuf, *gbuf, *bbuf, *abuf;
    ImageRec *image;
    int y;

    image = ImageOpen(name);
    
    if(!image)
        return NULL;
    (*width)=image->xsize;
    (*height)=image->ysize;
    if (image->zsize != 3 && image->zsize != 4) {
      ImageClose(image);
      return NULL;
    }

    base = (unsigned char*)malloc(image->xsize*image->ysize*sizeof(unsigned int)*3);
    rbuf = (unsigned char *)malloc(image->xsize*sizeof(unsigned char));
    gbuf = (unsigned char *)malloc(image->xsize*sizeof(unsigned char));
    bbuf = (unsigned char *)malloc(image->xsize*sizeof(unsigned char));
    abuf = (unsigned char *)malloc(image->xsize*sizeof(unsigned char));
    if(!base || !rbuf || !gbuf || !bbuf || !abuf) {
      if (base) free(base);
      if (rbuf) free(rbuf);
      if (gbuf) free(gbuf);
      if (bbuf) free(bbuf);
      if (abuf) free(abuf);
      return NULL;
    }
    ptr = base;
    for(y=0; y<image->ysize; y++) {
        if(image->zsize == 4) {
            ImageGetRow(image,rbuf,y,0);
            ImageGetRow(image,gbuf,y,1);
            ImageGetRow(image,bbuf,y,2);
            ImageGetRow(image,abuf,y,3);  /* Discard. */
            rgbtorgb(rbuf,gbuf,bbuf,ptr,image->xsize);
            ptr += (image->xsize * 3);
        } else {
            ImageGetRow(image,rbuf,y,0);
            ImageGetRow(image,gbuf,y,1);
            ImageGetRow(image,bbuf,y,2);
            rgbtorgb(rbuf,gbuf,bbuf,ptr,image->xsize);
            ptr += (image->xsize * 3);
        }
    }
    ImageClose(image);
    free(rbuf);
    free(gbuf);
    free(bbuf);
    free(abuf);

    return (GLubyte *) base;
}


static ImageRec *RawImageOpen(const char *fileName)
{
     union {
       int testWord;
       char testByte[4];
     } endianTest;

    ImageRec *image;
    int swapFlag;

    endianTest.testWord = 1;
    if (endianTest.testByte[0] == 1) {
        swapFlag = 1;
    } else {
        swapFlag = 0;
    }

    image = (ImageRec *)malloc(sizeof(ImageRec));
    if (image == NULL) {
        fprintf(stderr, "Out of memory!\n");
        exit(1);
    }
    if ((image->file = fgopen(fileName, "rb")) == NULL) {
      return NULL;
    }

    fgread(image->file, image, 12);

    if (swapFlag) {
        ConvertShort(&image->imagic, 6);
    }

    
	image->tmp = (unsigned char *)malloc(1);	//just allocate a pseudo value as I'm too lazy to change ImageClose()...
    if (image->tmp == NULL) {
        fprintf(stderr, "\nOut of memory!\n");
        exit(1);
    }

    return image;
}


GLubyte *
read_raw_texture(const char *name, int *width, int *height)
{
    unsigned char *base, *ptr;
    ImageRec *image;
    int y;

    image = RawImageOpen(name);
    
    if(!image)
        return NULL;
    (*width)=256; 
    (*height)=256; 

    base = (unsigned char*)malloc(256*256*sizeof(unsigned char)*3);
    if(!base) {
      if (base) free(base);
      return NULL;
    }
    ptr = base;
    for(y=0; y<256; y++) {
		fgread(image->file, ptr, 256*3);
		ptr+=256*3;
    }
    ImageClose(image);

    return (GLubyte *) base;
}


GLubyte *
read_r8_texture(const char *name, int *width, int *height)
{
    unsigned char *base, *ptr;
    ImageRec *image;
    int xy;
	unsigned char c[1];

    image = RawImageOpen(name);	//it wouldn't make sense to write a new function...
    
    if(!image)
        return NULL;
    (*width)=256; 
    (*height)=256; 

    base = (unsigned char*)malloc(256*256*sizeof(unsigned char)*3);
    if(!base) {
      if (base) free(base);
      return NULL;
    }
    ptr = base;
    for(xy=0; xy<(256*256); xy++) {
		fgread(image->file,c,1);
		ptr[0]=msfs_colour[c[0]][0];	//look in the table for the right colours
		ptr[1]=msfs_colour[c[0]][1];
		ptr[2]=msfs_colour[c[0]][2];
		
		ptr+=3;
    }
    ImageClose(image);

    return (GLubyte *) base;
}
