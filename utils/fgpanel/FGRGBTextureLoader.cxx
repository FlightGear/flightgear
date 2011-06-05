//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License as
//  published by the Free Software Foundation; either version 2 of the
//  License, or (at your option) any later version.
// 
//  This program is distributed in the hope that it will be useful, but
//  WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// From the OpenSceneGraph distribution ReaderWriterRGB.cpp
// Reader for sgi's .rgb format.
// specification can be found at http://local.wasp.uwa.edu.au/~pbourke/dataformats/sgirgb/sgiversion.html

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#include "FGRGBTextureLoader.hxx"
#if defined (SG_MAC)
#include <OpenGL/gl.h>
#include <GLUT/glut.h>
#else
#include <GL/gl.h>
#include <GL/glut.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fstream>

typedef struct _rawImageRec
{
    unsigned short imagic;
    unsigned short type;
    unsigned short dim;
    unsigned short sizeX, sizeY, sizeZ;
    unsigned long min, max;
    unsigned long wasteBytes;
    char name[80];
    unsigned long colorMap;
    std::istream *file;
    unsigned char *tmp, *tmpR, *tmpG, *tmpB, *tmpA;
    unsigned long rleEnd;
    GLuint *rowStart;
    GLint *rowSize;
    GLenum swapFlag;
    short bpc;
  
    typedef unsigned char * BytePtr;

    bool needsBytesSwapped()
    {
        union {
            int testWord;
            char testByte[sizeof(int)];
        }endianTest; 
        endianTest.testWord = 1;
        if( endianTest.testByte[0] == 1 )
            return true;
        else
            return false;
    }

    template <class T>
    inline void swapBytes(  T &s )
    {
        if( sizeof( T ) == 1 ) 
            return;

        T d = s;
        BytePtr sptr = (BytePtr)&s;
        BytePtr dptr = &(((BytePtr)&d)[sizeof(T)-1]);

        for( unsigned int i = 0; i < sizeof(T); i++ )
            *(sptr++) = *(dptr--);
    }

    void swapBytes()
    {
        swapBytes( imagic );
        swapBytes( type );
        swapBytes( dim );
        swapBytes( sizeX );
        swapBytes( sizeY );
        swapBytes( sizeZ );
        swapBytes( wasteBytes );
        swapBytes( min );
        swapBytes( max );
        swapBytes( colorMap );
    }
} rawImageRec;

static void ConvertShort(unsigned short *array, long length)
{
    unsigned long b1, b2;
    unsigned char *ptr;

    ptr = (unsigned char *)array;
    while (length--)
    {
        b1 = *ptr++;
        b2 = *ptr++;
        *array++ = (unsigned short) ((b1 << 8) | (b2));
    }
}

static void ConvertLong(GLuint *array, long length)
{
    unsigned long b1, b2, b3, b4;
    unsigned char *ptr;

    ptr = (unsigned char *)array;
    while (length--)
    {
        b1 = *ptr++;
        b2 = *ptr++;
        b3 = *ptr++;
        b4 = *ptr++;
        *array++ = (b1 << 24) | (b2 << 16) | (b3 << 8) | (b4);
    }
}


static void RawImageClose(rawImageRec *raw)
{
    if (raw)
    {
        
        if (raw->tmp) delete [] raw->tmp;
        if (raw->tmpR) delete [] raw->tmpR;
        if (raw->tmpG) delete [] raw->tmpG;
        if (raw->tmpB) delete [] raw->tmpB;
        if (raw->tmpA) delete [] raw->tmpA;

        if (raw->rowStart) delete [] raw->rowStart;        
        if (raw->rowSize) delete [] raw->rowSize;        

        delete raw;
    }
}


