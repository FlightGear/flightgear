/*
 * Create normal map textures from regular textures.
 * Created by: Erik Hofman
 *
 * This file is in public domain.
 */

#include <string.h>
#include <GL/gl.h>
#include <simgear/screen/texture.hxx>


static float contrast = 1.0;
static float brightness = 1.0;
static char *texture_file = NULL, *normalmap_file = NULL;
static SGTexture texture;

int parse_option(char **args, int n) {
   char *opt, *arg;
   int sz, ret=1;

   opt = args[n];
   if (*(opt+1) == '-')
      opt++;

   if ((arg = strchr(opt, '=')) != NULL)
      *arg++ = 0;

   else {
      ret++;
      arg = args[n+1];
   }

   sz = strlen(opt);
   if (!strncmp(opt, "-help", sz)) {
      printf("usage:\n  normalmap [-c=contrast] [-b=brightness]");
      printf(" --t=file [--o=file]\n");
      exit(0);
   }
   if (!strncmp(opt, "-contrast", sz)) {
      contrast = atof(arg);
      return ret;
   }
   if (!strncmp(opt, "-brightness", sz)) {
      brightness = atof(arg);
      return ret;
   }
   if (!strncmp(opt, "-texture", sz) ||
       !strncmp(opt, "-input", sz)) {
      texture_file = strdup(arg);
      return ret;
   }
   if (!strncmp(opt, "-normalmap", sz) ||
       !strncmp(opt, "-output", sz)) {
      normalmap_file = strdup(arg);
      return ret;
   }

   return 1;
}

GLubyte *make_map( GLubyte *data, int width, int height, int colors )
{
   GLubyte *map = (GLubyte *)malloc (width * height * 3);
   
   for (int y=0; y<height; y++)
      for (int x=0; x<width; x++)
      {
         int mpos = (x + y*width)*3;
         int dpos = (x + y*width)*colors;

         int xp1 = (x < (width-1)) ? x+1 : 0;
         int yp1 = (y < (height-1)) ? y+1 : 0;
         int posxp1 = (xp1 + y*width)*colors;
         int posyp1 = (x + yp1*width)*colors;
         
         map[mpos+0] = (128+(data[posxp1]-data[dpos])/2);
         map[mpos+1] = (128+(data[posyp1]-data[dpos])/2);
         map[mpos+2] = 128 + GLubyte(128*brightness);
      }

   return map;
}

int main (int argc, char **argv)
{
   int i;

   for (i=1; i<argc;)
      i += parse_option(argv, i);

   if ( !texture_file )
   {
      printf("Error: texture file not specified\n");
      return -1;
   }

   texture.read_rgb_texture(texture_file);
   if ( !texture.texture() )
   {
      printf("Error: unable to process input file: %s\n", texture_file);
      printf("       (%s)\n", texture.err_str());
      return -2;
   }

   if ( !normalmap_file )
   {
      int i;
      for (i=strlen(texture_file); i>=0; --i)
         if (texture_file[i] == '.')
            break;

      normalmap_file = (char *)malloc( i+8 );
      memcpy(normalmap_file, texture_file, i);
      memcpy(normalmap_file+i, "_n.rgb\0", 7);
   }

   // texture.make_grayscale();
   texture.make_normalmap();
   texture.write_texture(normalmap_file);

   free( normalmap_file );
   free( texture_file );

   return 0;
}
