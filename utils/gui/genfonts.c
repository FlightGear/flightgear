/*
 * A simple utility to generate the bitmap fonts to be used in freeglut.
 *
 * Copyright (c) 2005 Melchior FRANZ
 * Copyright (c) 1999-2000 by Pawel W. Olszta
 * Written by Pawel W. Olszta, <olszta@sourceforge.net>
 * Creation date: nie gru 26 21:52:36 CET 1999
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software")
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * PAWEL W. OLSZTA BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* we build our own alphabet ... see below */
unsigned char alphabet[256] = " abcdefghijklmnopqrstuwvxyzABCDEFGHIJKLMNOPQRSTUWVXYZ0123456789`~!@#$%^&*()-_=+[{}];:,.<>/?\\\"";
int alphabet_length = 0;

/* all undefined characters will get replaced by this one: */
char missing = '?';

FILE *out = NULL;
Display *display;

void write_font(char *fontname, char *x_fontname, int gap, int space)
{
	int i, lineWidth, maxWidth = 0, maxHeight = 0;
	XFontStruct *fontStruct = NULL;
	XGCValues contextValues;
	XImage *image = NULL;
	unsigned char *linebuffer;
	Pixmap buffer;
	GC context;

	fontStruct = XLoadQueryFont(display, x_fontname);

	if (fontStruct == NULL)
		fprintf(stderr, "ERROR: couldn't get font `%s' using local display", x_fontname);

	maxWidth  = fontStruct->max_bounds.rbearing - fontStruct->min_bounds.lbearing + gap;  // + gap?
	maxHeight = fontStruct->max_bounds.ascent   + fontStruct->max_bounds.descent;

	linebuffer = malloc(maxWidth);
	if (!linebuffer) {
		fprintf(stderr, "ERROR: couldn't allocate memory");
		exit(EXIT_FAILURE);
	}

	buffer = XCreatePixmap(display, RootWindow(display, DefaultScreen(display)),
			maxWidth, maxHeight, 1);

	context = XCreateGC(display, buffer, 0, &contextValues);

	XSetFont(display, context, fontStruct->fid);

	for (i = 0; i < alphabet_length; i++) {
		int x, y, start_x, stop_x;

		/* clear the context black */
		XSetForeground(display, context, 0x00);
		XFillRectangle(display, buffer, context, 0, 0, maxWidth, maxHeight);

		/* draw the characters white */
		XSetForeground(display, context, 0xff);

		/* draw the n-th character of the alphabet */
		XDrawString(display, buffer, context, -fontStruct->min_bounds.lbearing,
				fontStruct->max_bounds.ascent, (alphabet + i), 1);

		image = XGetImage(display, buffer, 0, 0, maxWidth, maxHeight, 1, XYPixmap);

		/* find the first non-empty column */
		start_x = -1;
		stop_x = -1;

		for (x = 0; x < maxWidth; x++)
			for (y = 0; y < maxHeight; y++)
				if ((XGetPixel(image, x, y) == 1) && (start_x == -1))
					start_x = x;

		/* find the last non-empty column */
		for (x = maxWidth - 1; x >= 0; x--)
			for (y = 0; y < maxHeight; y++)
				if ((XGetPixel(image, x, y) == 1) && (stop_x == -1))
					stop_x = x + 1;

		stop_x += gap;

		if (alphabet[i] == ' ')
			start_x = 0, stop_x = space;
		/* if the size is too small, enhance it a bit */
		else if (stop_x - start_x < 1)
			start_x = 0, stop_x = maxWidth - 1;

		fprintf(out, "static const GLubyte %s_Char_%03i[] = {%3i",
				fontname, (unsigned)alphabet[i], stop_x-start_x);

		for (y = maxHeight - 1; y >= 0; y--) {
			memset(linebuffer, 0, maxWidth);

			/* grab the rasterized character face into the line buffer */
			for (x = start_x, lineWidth = 0; x < stop_x; x++, lineWidth++)
				if (XGetPixel(image, x, y))
					linebuffer[lineWidth / 8] |= 1 << (7 - (lineWidth % 8));

			/* output the final line bitmap now */
			for (x = 0; x < (stop_x - start_x + 7) / 8; x++)
				fprintf(out, ",%3i", linebuffer[x]);
		}

		fprintf(out, "};\n");
		XDestroyImage(image);
	}

	fprintf(out, "\nstatic const GLubyte *%s_Char_Map[] = {\n\t", fontname);

	for (i = 1; i < 256; i++)
		fprintf(out, "%s_Char_%03i,%s", fontname, strchr(alphabet, (char)i)
				? (int)(unsigned char)i : (int)(unsigned char)missing,
				i % 4 ? " " : "\n\t");

	fprintf(out, "0};\n\n");
	fprintf(out, "const SFG_Font fgFont%s = {\"%s\", %i, %i, %s_Char_Map, 0.0f, 0.0f};\n\n",
			fontname, x_fontname, alphabet_length, maxHeight, fontname);

	fprintf(out, "static fntBitmapFont fnt%s(fgFont%s.Characters, 1,\n\t\t"
			"fgFont%s.Height, fgFont%s.xorig, fgFont%s.yorig);\n\n",
			fontname, fontname, fontname, fontname, fontname);

	XFreeGC(display, context);
	XFreePixmap(display, buffer);
	free(linebuffer);
}