static rawImageRec *RawImageOpen(std::istream& fin)
{
    union
    {
        int testWord;
        char testByte[4];
    } endianTest;
    rawImageRec *raw;
    int x;

    raw = new rawImageRec;
    if (raw == NULL)
    {
//        notify(WARN)<< "Out of memory!"<< std::endl;
        return NULL;
    }

    //Set istream pointer
    raw->file = &fin;

    endianTest.testWord = 1;
    if (endianTest.testByte[0] == 1)
    {
        raw->swapFlag = GL_TRUE;
    }
    else
    {
        raw->swapFlag = GL_FALSE;
    }

    fin.read((char*)raw,12);
    if (!fin.good())
        return NULL;

    if (raw->swapFlag)
    {
        ConvertShort(&raw->imagic, 6);
    }

    raw->tmp = raw->tmpR = raw->tmpG = raw->tmpB = raw->tmpA = 0L;
    raw->rowStart = 0;
    raw->rowSize = 0;
    raw->bpc = (raw->type & 0x00FF);

    raw->tmp = new unsigned char [raw->sizeX*256*raw->bpc];
    if (raw->tmp == NULL )
    {
//        notify(FATAL)<< "Out of memory!"<< std::endl;
        RawImageClose(raw);
        return NULL;
    }

    if( raw->sizeZ >= 1 )
    {
        if( (raw->tmpR = new unsigned char [raw->sizeX*raw->bpc]) == NULL )
        {
//            notify(FATAL)<< "Out of memory!"<< std::endl;
            RawImageClose(raw);
            return NULL;
        }
    }
    if( raw->sizeZ >= 2 )
    {
        if( (raw->tmpG = new unsigned char [raw->sizeX*raw->bpc]) == NULL )
        {
//            notify(FATAL)<< "Out of memory!"<< std::endl;
            RawImageClose(raw);
            return NULL;
        }
    }
    if( raw->sizeZ >= 3 )
    {
        if( (raw->tmpB = new unsigned char [raw->sizeX*raw->bpc]) == NULL )
        {
//            notify(FATAL)<< "Out of memory!"<< std::endl;
            RawImageClose(raw);
            return NULL;
        }
    }
    if (raw->sizeZ >= 4)
    {
        if( (raw->tmpA = new unsigned char [raw->sizeX*raw->bpc]) == NULL )
        {
//            notify(FATAL)<< "Out of memory!"<< std::endl;
            RawImageClose(raw);
            return NULL;
        }
    }
    
    if ((raw->type & 0xFF00) == 0x0100)
    {
        unsigned int ybyz = raw->sizeY * raw->sizeZ;
        if ( (raw->rowStart = new GLuint [ybyz]) == NULL )
        {
//            notify(FATAL)<< "Out of memory!"<< std::endl;
            RawImageClose(raw);
            return NULL;
        }

        if ( (raw->rowSize = new GLint [ybyz]) == NULL )
        {
//            notify(FATAL)<< "Out of memory!"<< std::endl;
            RawImageClose(raw);
            return NULL;
        }
        x = ybyz * sizeof(GLuint);
        raw->rleEnd = 512 + (2 * x);
                fin.seekg(512,std::ios::beg);
        fin.read((char*)raw->rowStart,x);
        fin.read((char*)raw->rowSize,x);
        if (raw->swapFlag)
        {
            ConvertLong(raw->rowStart, (long) (x/sizeof(GLuint)));
            ConvertLong((GLuint *)raw->rowSize, (long) (x/sizeof(GLint)));
        }
    }
    return raw;
}


