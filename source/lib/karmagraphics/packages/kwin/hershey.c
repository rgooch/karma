/*LINTLIBRARY*/
/*  hershey.c

    This code provides KPixCanvas objects.

    Copyright (C) 1996  Richard Gooch

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    Richard Gooch may be reached by email at  karma-request@atnf.csiro.au
    The postal address is:
      Richard Gooch, c/o ATNF, P. O. Box 76, Epping, N.S.W., 2121, Australia.
*/

/*  This file contains all routines needed for manipulating a simple pixel
    canvas (window) independent of the graphics system in use. This file
    contains the hershey code.


    Written by      Richard Gooch   18-OCT-1996

    Last updated by Richard Gooch   21-OCT-1996


*/
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include <karma.h>
#define NEW_WIN_SCALE
#define KWIN_GENERIC_ONLY
#include <karma_kwin.h>
#include <karma_chs.h>
#include <karma_ch.h>
#include <karma_ex.h>
#include <karma_a.h>
#include <karma_r.h>
#include <karma_m.h>


#define NUM_FONTS 4
#define FIRST_CHAR 33
#define LAST_CHAR 128
#define NUM_CHAR (LAST_CHAR + 1 - FIRST_CHAR)

/*  Structure declarations  */

typedef struct
{
    double expand;
    double cdef;
    double cheight;
    char esc_char;
    double slant;
    double supfrac;
    signed char nstroke[NUM_FONTS][NUM_CHAR];
    signed char ladj[NUM_FONTS][NUM_CHAR];
    signed char radj[NUM_FONTS][NUM_CHAR];
    int pointer[NUM_FONTS][NUM_CHAR];
    signed char *font;
} StrokeFont;

struct mode_struct
{
    flag italic;
    int superscript_level;
    unsigned int font;
    double superscript_factor;
    double superscript_shift;
};


/*  Private data  */
static StrokeFont *main_font = NULL;

/*  Private functions  */
STATIC_FUNCTION (void initialise, (void) );


/*  Public functions follow  */

/*EXPERIMENTAL_FUNCTION*/
flag kwin_hersey_draw_string (KPixCanvas canvas, CONST char *string,
			      double x, double y, double angle,
			      unsigned long pixel_value,
			      double *width, double *height)
