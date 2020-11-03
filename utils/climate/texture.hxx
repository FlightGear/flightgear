/*
 * \file texture.hxx
 * Texture manipulation routines
 *
 * Copyright (c) Mark J. Kilgard, 1997.
 * Code added in april 2003 by Erik Hofman
 *
 * This program is freely distributable without licensing fees 
 * and is provided without guarantee or warrantee expressed or 
 * implied. This program is -not- in the public domain.
 */

#ifndef __SG_TEXTURE_HXX
#define __SG_TEXTURE_HXX 1

#include <simgear/compiler.h>
#include <osg/GL>
#include <zlib.h>

#include <plib/sg.h>

/**
 * A class to encapsulate all the info surrounding an OpenGL texture
 * and also query various info and load various file formats
 */
class SGTexture {

private:

    GLuint texture_id;
    GLubyte *texture_data;

    GLsizei texture_width;
    GLsizei texture_height;
    GLsizei num_colors;

    void resize(unsigned int width = 256, unsigned int height = 256);

    const char *errstr;

protected:

    FILE *file;
    typedef struct _ImageRec {
        _ImageRec(void) : tmp(0), rowStart(0), rowSize(0) {}
        unsigned short imagic;
        unsigned short type;
        unsigned short dim;
        unsigned short xsize, ysize, zsize;
        unsigned int min, max;
        unsigned int wasteBytes;
        char name[80];
        unsigned long colorMap;
        gzFile file;
        GLubyte *tmp;
        unsigned long rleEnd;
        unsigned int *rowStart;
        int *rowSize;
    } ImageRec;

    void ConvertUint(unsigned *array, unsigned int length);
    void ConvertShort(unsigned short *array, unsigned int length);
    void rgbtorgb(GLubyte *r, GLubyte *g, GLubyte *b, GLubyte *l, int n);
    void rgbatorgba(GLubyte *r, GLubyte *g, GLubyte *b, GLubyte *a,
                    GLubyte *l, int n);

    ImageRec *ImageOpen(const char *fileName);
    ImageRec *ImageWriteOpen(const char *fileName);
    ImageRec *RawImageOpen(const char *fileName);
    void ImageClose(ImageRec *image);
    void ImageGetRow(ImageRec *image, GLubyte *buf, int y, int z);
    void ImagePutRow(ImageRec *image, GLubyte *buf, int y, int z);

    inline void free_id() {
#ifdef GL_VERSION_1_1
        glDeleteTextures(1, &texture_id);
#else
        glDeleteTexturesEXT(1, &texture_id);
#endif
        texture_id = 0;
    }


public:

    SGTexture();
    SGTexture(unsigned int width, unsigned int height);
    ~SGTexture();

    /* Copyright (c) Mark J. Kilgard, 1997.  */
    void read_alpha_texture(const char *name);
    void read_rgb_texture(const char *name);
    void read_rgba_texture(const char *name);
    void read_raw_texture(const char *name);
    void read_r8_texture(const char *name);
    void write_texture(const char *name);

    inline bool usable() { return (texture_id > 0) ? true : false; }

    inline GLuint id() { return texture_id; }
    inline GLubyte *texture() { return texture_data; }
    inline void set_data(GLubyte *data) { texture_data = data; }
    inline void set_colors(int c) { num_colors = c; }

    inline int width() { return texture_width; }
    inline int height() { return texture_height; }
    inline int colors() { return num_colors; }

    // dynamic texture functions.
    // everything drawn to the OpenGL screen after prepare is
    // called and before finish is called will be included
    // in the new texture.
    void prepare(unsigned int width = 256, unsigned int height = 256);
    void finish(unsigned int width, unsigned int height);

    // texture pixel manipulation functions.
    void set_pixel(GLuint x, GLuint y, GLubyte *c);
    GLubyte *get_pixel(GLuint x, GLuint y);

    void bind();
    inline void select(bool keep_data = false) {
        glTexImage2D( GL_TEXTURE_2D, 0, num_colors,
                      texture_width, texture_height, 0,
                      (num_colors==1)?GL_LUMINANCE:(num_colors==3)?GL_RGB:GL_RGBA, GL_UNSIGNED_BYTE, texture_data );

        if (!keep_data) {
            delete[] texture_data;
            texture_data = 0;
        }
    }

    // Nowhere does it say that resident textures have to be in video memory!
    // NVidia's OpenGL drivers implement glAreTexturesResident() correctly,
    // but the Matrox G400, for example, doesn't.
    inline bool is_resident() {
        GLboolean is_res;
        glAreTexturesResident(1, &texture_id, &is_res);
        return is_res != 0;
    }

    inline const char *err_str() { return errstr; }
    inline void clear_err_str() { errstr = ""; }

    void make_maxcolorwindow();
    void make_grayscale(float contrast = 1.0);
    void make_monochrome(float contrast = 1.0,
                         GLubyte r=255, GLubyte g=255, GLubyte b=255);
    void make_normalmap(float brightness = 1.0, float contrast = 1.0);
    void make_bumpmap(float brightness = 1.0, float contrast = 1.0);
};

#endif