static void RawImageGetRow(rawImageRec *raw, unsigned char *buf, int y, int z)
{
    unsigned char *iPtr, *oPtr;
    unsigned short pixel;
    int count, done = 0;
    unsigned short *tempShort;

    if ((raw->type & 0xFF00) == 0x0100)
    {
        raw->file->seekg((long) raw->rowStart[y+z*raw->sizeY], std::ios::beg);
        raw->file->read((char*)raw->tmp, (unsigned int)raw->rowSize[y+z*raw->sizeY]);

        iPtr = raw->tmp;
        oPtr = buf;
        while (!done)
        {
            if (raw->bpc == 1)
                pixel = *iPtr++;
            else
            {
                tempShort = reinterpret_cast<unsigned short*>(iPtr);
                pixel = *tempShort;
                tempShort++;
                iPtr = reinterpret_cast<unsigned char *>(tempShort);
            }
            
            if(raw->bpc != 1)
                ConvertShort(&pixel, 1);

            count = (int)(pixel & 0x7F);
            
            // limit the count value to the remiaing row size
            if (oPtr + count*raw->bpc > buf + raw->sizeX*raw->bpc)
            {
                count = ( (buf + raw->sizeX*raw->bpc) - oPtr ) / raw->bpc;
            }
                
            if (count<=0)
            {
                done = 1;
                return;
            }
            
            if (pixel & 0x80)
            {
                while (count--)
                {
                    if(raw->bpc == 1)
                        *oPtr++ = *iPtr++;
                    else{
                        tempShort = reinterpret_cast<unsigned short*>(iPtr);
                        pixel = *tempShort;
                        tempShort++;
                        iPtr = reinterpret_cast<unsigned char *>(tempShort);
                        
                        ConvertShort(&pixel, 1);

                        tempShort = reinterpret_cast<unsigned short*>(oPtr);
                        *tempShort = pixel;
                        tempShort++;
                        oPtr = reinterpret_cast<unsigned char *>(tempShort);
                    }
                }
            }
            else
            {
                if (raw->bpc == 1)
                {
                    pixel = *iPtr++;
                }
                else
                {
                    tempShort = reinterpret_cast<unsigned short*>(iPtr);
                    pixel = *tempShort;
                    tempShort++;
                    iPtr = reinterpret_cast<unsigned char *>(tempShort);
                }
                if(raw->bpc != 1)
                    ConvertShort(&pixel, 1);
                while (count--)
                {
                    if(raw->bpc == 1)
                        *oPtr++ = pixel;
                    else
                    {
                        tempShort = reinterpret_cast<unsigned short*>(oPtr);
                        *tempShort = pixel;
                        tempShort++;
                        oPtr = reinterpret_cast<unsigned char *>(tempShort);
                    }
                }
            }
        }
    }
    else
    {
        raw->file->seekg(512+(y*raw->sizeX*raw->bpc)+(z*raw->sizeX*raw->sizeY*raw->bpc),std::ios::beg);
        raw->file->read((char*)buf, raw->sizeX*raw->bpc);
        if(raw->swapFlag && raw->bpc != 1){
            ConvertShort(reinterpret_cast<unsigned short*>(buf), raw->sizeX);
        }
    }
}