int main(int argc, char **argv)
{
	char *filename = NULL;
	char *display_name = NULL;
	int i = 1;
	int A, B;

	int numfonts = 2;
	struct {
		char *name;
		char *spec;
		int gap;
		int space;
	} fontlist[] = { /* fontname, x_fontname, gap, space */
// fgfs
		{ "Helvetica14",  "-adobe-helvetica-medium-r-*-*-*-140-75-75-*-*-iso8859-1",       2, 5 },
		{ "Vera12B",      "-*-bitstream vera sans-bold-r-*-*-*-120-75-75-*-*-iso8859-1",   2, 4 },
		//"Helvetica14B", "-adobe-helvetica-bold-r-*-*-*-140-75-75-*-*-iso8859-1",
// freeglut/plib
		{ "Fixed8x13",    "-misc-fixed-medium-r-normal--13-120-75-75-C-80-iso8859-1",      0, 4 },
		{ "Fixed9x15",    "-misc-fixed-medium-r-normal--15-140-75-75-C-90-iso8859-1",      0, 4 },
		{ "Helvetica10",  "-adobe-helvetica-medium-r-normal--10-100-75-75-p-56-iso8859-1", 0, 4 },
		{ "Helvetica12",  "-adobe-helvetica-medium-r-normal--12-120-75-75-p-67-iso8859-1", 0, 5 },
		{ "Helvetica18",  "-adobe-helvetica-medium-r-normal--18-180-75-75-p-98-iso8859-1", 0, 7 },
		{ "TimesRoman10", "-adobe-times-medium-r-normal--10-100-75-75-p-54-iso8859-1",     0, 5 },
		{ "TimesRoman24", "-adobe-times-medium-r-normal--24-240-75-75-p-124-iso8859-1",    0, 8 },
	};

	/* initialize the alphabet's length */
	for (A = B = 0; A < 256; A++)
		if (isprint(A) || A >= 128)
			alphabet[B++] = A;
	alphabet[B] = 0;
	alphabet_length = strlen(alphabet);

	/* make sure that the no-character character is in the alphabet */
	if (!strchr(alphabet, missing))
		fprintf(stderr, "ERROR: the \"missing\" character '%c' not found in the alphabet '%s'",
				missing, alphabet);

	display_name = strdup(getenv("DISPLAY"));
	filename = strdup("font.c");

	/*
	 * process the command line arguments:
	 *
	 *       --display <DISPLAYNAME>    ... the display to connect to
	 *       --file    <FILENAME>       ... the destination file name
	 */
	while (i < argc) {
		if (!strcmp(argv[i], "--display")) {
			assert((i + 1) < argc);
			free(display_name);
			display_name = strdup(argv[++i]);

		} else if (!strcmp(argv[i], "--file")) {
			assert((i + 1) < argc);
			free(filename);
			filename = strdup(argv[++i]);
		}
		i++;
	}

	display = XOpenDisplay(display_name);
	assert(display != NULL);

	out = fopen(filename, "wt");
	assert(out != NULL);

	fprintf(out, "\n/*\n * Following fonts are defined in this file:\n *\n");

	for (i = 0; i < numfonts; i++)
		fprintf(out, " * %i. fgFont%s <%s>\n", i + 1, fontlist[i].name, fontlist[i].spec);

	fprintf(out, " */\n\n");

	for (i = 0; i < numfonts; i++)
		write_font(fontlist[i].name, fontlist[i].spec, fontlist[i].gap, fontlist[i].space);

	fclose(out);
	XCloseDisplay(display);
	free(filename);
	free(display_name);

	return EXIT_SUCCESS;
}