/*  [SUMMARY] Draw string using Hershey (stroke) font.
    <canvas> The KPixCanvas to draw onto. If this is NULL nothing is drawn.
    <string> The string to draw.
    <x> The starting horizontal position of the string.
    <y> The starting vertical position of the string.
    <angle> The rotation angle of the string in degrees.
    <pixel_value> The pixel value to use when drawing the string.
    <width> The width of the string in pixels is written here.
    <height> The height of the string in pixels is written here.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    flag change_mode, relo;
    int char_offset, ladj, radj, stroke_offset, ih, iv;
    unsigned int count, string_length, scount;
    double cos_angle, sin_angle;
    double lwidth, lheight;
    double hpos, vpos, h, v, hold, vold;
    double x0, y0, x1, y1;
    struct mode_struct main_mode, char_mode;
    extern StrokeFont *main_font;

    initialise ();
    if (main_font == NULL) return (FALSE);
    lwidth = 0;
    lheight = main_font->cdef * main_font->cheight * main_font->expand;
    cos_angle = cos (angle * PION180);
    sin_angle = sin (angle * PION180);
    main_mode.italic = FALSE;
    main_mode.font = 0;
    main_mode.superscript_level = 0;
    main_mode.superscript_factor = 1.0;
    main_mode.superscript_shift = 0.0;
    string_length = strlen (string);
    char_mode = main_mode;
    hpos = 0.0;
    vpos = 0.0;
    for (count = 0; count < string_length; ++count)
    {
	if (string[count] == main_font->esc_char)
	{
	    /*  Mode change required  */
	    change_mode = FALSE;
	    if (string[count + 1] == main_font->esc_char)
	    {
		change_mode = TRUE;
		++count;
	    }
	    if (++count >= string_length) return (FALSE);
	    switch (string[count])
	    {
	      case 'b':
	      case 'B':
		hpos -= main_font->expand * main_font->cdef *
		    char_mode.superscript_factor * 18.0;
		break;
	      case 'r':
	      case 'R':
		char_mode.font = 0;
		break;
	      case 'g':
	      case 'G':
		char_mode.font = 1;
		break;
	      case 's':
	      case 'S':
		char_mode.font = 2;
		break;
	      case 't':
	      case 'T':
		char_mode.font = 3;
		break;
	      case 'i':
	      case 'I':
		char_mode.italic = char_mode.italic ? FALSE : TRUE;
		break;
	      case 'u':
	      case 'U':
		if (char_mode.superscript_level >= 0)
		{
		    ++char_mode.superscript_level;
		    char_mode.superscript_factor =
			exp (-abs (char_mode.superscript_level) *
			     main_font->supfrac);
		    char_mode.superscript_shift = char_mode.superscript_shift +
			16.0 * char_mode.superscript_factor;
		}
		else
		{
		    char_mode.superscript_shift = char_mode.superscript_shift +
			16.0 * char_mode.superscript_factor;
		    ++char_mode.superscript_level;
		    char_mode.superscript_factor =
			exp (-abs (char_mode.superscript_level) *
			     main_font->supfrac);
		}
		break;
	      case 'd':
	      case 'D':
		if (char_mode.superscript_level <= 0)
		{
		    --char_mode.superscript_level;
		    char_mode.superscript_factor =
			exp (-abs (char_mode.superscript_level) *
			     main_font->supfrac);
		    char_mode.superscript_shift = char_mode.superscript_shift -
			16.0 * char_mode.superscript_factor;
		}
		else
		{
		    char_mode.superscript_shift = char_mode.superscript_shift -
			16.0 * char_mode.superscript_factor;
		    --char_mode.superscript_level;
		    char_mode.superscript_factor =
			exp (-abs (char_mode.superscript_level) *
			     main_font->supfrac);
		}
		break;
	      default:
		break;
	    }
	    if (change_mode) main_mode = char_mode;
	    continue;
	}
	/*  Process an ordinary character  */
	char_offset = string[count] - ' ';
	ladj = main_font->ladj[char_mode.font][char_offset];
	if (ladj > 127) ladj = ladj - 256;
	radj = main_font->radj[char_mode.font][char_offset];
	if (radj > 127) radj = radj - 256;
	if (canvas != NULL)
	{
	    /*  Must actually draw the character  */
	    relo = TRUE;
	    hold = hpos;
	    vold = vpos;
	    for (scount = 0;
		 scount < main_font->nstroke[char_mode.font][char_offset];
		 ++scount)
	    {
		stroke_offset =main_font->pointer[char_mode.font][char_offset];
		ih = main_font->font[stroke_offset + scount * 2];
		if (ih == 31) relo = TRUE;
		else
		{
		    iv = main_font->font[stroke_offset + scount * 2 + 1];
		    h = ih - ladj;
		    if (char_mode.italic) h += main_font->slant * (iv + 9);
		    h *= main_font->expand * main_font->cdef *
			char_mode.superscript_factor;
		    v = -main_font->expand * main_font->cdef *
			(iv * char_mode.superscript_factor +
			 char_mode.superscript_shift);
		    h += hpos;
		    v += vpos;
		    if (relo) relo = FALSE;
		    else
		    {
			/*  Rotate  */
			x0 = cos_angle * hold + sin_angle * vold + x;
			y0 = cos_angle * vold - sin_angle * hold + y;
			x1 = cos_angle * h + sin_angle * v + x;
			y1 = cos_angle * v - sin_angle * h + y;
			kwin_draw_line (canvas, x0, y0, x1, y1, pixel_value);
		    }
		    hold = h;
		    vold = v;
		}
	    }
	}
	/*  Update position  */
	h = main_font->expand * main_font->cdef * (radj - ladj) *
	    char_mode.superscript_factor;
	lwidth += h;
	hpos += h;
	/*  Restore mode  */
	char_mode = main_mode;
    }
    if (width != NULL) *width = lwidth;
    if (height != NULL) *height = lheight;
    return (TRUE);
}   /*  End Function kwin_hersey_draw_string  */