static void RawImageGetData(rawImageRec *raw, unsigned char **data )
{
    unsigned char *ptr;
    int i, j;
    unsigned short *tempShort;

    //     // round the width to a factor 4
    //     int width = (int)(floorf((float)raw->sizeX/4.0f)*4.0f);
    //     if (width!=raw->sizeX) width += 4;

    // byte aligned.
    
//    osg::notify(osg::INFO)<<"raw->sizeX = "<<raw->sizeX<<std::endl;
//    osg::notify(osg::INFO)<<"raw->sizeY = "<<raw->sizeY<<std::endl;
//    osg::notify(osg::INFO)<<"raw->sizeZ = "<<raw->sizeZ<<std::endl;
//    osg::notify(osg::INFO)<<"raw->bpc = "<<raw->bpc<<std::endl;
    
    *data = new unsigned char [(raw->sizeX)*(raw->sizeY)*(raw->sizeZ)*(raw->bpc)];

    ptr = *data;
    for (i = 0; i < (int)(raw->sizeY); i++)
    {
        if( raw->sizeZ >= 1 )
            RawImageGetRow(raw, raw->tmpR, i, 0);
        if( raw->sizeZ >= 2 )
            RawImageGetRow(raw, raw->tmpG, i, 1);
        if( raw->sizeZ >= 3 )
            RawImageGetRow(raw, raw->tmpB, i, 2);
        if( raw->sizeZ >= 4 )
            RawImageGetRow(raw, raw->tmpA, i, 3);
        for (j = 0; j < (int)(raw->sizeX); j++)
        {
          if(raw->bpc == 1){
            if( raw->sizeZ >= 1 )
                *ptr++ = *(raw->tmpR + j);
            if( raw->sizeZ >= 2 )
                *ptr++ = *(raw->tmpG + j);
            if( raw->sizeZ >= 3 )
                *ptr++ = *(raw->tmpB + j);
            if( raw->sizeZ >= 4 )
                *ptr++ = *(raw->tmpA + j);
          }else{
            if( raw->sizeZ >= 1 )
            {
                tempShort = reinterpret_cast<unsigned short*>(ptr);
                *tempShort = *(reinterpret_cast<unsigned short*>(raw->tmpR) + j);
                tempShort++;
                ptr = reinterpret_cast<unsigned char *>(tempShort);
            }
            if( raw->sizeZ >= 2 )
            {
                tempShort = reinterpret_cast<unsigned short*>(ptr);
                *tempShort = *(reinterpret_cast<unsigned short*>(raw->tmpG) + j);
                tempShort++;
                ptr = reinterpret_cast<unsigned char *>(tempShort);
            }
            if( raw->sizeZ >= 3 )
            {
                tempShort = reinterpret_cast<unsigned short*>(ptr);
                *tempShort = *(reinterpret_cast<unsigned short*>(raw->tmpB) + j);
                tempShort++;
                ptr = reinterpret_cast<unsigned char *>(tempShort);
            }
            if( raw->sizeZ >= 4 )
            {
                tempShort = reinterpret_cast<unsigned short*>(ptr);
                *tempShort = *(reinterpret_cast<unsigned short*>(raw->tmpA) + j);
                tempShort++;
                ptr = reinterpret_cast<unsigned char *>(tempShort);
            }
          }
        }
        //         // pad the image width with blanks to bring it up to the rounded width.
        //         for(;j<width;++j) *ptr++ = 0;
    }
}


//            supportsExtension("rgb","rgb image format");
//            supportsExtension("rgba","rgba image format");
//            supportsExtension("sgi","sgi image format");
//            supportsExtension("int","int image format");
//            supportsExtension("inta","inta image format");
//            supportsExtension("bw","bw image format");
        
        GLuint readRGBStream(std::istream& fin)
        {
            rawImageRec *raw;

            if( (raw = RawImageOpen(fin)) == NULL )
            {
                return 0;
            }

            int s = raw->sizeX;
            int t = raw->sizeY;
//            int r = 1;

        #if 0
            int internalFormat = raw->sizeZ == 3 ? GL_RGB5 :
            raw->sizeZ == 4 ? GL_RGB5_A1 : GL_RGB;
        #else
//            int internalFormat = raw->sizeZ;
        #endif
            unsigned int pixelFormat =
                raw->sizeZ == 1 ? GL_LUMINANCE :
                raw->sizeZ == 2 ? GL_LUMINANCE_ALPHA :
                raw->sizeZ == 3 ? GL_RGB :
                raw->sizeZ == 4 ? GL_RGBA : (GLenum)-1;
            GLint component = raw->sizeZ;

            unsigned int dataType = raw->bpc == 1 ? GL_UNSIGNED_BYTE :
              GL_UNSIGNED_SHORT;

            unsigned char *data;
            RawImageGetData(raw, &data);
            RawImageClose(raw);

  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  gluBuild2DMipmaps( GL_TEXTURE_2D, component, s, t, pixelFormat, dataType, (GLvoid*)data );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR );

            delete []data;
            return texture;
        }

GLuint FGRGBTextureLoader::loadTexture( const std::string & filename )
{
  GLuint texture = NOTEXTURE;
  std::ifstream istream(filename.c_str(), std::ios::in | std::ios::binary );
  texture = readRGBStream(istream);
  istream.close();
  return texture;
}

