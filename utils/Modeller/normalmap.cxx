/*
 * Create normal map textures from regular textures.
 * Created by: Erik Hofman
 *
 * This file is in public domain.
 */
#include <simgear/compiler.h>

#include <string.h>
#include <stdlib.h>
#include <osg/GL>

#include "texture.hxx"


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
   if (!strncmp(opt, "-help", sz)) {
      printf("usage:\n  normalmap [-c=contrast] [-b=brightness]");
      printf(" --i=file [--o=file]\n\n");
      exit(0);
   }

   return 1;
}

int main (int argc, char **argv)
{
   int i;

   for (i=1; i<argc;)
      i += parse_option(argv, i);

   if ( !texture_file )
   {
      printf("Error: texture file not specified.\n");
      printf("usage:\n  normalmap [-c=contrast] [-b=brightness]");
      printf(" --i=file [--o=file]\n\n");
      return -1;
   }

   texture.read_rgb_texture(texture_file);
   if ( !texture.texture() )
   {
      printf("Error: unable to process input file: %s\n", texture_file);
      printf("       (%s)\n\n", texture.err_str());
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

   texture.make_normalmap(brightness, contrast);
   texture.write_texture(normalmap_file);

   free( normalmap_file );
   free( texture_file );

   return 0;
}