/*  Private functions follow  */

static void initialise (void)
/*  [SUMMARY] Initialise.
    [RETURNS] Nothing.
*/
{
    Channel channel;
    int num_char, num_segs;
    int font_count, char_count, index, seg_count;
    StrokeFont *new;
    char *karmabase, *p;
    char file[STRING_LENGTH];
    char line[STRING_LENGTH];
    extern StrokeFont *main_font;
    static flag initialised = FALSE;
    static char function_name[] = "__kwin_hershey_initialise";

    if (initialised) return;
    initialised = TRUE;
    karmabase = r_get_karmabase ();
    sprintf (file, "%s/share/fonts.dat", karmabase);
    if ( ( channel = ch_open_file (file, "r") ) == NULL )
    {
	fprintf (stderr, "%s: error opening font file: \"%s\"\n",
		 function_name, file);
	return;
    }
    /*  Read first line  */
    if ( !chs_get_line (channel, line, STRING_LENGTH) )
    {
	fprintf (stderr, "%s: error reading line\n", function_name);
	ch_close (channel);
	return;
    }
    num_char = ex_int (line, &p);
    if (num_char != NUM_FONTS * NUM_CHAR)
    {
	fprintf (stderr, "%s: corrupted font file\n", function_name);
	ch_close (channel);
	return;
    }
    num_segs = ex_int (p, &p);
    if ( ( new = (StrokeFont *) m_alloc (sizeof *main_font) ) == NULL )
    {
	m_abort (function_name, "font structure");
    }
    /*  Read the character data  */
    for (font_count = 0; font_count < NUM_FONTS; ++font_count)
    {
	for (char_count = 0; char_count < NUM_CHAR; ++char_count)
	{
	    if ( !chs_get_line (channel, line, STRING_LENGTH) )
	    {
		fprintf (stderr, "%s: error reading line\n", function_name);
		ch_close (channel);
		m_free ( (char *) new );
		return;
	    }
	    index = ex_int (line, &p);
	    if (index != char_count + font_count * NUM_CHAR + 1)
	    {
		fprintf (stderr, "%s: corrupted font file\n", function_name);
		ch_close (channel);
		m_free ( (char *) new );
		return;
	    }
	    new->nstroke[font_count][char_count] = ex_int (p, &p) / 2;
	    new->ladj[font_count][char_count] = ex_int (p, &p);
	    new->radj[font_count][char_count] = ex_int (p, &p);
	    /*  Font file has stupid fortran indexing: correct it  */
	    new->pointer[font_count][char_count] = ex_int (p, &p) - 1;
	}
    }
    /*  Correct total number of segments value  */
    num_segs = new->pointer[NUM_FONTS - 1][NUM_CHAR - 1] +
	new->nstroke[NUM_FONTS - 1][NUM_CHAR - 1] * 2;
    if ( ( new->font = (signed char *) m_alloc (num_segs) ) == NULL )
    {
	m_abort (function_name, "font information");
    }
    for (seg_count = 0; seg_count < num_segs; ++seg_count)
    {
	new->font[seg_count] = chs_get_int (channel);
    }
    ch_close (channel);
    new->expand = 1.0;
    new->cdef = 0.4;
    new->cheight = 32.0;
    new->esc_char = '\\';
    new->slant = 0.2;
    new->supfrac = 0.3;
    main_font = new;
}   /*  End Function initialise  */
