/*LINTLIBRARY*/
/*  main.c

    This code provides routines to draw axes into an X window.

    Copyright (C) 1992,1993,1994  Richard Gooch

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

/*  This file contains all routines needed for the drawing of axes to an X11
    server.


    Written by      Richard Gooch   25-SEP-1992

    Updated by      Richard Gooch   4-DEC-1992

    Last updated by Richard Gooch   26-NOV-1994: Moved to  packages/ax/main.c


*/
#include <stdio.h>
#include <math.h>
#include <karma.h>
#include <karma_a.h>
#include <karma_ax.h>

#define MAJOR_TICK_MM (double) 3.0
#define MEDIUM_TICK_MM (double) 2.0
#define MINOR_TICK_MM (double) 1.0
#define PLOT_BORDER_MM (double)  5.0
#define MAX_MAJOR_TICKS 11
#define MAX_MINOR_TICKS 55
#define MAX_LINEAR_SPACINGS (unsigned int) 3
#define NUM_MINOR_TICKS_PER_MAJOR_TICK (double) 10.0
#define NUM_MINOR_TICKS_PER_MEDIUM_TICK (double) 5.0
#define LIN_SCALE_CUTOFF1 (double) 4.0
#define LIN_SCALE_CUTOFF2 (double) -2.0
#define ABS_SCALE_SPACING_MM (double) 10.0
#define MAX_ORD_INTERVAL_CHOICES (unsigned int) 5
#define ORDINATE_TRACE_SEPARATION_FACTOR (double) 1.05


/*  Private functions follow  */

static int xverticaltextwidth (font_info, string)
/*  This routine will determine the width of the widest character in a string.
    The font information must be pointed to by  font_info  .
    The string must be pointed to by  string  and must be NULL terminated.
    The routine returns the width of the widest charecter in pixels.
*/
XFontStruct *font_info;
char *string;
{
    static char function_name[] = "xverticaltextwidth";
    int max_width = 0;
    int char_width;
    unsigned int char_count;
    unsigned int string_length;

    if ( ( string_length = (unsigned int) strlen (string) ) == 0 )
    {
	return (0);
    }
    for (char_count = 0; char_count < string_length; ++char_count)
    {
	if ( ( char_width = XTextWidth (font_info, &string[char_count], 1) )
	    > max_width )
	{
	    max_width = char_width;
	}
    }
    return (max_width);
}   /*  End Function xverticaltextwidth  */

static void xdrawverticalstring (display, window, gc, font_height, x, y,
				 string)
/*  This routine will draw a vertical string onto a window. The string is drawn
    downwards.
    The display and window to draw onto must be given by  display  and  window
    ,respectively.
    The graphics context to use must be in  gc  .
    The height of the font must be in  font_height  .
    The x and y co-ordinates at which to place the string must be in  x  and  y
    ,respectively.
    The string to be draw must be pointed to by  string  .
    The routine returns nothing.
*/
Display *display;
Window window;
GC gc;
int font_height;
int x;
int y;
char *string;
{
    static char function_name[] = "xdrawverticalstring";
    unsigned int string_length;
    unsigned int char_count;

    string_length = (unsigned int) strlen (string);
    for (char_count = 0; char_count < string_length; ++char_count)
    {
	XDrawString (display, window, gc, x, y, &string[char_count], 1);
	y += font_height;
    }
}   /*  End Function xdrawverticalstring  */


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
flag ax_plot_dressing (display, window, gc,
		       title_string, abscissa_label, ordinate_label,
		       title_font_name, axes_font_name, scale_font_name,
		       win_scale, max_log_cycles, error_notify_func)
/*  This routine will draw all the dressings required for the plot. It will
    plot the title, axes and their labels and the scales on the axes.
    The display and window to plot in must be given in  display  and  window  ,
    respectively.
    The graphics context to be used must be in  gc  .
    The title string must be pointed to  title_string  .
    The abscissa label string must be pointed to by  abscissa_label  .
    The ordinate label string must be pointed to by  ordinate_label  .
    The name of the title, axes and scale fonts must be pointed to by
    title_font_name  ,  axes_font_name  and  scale_font_name  ,respectively.
    The scaling information must be pointed to by  win_scale  .The entries in
    this structure will be updated by the axis scaling routine.
    The maximum number of log cycles to plot must be given by  max_log_cycles
    The routine used to notify the user of errors must be pointed to by
    error_notify_func  .If this is NULL, errors are sent to the standard error.
    The interface to this function is described below:

    void error_notify_func (error_message)
    *   This routine will display the error message pointed to by
        error_message  .
	The routine returns nothing.
    *
    {}

    The routine returns TRUE on success, else it returns FALSE.
*/
Display *display;
Window window;
GC gc;
char *title_string;
char *abscissa_label;
char *ordinate_label;
char *title_font_name;
char *axes_font_name;
char *scale_font_name;
struct win_scale_type *win_scale;
unsigned int max_log_cycles;
void (*error_notify_func) ();
{
    static char function_name[] = "ax_plot_dressing";
    int title_font_height;
    int axes_font_height;
    int scale_font_height;
    int ord_label_width;
    int string_pixels;
    int plot_border_x;
    int plot_border_y;
    int title_offset;
    int abs_label_offset;
    int ord_label_offset;
    int abs_scale_offset;
    int default_screen;
    float pixels_per_mm_x;
    float pixels_per_mm_y;
    XFontStruct *title_font;
    XFontStruct *axes_font;
    XFontStruct *scale_font;
    char txt[STRING_LENGTH];

    default_screen = DefaultScreen(display);
    /*  Calculate number of pixels in ticks  */
    pixels_per_mm_x = ( (float) DisplayWidth (display, default_screen) /
		       (float) DisplayWidthMM (display, default_screen) );
    pixels_per_mm_y = ( (float) DisplayHeight (display, default_screen ) /
		       (float) DisplayHeightMM (display, default_screen) );
    /*  Calculate number of plot border pixels  */
    plot_border_x = (pixels_per_mm_x * PLOT_BORDER_MM + (float) 0.5);
    plot_border_y = (pixels_per_mm_y * PLOT_BORDER_MM + (float) 0.5);

    /*  Load title font  */
    if ( ( title_font = XLoadQueryFont (display, title_font_name) ) == NULL )
    {
	(void) sprintf (txt,"cannot open title font: \"%s\"", title_font_name);
	if (error_notify_func == NULL)
	{
	    (void) fprintf (stderr, "\n%s", txt);
	}
	else
	{
	    (*error_notify_func) (txt);
	}
	return (FALSE);
    }
    /*  Load axes font  */
    if ( ( axes_font = XLoadQueryFont (display, axes_font_name) ) == NULL )
    {
	(void) sprintf (txt, "cannot open axes font: \"%s\"", axes_font_name);
	if (error_notify_func == NULL)
	{
	    (void) fprintf (stderr, "\n%s", txt);
	}
	else
	{
	    (*error_notify_func) (txt);
	}
	/*  Free all font info  */
	free ( (char *) title_font );
	return (FALSE);
    }
    /*  Load scale font  */
    if ( ( scale_font = XLoadQueryFont (display, scale_font_name) ) == NULL )
    {
	(void) sprintf (txt,"cannot open scale font: \"%s\"", scale_font_name);
	if (error_notify_func == NULL)
	{
	    (void) fprintf (stderr, "\n%s", txt);
	}
	else
	{
	    (*error_notify_func) (txt);
	}
	/*  Free all font info  */
	free ( (char *) title_font );
	free ( (char *) axes_font );
	return (FALSE);
    }
    /*  Determine font heights  */
    title_font_height = (*title_font).ascent + (*title_font).descent;
    axes_font_height = (*axes_font).ascent + (*axes_font).descent;
    scale_font_height = (*scale_font).ascent + (*scale_font).descent;
    /*  Determine ordinate label width  */
    ord_label_width = xverticaltextwidth (axes_font, ordinate_label);
    /*  Determine title y offset  */
    if (title_font_height > (*win_scale).y_pixels)
    {
	(void) strcpy (txt, "window too small for title font height");
	if (error_notify_func == NULL)
	{
	    (void) fprintf (stderr, "\n%s", txt);
	}
	else
	{
	    (*error_notify_func) (txt);
	}
	return (FALSE);
    }
    title_offset = (*win_scale).y_offset + title_font_height - 1;
    /*  Determine abscissa label y offset  */
    if (axes_font_height + title_font_height > (*win_scale).y_pixels)
    {
	(void) strcpy (txt, "window too small for abscissa label font height");
	if (error_notify_func == NULL)
	{
	    (void) fprintf (stderr, "\n%s", txt);
	}
	else
	{
	    (*error_notify_func) (txt);
	}
	return (FALSE);
    }
    abs_label_offset = (*win_scale).y_offset + (*win_scale).y_pixels - 1;
    /*  Determine ordinate label x offset  */
    if (ord_label_width > (*win_scale).x_pixels)
    {
	(void) strcpy (txt, "window too small for ordinate font width");
	if (error_notify_func == NULL)
	{
	    (void) fprintf (stderr, "\n%s", txt);
	}
	else
	{
	    (*error_notify_func) (txt);
	}
	return (FALSE);
    }
    ord_label_offset = plot_border_x;
    /*  Compute new display area  */
    (*win_scale).x_offset += ord_label_offset + ord_label_width +plot_border_x;
    (*win_scale).x_pixels -= ord_label_offset + ord_label_width +plot_border_x;
    (*win_scale).y_offset += title_font_height;
    (*win_scale).y_pixels -= title_font_height + axes_font_height;
    /*  Determine abscissa scale y offset  */
    abs_scale_offset = (*win_scale).y_offset + (*win_scale).y_pixels - 1;
    /*  Compute new display area  */
    (*win_scale).y_pixels -= scale_font_height;

    XSetFont (display, gc, (*scale_font).fid);
    /*  Draw ordinate scale  */
    if (ax_draw_ordinate_scale (display, window, gc, scale_font, win_scale,
				max_log_cycles, error_notify_func)
	== FALSE)
    {
	return (FALSE);
    }

    /*  Draw abscissa scale  */
    if (scale_font_height > (*win_scale).y_pixels)
    {
	(void) strcpy (txt, "window too small for abscissa scale font height");
	if (error_notify_func == NULL)
	{
	    (void) fprintf (stderr, "\n%s", txt);
	}
	else
	{
	    (*error_notify_func) (txt);
	}
	return (FALSE);
    }
    if (ax_draw_abscissa_scale (display, window, gc, scale_font,
				abs_scale_offset, win_scale, max_log_cycles,
				error_notify_func)
	== FALSE)
    {
	return (FALSE);
    }

    /*  Draw title, abscissa and ordinate labels  */
    /*  Draw title string  */
    XSetFont (display, gc, (*title_font).fid);
    string_pixels = XTextWidth ( title_font, title_string,
				strlen (title_string) );
    if (string_pixels > (*win_scale).x_pixels)
    {
	(void) strcpy (txt, "window too small for title string");
	if (error_notify_func == NULL)
	{
	    (void) fprintf (stderr, "\n%s", txt);
	}
	else
	{
	    (*error_notify_func) (txt);
	}
	return (FALSE);
    }
    XDrawString ( display, window, gc,
		 (*win_scale).x_offset + ( (*win_scale).x_pixels -
					  string_pixels ) / 2,
		 title_offset - (*title_font).descent,
		 title_string, strlen (title_string) );
    /*  Draw abscissa axis name  */
    XSetFont (display, gc, (*axes_font).fid);
    string_pixels = XTextWidth ( axes_font, abscissa_label,
				strlen (abscissa_label) );
    if (string_pixels > (*win_scale).x_pixels)
    {
	(void) strcpy (txt, "window too small for abscissa label");
	if (error_notify_func == NULL)
	{
	    (void) fprintf (stderr, "\n%s", txt);
	}
	else
	{
	    (*error_notify_func) (txt);
	}
	return (FALSE);
    }
    XDrawString ( display, window, gc,
		 (*win_scale).x_offset + ( (*win_scale).x_pixels -
					  string_pixels ) / 2,
		 abs_label_offset - (*axes_font).descent,
		 abscissa_label, strlen (abscissa_label) );
    /*  Draw ordinate axis name  */
    string_pixels = axes_font_height * strlen (ordinate_label);
    if (string_pixels > (*win_scale).y_pixels)
    {
	(void) strcpy (txt, "window too small for ordinate label");
	if (error_notify_func == NULL)
	{
	    (void) fprintf (stderr, "\n%s", txt);
	}
	else
	{
	    (*error_notify_func) (txt);
	}
	return (FALSE);
    }
    xdrawverticalstring (display, window, gc, axes_font_height,
			 ord_label_offset,
			 (*win_scale).y_offset + ( (*win_scale).y_pixels -
						  string_pixels ) / 2 -
			 (*axes_font).descent,
			 ordinate_label);

    return (TRUE);
}   /*  End Function ax_plot_dressing  */

/*PUBLIC_FUNCTION*/
flag ax_choose_scale (min, max, log, new_scale, max_log_cycles,
		      error_notify_func)
/*  This routine will choose a scale and decide where to put tick marks.
    The input range must be in  min  and  max  ,respectively.
    If the scale is logarithmic, then  log  must be TRUE.
    The scale information is written to the structure pointed to by  new_scale
    This routine works for either ordinate or abscissa axes.
    The maximum number of log cycles to plot must be given by  max_log_cycles
    The routine used to notify the user of errors must be pointed to by
    error_notify_func  .If this is NULL, errors are sent to the standard error.
    The interface to this function is described below:

    void error_notify_func (error_message)
    *   This routine will display the error message pointed to by
        error_message  .
	The routine returns nothing.
    *
    {}

    The routine returns TRUE on success, else it returns FALSE.
*/
double min;
double max;
flag log;
struct scale_type *new_scale;
unsigned int max_log_cycles;
void (*error_notify_func) ();
{
    static char function_name[] = "ax_choose_scale";
    unsigned int spacing_count = 0;
    double inp_norm_scale;
    double inp_scale_exp;
    double major_tick_interval;
    double last_major_tick;
    char *txt;
    static double spacings[MAX_LINEAR_SPACINGS] =
    {
	0.1, 0.2, 1.0
    };

    if (log == TRUE)
    {
	/*  Logarithmic scale  */
	if (min > 0.0)
	{
	    (*new_scale).min = floor ( log10 (min) );
	}
	else
	{
	    (*new_scale).min = -TOOBIG;
	}
	if (max <= 0.0)
	{
	    txt = "Maximum scale must be greater than zero for log scale";
	    if (error_notify_func == NULL)
	    {
		(void) fprintf (stderr, "\n%s", txt);
	    }
	    else
	    {
		(*error_notify_func) (txt);
	    }
	    return (FALSE);
	}
	(*new_scale).max = ceil ( log10 (max) );
	if ( (*new_scale).max - (*new_scale).min > (double) max_log_cycles )
	{
	    (*new_scale).min = (*new_scale).max - (double) max_log_cycles;
	}
	(*new_scale).num_major_ticks = ( (*new_scale).max -
					(*new_scale).min + 1.0 );
	(*new_scale).min = pow (10.0, (*new_scale).min);
	(*new_scale).max = pow (10.0, (*new_scale).max);
	return (TRUE);
    }
    /*  Linear scale  */
    /*  Normalise scale  */
    inp_scale_exp = pow ( 10.0, floor ( log10 (max - min) ) );
    inp_norm_scale = (max - min) / inp_scale_exp;
    do
    {
	(*new_scale).num_major_ticks = 1 + (inp_norm_scale /
					    spacings[spacing_count]);
    }
    while ( ( (*new_scale).num_major_ticks > MAX_MAJOR_TICKS ) &&
	   (++spacing_count < MAX_LINEAR_SPACINGS) );
    if ( (*new_scale).num_major_ticks > MAX_MAJOR_TICKS )
    {
	txt = "too many major ticks: program bug";
	if (error_notify_func == NULL)
	{
	    (void) fprintf (stderr, "\n%s", txt);
	}
	else
	{
	    (*error_notify_func) (txt);
	}
	return (FALSE);
    }
    major_tick_interval = spacings[spacing_count] * inp_scale_exp;
    (*new_scale).tick_interval = (major_tick_interval /
				  NUM_MINOR_TICKS_PER_MAJOR_TICK);
    (*new_scale).min = ( floor (min / (*new_scale).tick_interval) *
			(*new_scale).tick_interval );
    (*new_scale).max =  ( ceil (max / (*new_scale).tick_interval) *
			 (*new_scale).tick_interval );
    (*new_scale).num_ticks = ( ( (*new_scale).max - (*new_scale).min ) /
			      (*new_scale).tick_interval + 1.5 );
    (*new_scale).first_major_tick = ( ceil ( (*new_scale).min /
					    major_tick_interval ) *
				     major_tick_interval );
    last_major_tick = ( floor ( (*new_scale).max / major_tick_interval ) *
		       major_tick_interval );
    (*new_scale).num_major_ticks = ( (last_major_tick -
				      (*new_scale).first_major_tick) /
				    major_tick_interval + 1.5 );
    (*new_scale).first_tick_num = floor ( ( (*new_scale).min
					   - (*new_scale).first_major_tick ) /
					 (*new_scale).tick_interval + 0.5 );
    return (TRUE);
}   /*  End Function ax_choose_scale  */

/*PUBLIC_FUNCTION*/
flag ax_draw_ordinate_scale (display, window, gc, font_info, win_scale,
			     max_log_cycles, error_notify_func)
/*  This routine will draw the ordinate scale into the window indicated by
    display  and  window  .
    The graphics context to be used must be in  gc  .
    The font information for the scale font must be pointed to by  font_info  .
    The scaling information must be pointed to by  win_scale  .The entries in
    this structure will be updated with values chosen by the scaling routine.
    The maximum number of log cycles to plot must be given by  max_log_cycles
    The routine used to notify the user of errors must be pointed to by
    error_notify_func  .If this is NULL, errors are sent to the standard error.
    The interface to this function is described below:

    void error_notify_func (error_message)
    *   This routine will display the error message pointed to by
        error_message  .
	The routine returns nothing.
    *
    {}

    The routine returns TRUE on success, else it returns FALSE.
*/
Display *display;
Window window;
GC gc;
XFontStruct *font_info;
struct win_scale_type *win_scale;
unsigned int max_log_cycles;
void (*error_notify_func) ();
{
    static char function_name[] = "ax_draw_ordinate_scale";
    int first_log;
    int width = 0;
    int string_length;
    int string_pixels;
    int major_tick_position;
    int minor_tick_position;
    int tick_width;
    int pixels_major;
    int pixels_medium;
    int pixels_minor;
    int font_height;
    double pixels_per_mm;
    double lin_scale;
    double major_tick;
    unsigned int major_tick_count;
    unsigned int minor_tick_count;
    struct scale_type scale_info;
    char txt[STRING_LENGTH];

    /*  Calculate number of pixels in ticks  */
    pixels_per_mm = ( (float)DisplayWidth (display, DefaultScreen (display) ) /
		     (float) DisplayWidthMM(display, DefaultScreen(display) ));
    pixels_major = (pixels_per_mm * MAJOR_TICK_MM + (float) 0.5);
    pixels_medium = (pixels_per_mm * MEDIUM_TICK_MM + (float) 0.5);
    pixels_minor = (pixels_per_mm * MINOR_TICK_MM + (float) 0.5);

    if (ax_choose_scale ( (*win_scale).y_min, (*win_scale).y_max,
			 (*win_scale).y_log, &scale_info, max_log_cycles,
			 error_notify_func )
	== FALSE)
    {
	return (FALSE);
    }
    (*win_scale).y_min = scale_info.min;
    (*win_scale).y_max = scale_info.max;
    font_height = (*font_info).ascent + (*font_info).descent;
    if ( (*win_scale).y_log == TRUE )
    {
	/*  Logarithmic scale  */
	/*  Leave room for endpoint scale numbers  */
	(*win_scale).y_offset += ( (double) (*font_info).ascent / 2.0 + 0.5 );
	(*win_scale).y_pixels -= font_height;
	if (scale_info.num_major_ticks * font_height > (*win_scale).y_pixels)
	{
	    (void) strcpy (txt, "window not tall enough for scale");
	    if (error_notify_func == NULL)
	    {
		(void) fprintf (stderr, "\n%s", txt);
	    }
	    else
	    {
		(*error_notify_func) (txt);
	    }
	    return (FALSE);
	}
	/*  Draw scale numbers  */
	first_log = (int) log10 ( (*win_scale).y_min );
	for (major_tick_count = 0;
	     major_tick_count < scale_info.num_major_ticks;
	     ++major_tick_count)
	{
	    (void) sprintf (txt, "1E%d", first_log + (int) major_tick_count);
	    string_length = strlen (txt);
	    string_pixels = XTextWidth (font_info, txt, string_length);
	    if (string_pixels > width)
	    {
		width = string_pixels;
	    }
	    /*  Draw the scale numbers  */
	    XDrawString (display, window, gc,
			 (*win_scale).x_offset,
			 (*win_scale).y_offset + (*win_scale).y_pixels - 1 +
			 ( (*font_info).ascent / 2 ) -
			 (int) major_tick_count *
			 (*win_scale).y_pixels /
			 (int) (scale_info.num_major_ticks - 1),
			 txt, string_length);
	}
	/*  Compute further reduced plot area  */
	(*win_scale).x_offset += width;
	(*win_scale).x_pixels -= width;
	/*  Plot axes and tick marks  */
	XDrawLine (display, window, gc,
		   (*win_scale).x_offset, (*win_scale).y_offset,
		   (*win_scale).x_offset, (*win_scale).y_offset +
		   (*win_scale).y_pixels - 1);
	XDrawLine (display, window, gc,
		   (*win_scale).x_offset + (*win_scale).x_pixels - 1,
		   (*win_scale).y_offset,
		   (*win_scale).x_offset + (*win_scale).x_pixels - 1,
		   (*win_scale).y_offset + (*win_scale).y_pixels - 1);
	for (major_tick_count = 0;
	     major_tick_count < scale_info.num_major_ticks - 1;
	     ++major_tick_count)
	{
	    major_tick_position = ( (*win_scale).y_offset +
				   (*win_scale).y_pixels - 1 -
				   (int) major_tick_count *
				   (*win_scale).y_pixels /
				   (int) (scale_info.num_major_ticks -1) );
	    if (major_tick_count != 0)
	    {
		/*  Draw tick only if not first one: saves graphics calls and
		    hence network traffic  */
		XDrawLine (display, window, gc,
			   (*win_scale).x_offset, major_tick_position,
			   (*win_scale).x_offset + pixels_major - 1,
			   major_tick_position);
		XDrawLine (display, window, gc,
			   (*win_scale).x_offset + (*win_scale).x_pixels - 1,
			   major_tick_position,
			   (*win_scale).x_offset + (*win_scale).x_pixels - 2 -
			   pixels_major,
			   major_tick_position);
	    }
	    for (minor_tick_count = 2;
		 minor_tick_count <
		 (unsigned int) NUM_MINOR_TICKS_PER_MAJOR_TICK;
		 ++minor_tick_count)
	    {
		/*  Determine tick position  */
		minor_tick_position = -( log10 ( (double) minor_tick_count ) *
					(double) (*win_scale).y_pixels /
					(double) (scale_info.num_major_ticks
						  - 1) );
		minor_tick_position += major_tick_position;
		if (minor_tick_count % (int) NUM_MINOR_TICKS_PER_MEDIUM_TICK
		    == 0)
		{
		    tick_width = pixels_medium;
		}
		else
		{
		    tick_width = pixels_minor;
		}
		XDrawLine (display, window, gc,
			   (*win_scale).x_offset, minor_tick_position,
			   (*win_scale).x_offset + tick_width - 1,
			   minor_tick_position);
		XDrawLine (display, window, gc,
			   (*win_scale).x_offset + (*win_scale).x_pixels - 1,
			   minor_tick_position,
			   (*win_scale).x_offset + (*win_scale).x_pixels - 2 -
			   tick_width,
			   minor_tick_position);
	    }
	}
	return (TRUE);
    }

    /*  Linear scale  */
    if (fabs ( (*win_scale).y_min ) == 0.0)
    {
	if (fabs ( (*win_scale).y_max ) == 0.0)
	{
	    (void) strcpy (txt, "minimum and maximum are both 0: program bug");
	    if (error_notify_func == NULL)
	    {
		(void) fprintf (stderr, "\n%s", txt);
	    }
	    else
	    {
		(*error_notify_func) (txt);
	    }
	    return (FALSE);
	}
	lin_scale = log10 ( fabs ( (*win_scale).y_max ) );
    }
    else
    {
	lin_scale = log10 ( fabs ( (*win_scale).y_min ) );
	if (lin_scale >= 0.0)
	{
	    /*  Positive exponent  */
	    if (fabs ( (*win_scale).y_max ) != 0.0)
	    {
		if (log10 ( fabs ( (*win_scale).y_max ) ) > lin_scale)
		{
		    lin_scale = log10 ( fabs ( (*win_scale).y_max ) );
		}
	    }
	}
	else
	{
	    /*  Negative exponent  */
	    if (fabs ( (*win_scale).y_max ) != 0.0)
	    {
		if (log10 ( fabs ( (*win_scale).y_max ) ) < lin_scale)
		{
		    lin_scale = log10 ( fabs ( (*win_scale).y_max ) );
		}
	    }
	}
    }
    if ( (lin_scale >= LIN_SCALE_CUTOFF1) || (lin_scale < LIN_SCALE_CUTOFF2) )
    {
	/*  Draw scale multiplier  */
	if (font_height > (*win_scale).y_pixels)
	{
	    (void) strcpy (txt, "window not tall enough for scale multiplier");
	    if (error_notify_func == NULL)
	    {
		(void) fprintf (stderr, "\n%s", txt);
	    }
	    else
	    {
		(*error_notify_func) (txt);
	    }
	    return (FALSE);
	}
	lin_scale = floor (lin_scale);
	(void) sprintf (txt, "*1E%d", (int) lin_scale);
	lin_scale = pow (10.0, -lin_scale);
	string_length = strlen (txt);
	XDrawString (display, window, gc,
		     (*win_scale).x_offset,
		     (*win_scale).y_offset + (*font_info).ascent - 1,
		     txt, string_length);
	/*  Compute further reduced plot area  */
	(*win_scale).y_offset += font_height;
	(*win_scale).y_pixels -= font_height;
    }
    else
    {
	lin_scale = 1.0;
    }
    /*  Leave room for endpoint scale numbers (even if they don't exist)  */
    (*win_scale).y_offset += ( (double) (*font_info).ascent / 2.0 + 0.5 );
    (*win_scale).y_pixels -= font_height;
    if (scale_info.num_major_ticks * font_height > (*win_scale).y_pixels)
    {
	(void) strcpy (txt, "window not tall enough for scale");
	if (error_notify_func == NULL)
	{
	    (void) fprintf (stderr, "\n%s", txt);
	}
	else
	{
	    (*error_notify_func) (txt);
	}
	return (FALSE);
    }
    /*  Draw scale numbers  */
    for (major_tick_count = 0;
	 major_tick_count < scale_info.num_major_ticks;
	 ++major_tick_count)
    {
	major_tick = (scale_info.first_major_tick +
		      (double) major_tick_count *
		      scale_info.tick_interval *
		      NUM_MINOR_TICKS_PER_MAJOR_TICK);
	/*  Do a bit of rounding  */
	major_tick = (floor ( (major_tick / scale_info.tick_interval) + 0.5 )
		      * scale_info.tick_interval);
	if (lin_scale == 1.0)
	{
	    /*  Unscaled  */
	    (void) sprintf (txt, "%g", major_tick);
	}
	else
	{
	    /*  Scaled  */
	    (void) sprintf (txt, "%g", major_tick * lin_scale);
	}
	string_length = strlen (txt);
	string_pixels = XTextWidth (font_info, txt, string_length);
	if (string_pixels > width)
	{
	    width = string_pixels;
	}
	/*  Draw the scale numbers  */
	XDrawString (display, window, gc,
		     (*win_scale).x_offset, (*win_scale).y_offset +
		     (*win_scale).y_pixels - 1 +
		     (*font_info).ascent / 2 -
		     ( (int) major_tick_count *
		      (int) NUM_MINOR_TICKS_PER_MAJOR_TICK -
		      scale_info.first_tick_num ) *
		     ( (*win_scale).y_pixels - 1 ) /
		     (int) (scale_info.num_ticks - 1),
		     txt, string_length);
    }
    /*  Compute further reduced plot area  */
    (*win_scale).x_offset += width;
    (*win_scale).x_pixels -= width;
    /*  Plot axes and tick marks  */
    XDrawLine (display, window, gc,
	       (*win_scale).x_offset, (*win_scale).y_offset,
	       (*win_scale).x_offset, (*win_scale).y_offset +
	       (*win_scale).y_pixels - 1);
    XDrawLine (display, window, gc,
	       (*win_scale).x_offset + (*win_scale).x_pixels - 1,
	       (*win_scale).y_offset,
	       (*win_scale).x_offset + (*win_scale).x_pixels - 1,
	       (*win_scale).y_offset + (*win_scale).y_pixels - 1);
    for (minor_tick_count = 0; minor_tick_count < scale_info.num_ticks - 1;
	 ++minor_tick_count)
    {
	minor_tick_position = ( (*win_scale).y_offset + (*win_scale).y_pixels -
			       1 - (int) minor_tick_count *
			       ( (*win_scale).y_pixels - 1 ) /
			       (int) (scale_info.num_ticks - 1) );
	if ( ( (int) minor_tick_count + scale_info.first_tick_num )
	    % (int) NUM_MINOR_TICKS_PER_MAJOR_TICK == 0 )
	{
	    /*  Major tick  */
	    XDrawLine (display, window, gc,
		       (*win_scale).x_offset, minor_tick_position,
		       (*win_scale).x_offset + pixels_major - 1,
		       minor_tick_position);
	    XDrawLine (display, window, gc,
		       (*win_scale).x_offset + (*win_scale).x_pixels - 1,
		       minor_tick_position,
		       (*win_scale).x_offset + (*win_scale).x_pixels - 2 -
		       pixels_major,
		       minor_tick_position);
	}
	else if ( ( (int) minor_tick_count + scale_info.first_tick_num )
		 % (int) NUM_MINOR_TICKS_PER_MEDIUM_TICK == 0 )
	{
	    /*  Medium tick  */
	    XDrawLine (display, window, gc,
		       (*win_scale).x_offset, minor_tick_position,
		       (*win_scale).x_offset + pixels_medium - 1,
		       minor_tick_position);
	    XDrawLine (display, window, gc,
		       (*win_scale).x_offset + (*win_scale).x_pixels - 1,
		       minor_tick_position,
		       (*win_scale).x_offset + (*win_scale).x_pixels - 2 -
		       pixels_medium,
		       minor_tick_position);
	}
	else
	{
	    /*  Minor tick  */
	    if (scale_info.num_ticks <= MAX_MINOR_TICKS)
	    {
		/*  May plot minor tick  */
		XDrawLine (display, window, gc,
			   (*win_scale).x_offset, minor_tick_position,
			   (*win_scale).x_offset + pixels_minor - 1,
			   minor_tick_position);
		XDrawLine (display, window, gc,
			   (*win_scale).x_offset + (*win_scale).x_pixels - 1,
			   minor_tick_position,
			   (*win_scale).x_offset + (*win_scale).x_pixels - 2 -
			   pixels_minor,
			   minor_tick_position);
	    }
	}
    }
    return (TRUE);
}   /*  End Function ax_draw_ordinate_scale  */

/*PUBLIC_FUNCTION*/
flag ax_draw_abscissa_scale (display, window, gc, font_info, scale_offset,
			     win_scale, max_log_cycles, error_notify_func)
/*  This routine will draw the abscissa scale into the window indicated by
    display  and  window  .
    The graphics context to be used must be in  gc  .
    The font information for the scale font must be pointed to by  font_info  .
    The offset for the scale numbers must be in  scale_offset  .
    The scaling information must be pointed to by  win_scale  .The entries in
    this structure will be updated with values chosen by the scaling routine.
    The maximum number of log cycles to plot must be given by  max_log_cycles
    The routine used to notify the user of errors must be pointed to by
    error_notify_func  .If this is NULL, errors are sent to the standard error.
    The interface to this function is described below:

    void error_notify_func (error_message)
    *   This routine will display the error message pointed to by
        error_message  .
	The routine returns nothing.
    *
    {}

    The routine returns TRUE on success, else it returns FALSE.
*/
Display *display;
Window window;
GC gc;
XFontStruct *font_info;
int scale_offset;
struct win_scale_type *win_scale;
unsigned int max_log_cycles;
void (*error_notify_func) ();
{
    static char function_name[] = "ax_draw_abscissa_scale";
    int first_log;
    int string_length;
    int string_pixels;
    int major_tick_position;
    int minor_tick_position;
    int tick_width;
    int pixels_major;
    int pixels_medium;
    int pixels_minor;
    int font_height;
    int prev_scale_end;
    int final_scale_start;
    int abs_scale_spacing;
    int new_x_offset;
    int new_x_pixels;
    double pixels_per_mm;
    double lin_scale;
    double major_tick;
    unsigned int major_tick_count;
    unsigned int minor_tick_count;
    struct scale_type scale_info;
    char txt[STRING_LENGTH];

    /*  Calculate number of pixels in ticks  */
    pixels_per_mm = ( (float)DisplayHeight (display,DefaultScreen (display) ) /
		     (float) DisplayHeightMM(display,DefaultScreen(display) ));
    pixels_major = (pixels_per_mm * MAJOR_TICK_MM + (float) 0.5);
    pixels_medium = (pixels_per_mm * MEDIUM_TICK_MM + (float) 0.5);
    pixels_minor = (pixels_per_mm * MINOR_TICK_MM + (float) 0.5);
    abs_scale_spacing = (pixels_per_mm * ABS_SCALE_SPACING_MM + (float) 0.5);

    if (ax_choose_scale ( (*win_scale).x_min, (*win_scale).x_max,
			 (*win_scale).x_log, &scale_info, max_log_cycles,
			 error_notify_func )
	== FALSE)
    {
	return (FALSE);
    }
    (*win_scale).x_min = scale_info.min;
    (*win_scale).x_max = scale_info.max;
    font_height = (*font_info).ascent + (*font_info).descent;
    if (font_height > (*win_scale).y_pixels)
    {
	(void) strcpy (txt, "window not tall enough for scale");
	if (error_notify_func == NULL)
	{
	    (void) fprintf (stderr, "\n%s", txt);
	}
	else
	{
	    (*error_notify_func) (txt);
	}
	return (FALSE);
    }
    /*  Plot axes  */
    XDrawLine (display, window, gc,
	       (*win_scale).x_offset, (*win_scale).y_offset,
	       (*win_scale).x_offset + (*win_scale).x_pixels - 1,
	       (*win_scale).y_offset);
    XDrawLine (display, window, gc,
	       (*win_scale).x_offset,
	       (*win_scale).y_offset + (*win_scale).y_pixels - 1,
	       (*win_scale).x_offset + (*win_scale).x_pixels - 1,
	       (*win_scale).y_offset + (*win_scale).y_pixels - 1);
    if ( (*win_scale).x_log == TRUE )
    {
	/*  Logarithmic scale  */
	/*  Draw first scale number  */
	(void) sprintf ( txt, "1E%d", (int) log10 ( (*win_scale).x_min ) );
	string_length = strlen (txt);
	string_pixels = XTextWidth (font_info, txt, string_length);
	if (string_pixels + abs_scale_spacing > (*win_scale).x_pixels)
	{
	    (void) strcpy (txt, "window not wide enough for scale");
	    if (error_notify_func == NULL)
	    {
		(void) fprintf (stderr, "\n%s", txt);
	    }
	    else
	    {
		(*error_notify_func) (txt);
	    }
	    return (FALSE);
	}
	XDrawString (display, window, gc,
		     (*win_scale).x_offset,
		     scale_offset - (*font_info).descent,
		     txt, string_length);
	prev_scale_end = (*win_scale).x_offset + string_pixels - 1;
	(*win_scale).x_offset += string_pixels / 2;
	(*win_scale).x_pixels -= ( (double) string_pixels / 2.0 + 0.5 );
	/*  Draw last scale number  */
	(void) sprintf ( txt, "1E%d", (int) log10 ( (*win_scale).x_max ) );
	string_length = strlen (txt);
	string_pixels = XTextWidth (font_info, txt, string_length);
	if (string_pixels + abs_scale_spacing > (*win_scale).x_pixels)
	{
	    (void) strcpy (txt, "window not wide enough for scale");
	    if (error_notify_func == NULL)
	    {
		(void) fprintf (stderr, "\n%s", txt);
	    }
	    else
	    {
		(*error_notify_func) (txt);
	    }
	    return (FALSE);
	}
	XDrawString (display, window, gc,
		     (*win_scale).x_offset + (*win_scale).x_pixels -
		     string_pixels,
		     scale_offset - (*font_info).descent,
		     txt, string_length);
	final_scale_start = ( (*win_scale).x_offset + (*win_scale).x_pixels -
			     string_pixels );
	(*win_scale).x_pixels -= ( (double) string_pixels / 2.0 + 0.5 );
	/*  Draw other scale numbers  */
	first_log = (int) log10 ( (*win_scale).x_min );
	for (major_tick_count = 1;
	     major_tick_count < scale_info.num_major_ticks - 1;
	     ++major_tick_count)
	{
	    (void) sprintf (txt, "1E%d", first_log + (int) major_tick_count);
	    string_length = strlen (txt);
	    string_pixels = XTextWidth (font_info, txt, string_length);
	    major_tick_position = ( (*win_scale).x_offset +
				   (int) major_tick_count *
				   (*win_scale).x_pixels /
				   (int) (scale_info.num_major_ticks - 1) );
	    if (major_tick_position - string_pixels / 2 <=
		prev_scale_end + abs_scale_spacing)
	    {
		(void) strcpy (txt, "window not wide enough for scale");
		if (error_notify_func == NULL)
		{
		    (void) fprintf (stderr, "\n%s", txt);
		}
		else
		{
		    (*error_notify_func) (txt);
		}
		return (FALSE);
	    }
	    if (major_tick_position + abs_scale_spacing +
		(int) ( (double) string_pixels / 2.0 + 0.5 )
		>= final_scale_start)
	    {
		(void) strcpy (txt, "window not wide enough for scale");
		if (error_notify_func == NULL)
		{
		    (void) fprintf (stderr, "\n%s", txt);
		}
		else
		{
		    (*error_notify_func) (txt);
		}
		return (FALSE);
	    }
	    /*  Draw the scale number  */
	    XDrawString (display, window, gc,
			 major_tick_position - string_pixels / 2,
			 scale_offset - (*font_info).descent,
			 txt, string_length);
	}
	/*  Plot tick marks  */
	for (major_tick_count = 0;
	     major_tick_count < scale_info.num_major_ticks - 1;
	     ++major_tick_count)
	{
	    major_tick_position = ( (*win_scale).x_offset +
				   (int) major_tick_count *
				   (*win_scale).x_pixels /
				   (int) (scale_info.num_major_ticks - 1) );
	    XDrawLine (display, window, gc,
		       major_tick_position, (*win_scale).y_offset,
		       major_tick_position,
		       (*win_scale).y_offset + pixels_major - 1);
	    XDrawLine (display, window, gc,
		       major_tick_position,
		       (*win_scale).y_offset + (*win_scale).y_pixels - 1,
		       major_tick_position,
		       (*win_scale).y_offset + (*win_scale).y_pixels - 2 -
		       pixels_major);
	    for (minor_tick_count = 2;
		 minor_tick_count <
		 (unsigned int) NUM_MINOR_TICKS_PER_MAJOR_TICK;
		 ++minor_tick_count)
	    {
		/*  Determine tick position  */
		minor_tick_position = ( log10 ( (double) minor_tick_count ) *
				       (double) (*win_scale).x_pixels /
				       (double) (scale_info.num_major_ticks
						 - 1) );
		minor_tick_position += major_tick_position;
		if (minor_tick_count % (int) NUM_MINOR_TICKS_PER_MEDIUM_TICK
		    == 0)
		{
		    tick_width = pixels_medium;
		}
		else
		{
		    tick_width = pixels_minor;
		}
		XDrawLine (display, window, gc,
			   minor_tick_position, (*win_scale).y_offset,
			   minor_tick_position,
			   (*win_scale).y_offset + tick_width - 1);
		XDrawLine (display, window, gc,
			   minor_tick_position,
			   (*win_scale).y_offset + (*win_scale).y_pixels - 1,
			   minor_tick_position,
			   (*win_scale).y_offset + (*win_scale).y_pixels - 2
			   - tick_width);
	    }
	}
	/*  Plot last major tick mark  */
	XDrawLine (display, window, gc,
		   (*win_scale).x_offset + (*win_scale).x_pixels - 1,
		   (*win_scale).y_offset,
		   (*win_scale).x_offset + (*win_scale).x_pixels - 1,
		   (*win_scale).y_offset + pixels_major - 1);
	XDrawLine (display, window, gc,
		   (*win_scale).x_offset + (*win_scale).x_pixels - 1,
		   (*win_scale).y_offset + (*win_scale).y_pixels - 1,
		   (*win_scale).x_offset + (*win_scale).x_pixels - 1,
		   (*win_scale).y_offset + (*win_scale).y_pixels - 2
		   - pixels_major);
	return (TRUE);
    }

    /*  Linear scale  */
    if (fabs ( (*win_scale).x_min ) == 0.0)
    {
	if (fabs ( (*win_scale).x_max ) == 0.0)
	{
	    (void) strcpy (txt, "minimum and maximum are both 0: program bug");
	    if (error_notify_func == NULL)
	    {
		(void) fprintf (stderr, "\n%s", txt);
	    }
	    else
	    {
		(*error_notify_func) (txt);
	    }
	    return (FALSE);
	}
	lin_scale = log10 ( fabs ( (*win_scale).x_max ) );
    }
    else
    {
	lin_scale = log10 ( fabs ( (*win_scale).x_min ) );
	if (lin_scale >= 0.0)
	{
	    /*  Positive exponent  */
	    if (fabs ( (*win_scale).x_max ) != 0.0)
	    {
		if (log10 ( fabs ( (*win_scale).x_max ) ) > lin_scale)
		{
		    lin_scale = log10 ( fabs ( (*win_scale).x_max ) );
		}
	    }
	}
	else
	{
	    /*  Negative exponent  */
	    if (fabs ( (*win_scale).x_max ) != 0.0)
	    {
		if (log10 ( fabs ( (*win_scale).x_max ) ) < lin_scale)
		{
		    lin_scale = log10 ( fabs ( (*win_scale).x_max ) );
		}
	    }
	}
    }
    if ( (lin_scale >= LIN_SCALE_CUTOFF1) || (lin_scale < LIN_SCALE_CUTOFF2) )
    {
	/*  Draw scale multiplier  */
	if (font_height > (*win_scale).y_pixels)
	{
	    (void) strcpy (txt, "window not tall enough for scale multiplier");
	    if (error_notify_func == NULL)
	    {
		(void) fprintf (stderr, "\n%s", txt);
	    }
	    else
	    {
		(*error_notify_func) (txt);
	    }
	    return (FALSE);
	}
	lin_scale = floor (lin_scale);
	(void) sprintf (txt, "*1E%d", (int) lin_scale);
	lin_scale = pow (10.0, -lin_scale);
	string_length = strlen (txt);
	string_pixels = XTextWidth (font_info, txt, string_length);
	if (string_pixels + abs_scale_spacing > (*win_scale).x_pixels)
	{
	    (void) strcpy (txt, "window not wide enough for scale");
	    if (error_notify_func == NULL)
	    {
		(void) fprintf (stderr, "\n%s", txt);
	    }
	    else
	    {
		(*error_notify_func) (txt);
	    }
	    return (FALSE);
	}
	XDrawString (display, window, gc,
		     (*win_scale).x_offset + (*win_scale).x_pixels -
		     string_pixels,
		     scale_offset - (*font_info).descent,
		     txt, string_length);
	/*  Compute further reduced plot area  */
	(*win_scale).x_pixels -= string_pixels + abs_scale_spacing;
    }
    else
    {
	lin_scale = 1.0;
    }
    /*  Draw first scale number  */
    if (lin_scale == 1.0)
    {
	/*  Unscaled  */
	(void) sprintf (txt, "%g", scale_info.first_major_tick);
    }
    else
    {
	/*  Scaled  */
	(void) sprintf (txt, "%g", lin_scale * scale_info.first_major_tick);
    }
    string_length = strlen (txt);
    string_pixels = XTextWidth (font_info, txt, string_length);
    /*  First make sure we are not overunning window boundaries  */
    /*  Loop (it's an iterative process)  */
    new_x_offset = (*win_scale).x_offset;
    while ( ( major_tick_position = ( new_x_offset - (*win_scale).x_pixels *
				     scale_info.first_tick_num /
				     (int) (scale_info.num_ticks - 1)
				     - string_pixels / 2 ) )
	   < (*win_scale).x_offset )
    {
	/*  Need to leave some room  */
	new_x_offset += (*win_scale).x_offset - (major_tick_position - 1);
	(*win_scale).x_pixels -= (*win_scale).x_offset - (major_tick_position
							  - 1);
	if ( (*win_scale).x_pixels < string_length + abs_scale_spacing )
	{
	    (void) strcpy (txt, "window too narrow for scale");
	    if (error_notify_func == NULL)
	    {
		(void) fprintf (stderr, "\n%s", txt);
	    }
	    else
	    {
		(*error_notify_func) (txt);
	    }
	    return (FALSE);
	}
    }
    (*win_scale).x_offset = new_x_offset;
    XDrawString (display, window, gc,
		 major_tick_position, scale_offset - (*font_info).descent,
		 txt, string_length);
    prev_scale_end = (*win_scale).x_offset + string_pixels - 1;
    /*  Draw last scale number  */
    if (lin_scale == 1.0)
    {
	/*  Unscaled  */
	(void) sprintf (txt, "%g",
			scale_info.first_major_tick +
			(double) (scale_info.num_major_ticks - 1) *
			scale_info.tick_interval *
			NUM_MINOR_TICKS_PER_MAJOR_TICK);
    }
    else
    {
	/*  Scaled  */
	(void) sprintf ( txt, "%g",
			lin_scale * (scale_info.first_major_tick +
				     (double) (scale_info.num_major_ticks - 1)
				     * scale_info.tick_interval *
				     NUM_MINOR_TICKS_PER_MAJOR_TICK) );
    }
    string_length = strlen (txt);
    string_pixels = XTextWidth (font_info, txt, string_length);
    /*  First make sure we are not overunning window boundaries  */
    /*  Loop (it's an iterative process)  */
    new_x_pixels = (*win_scale).x_pixels;
    while ( ( major_tick_position = ( (*win_scale).x_offset + new_x_pixels *
				     (-scale_info.first_tick_num +
				      (int) (scale_info.num_major_ticks - 1) *
				      (int) NUM_MINOR_TICKS_PER_MAJOR_TICK) /
				     (int) (scale_info.num_ticks - 1)
				     + (int) ( (double) string_pixels / 2.0 +
					      0.5 ) ) )
	   > (*win_scale).x_offset + (*win_scale).x_pixels )
    {
	/*  Need to leave some room  */
	new_x_pixels -= ( major_tick_position + 1 - ( (*win_scale).x_offset +
						     (*win_scale).x_pixels) );
	if (new_x_pixels < string_length + abs_scale_spacing)
	{
	    (void) strcpy (txt, "window too narrow for scale");
	    if (error_notify_func == NULL)
	    {
		(void) fprintf (stderr, "\n%s", txt);
	    }
	    else
	    {
		(*error_notify_func) (txt);
	    }
	    return (FALSE);
	}
    }
    major_tick_position -= string_pixels;
    (*win_scale).x_pixels = new_x_pixels;
    XDrawString (display, window, gc,
		 major_tick_position, scale_offset - (*font_info).descent,
		 txt, string_length);
    final_scale_start = major_tick_position;
    /*  Draw other scale numbers  */
    for (major_tick_count = 1;
	 major_tick_count < (scale_info.num_major_ticks - 1);
	 ++major_tick_count)
    {
	major_tick = (scale_info.first_major_tick +
		      (double) major_tick_count *
		      scale_info.tick_interval *
		      NUM_MINOR_TICKS_PER_MAJOR_TICK);
	/*  Do a bit of rounding  */
	major_tick = (floor ( (major_tick / scale_info.tick_interval) + 0.5 )
		      * scale_info.tick_interval);
	if (lin_scale == 1.0)
	{
	    /*  Unscaled  */
	    (void) sprintf (txt, "%g", major_tick);
	}
	else
	{
	    /*  Scaled  */
	    (void) sprintf (txt, "%g", lin_scale * major_tick);
	}
	string_length = strlen (txt);
	string_pixels = XTextWidth (font_info, txt, string_length);
	major_tick_position = ( (*win_scale).x_offset + (*win_scale).x_pixels *
			       ( (int) major_tick_count *
				(int) NUM_MINOR_TICKS_PER_MAJOR_TICK -
				scale_info.first_tick_num ) /
			       (int) (scale_info.num_ticks - 1)  );
	if (major_tick_position - string_pixels / 2 <=
	    prev_scale_end + abs_scale_spacing)
	{
	    (void) strcpy (txt, "window not wide enough for scale");
	    if (error_notify_func == NULL)
	    {
		(void) fprintf (stderr, "\n%s", txt);
	    }
	    else
	    {
		(*error_notify_func) (txt);
	    }
	    return (FALSE);
	}
	if (major_tick_position + abs_scale_spacing +
	    (int) ( (double) string_pixels / 2.0 + 0.5 )
	    >= final_scale_start)
	{
	    (void) strcpy (txt, "window not wide enough for scale");
	    if (error_notify_func == NULL)
	    {
		(void) fprintf (stderr, "\n%s", txt);
	    }
	    else
	    {
		(*error_notify_func) (txt);
	    }
	    return (FALSE);
	}
	/*  Draw the scale number  */
	XDrawString (display, window, gc,
		     major_tick_position - string_pixels / 2,
		     scale_offset - (*font_info).descent,
		     txt, string_length);
    }
    /*  Plot tick marks  */
    for (minor_tick_count = 0; minor_tick_count < scale_info.num_ticks;
	 ++minor_tick_count)
    {
	minor_tick_position = ( (*win_scale).x_offset + (*win_scale).x_pixels *
			       (int) minor_tick_count /
			       (int) (scale_info.num_ticks - 1) );
	if ( ( (int) minor_tick_count + scale_info.first_tick_num )
	    % (int) NUM_MINOR_TICKS_PER_MAJOR_TICK == 0 )
	{
	    /*  Major tick  */
	    XDrawLine (display, window, gc,
		       minor_tick_position, (*win_scale).y_offset,
		       minor_tick_position,
		       (*win_scale).y_offset + pixels_major - 1);
	    XDrawLine (display, window, gc,
		       minor_tick_position,
		       (*win_scale).y_offset + (*win_scale).y_pixels - 1,
		       minor_tick_position,
		       (*win_scale).y_offset + (*win_scale).y_pixels - 2 -
		       pixels_major);
	}
	else if ( ( (int) minor_tick_count + scale_info.first_tick_num )
		 % (int) NUM_MINOR_TICKS_PER_MEDIUM_TICK == 0 )
	{
	    /*  Medium tick  */
	    XDrawLine (display, window, gc,
		       minor_tick_position, (*win_scale).y_offset,
		       minor_tick_position,
		       (*win_scale).y_offset + pixels_medium - 1);
	    XDrawLine (display, window, gc,
		       minor_tick_position,
		       (*win_scale).y_offset + (*win_scale).y_pixels - 1,
		       minor_tick_position,
		       (*win_scale).y_offset + (*win_scale).y_pixels - 2 -
		       pixels_medium);
	}
	else
	{
	    /*  Minor tick  */
	    if (scale_info.num_ticks <= MAX_MINOR_TICKS)
	    {
		/*  May plot minor tick  */
		XDrawLine (display, window, gc,
			   minor_tick_position, (*win_scale).y_offset,
			   minor_tick_position,
			   (*win_scale).y_offset + pixels_minor - 1);
		XDrawLine (display, window, gc,
			   minor_tick_position,
			   (*win_scale).y_offset + (*win_scale).y_pixels - 1,
			   minor_tick_position,
			   (*win_scale).y_offset + (*win_scale).y_pixels - 2
			   - pixels_minor);
	    }
	}
    }
    return (TRUE);
}   /*  End Function ax_draw_abscissa_scale  */

/*PUBLIC_FUNCTION*/
void ax_set_zoom_h_info (x1, x2, abs_zoomed, win_scale)
/*  This routine will set the horizontal zoom information based upon the two
    horizontal pixel locations given by  x1  and  x2  .
    The routine modifies the  variable pointed to by  abs_zoomed  and the
    structure pointed to by  win_scale  .
    The routine returns nothing.
*/
int x1;
int x2;
flag *abs_zoomed;
struct win_scale_type *win_scale;
{
    static char function_name[] = "ax_set_zoom_h_info";
    double min;
    double max;

    if (x1 < x2)
    {
	min = (double) x1;
	max = (double) x2;
    }
    else
    {
	min = (double) x2;
	max = (double) x1;
    }
    min -= (double) (*win_scale).x_offset;
    max -= (double) (*win_scale).x_offset;
    min /= (double) ((*win_scale).x_pixels - 1);
    max /= (double) ( (*win_scale).x_pixels - 1 );
    if ( (*win_scale).x_log == TRUE )
    {
	/*  Logarithmic scale  */
	min *= log10 ( (*win_scale).x_max / (*win_scale).x_min );
	min = (*win_scale).x_min * pow (10.0, min);
	max *= log10 ( (*win_scale).x_max / (*win_scale).x_min );
	max = (*win_scale).x_min * pow (10.0, max);
    }
    else
    {
	/*  Linear scale  */
	min *= ( (*win_scale).x_max - (*win_scale).x_min );
	min += (*win_scale).x_min;
	max *= ( (*win_scale).x_max - (*win_scale).x_min );
	max += (*win_scale).x_min;
    }
    (*win_scale).x_min = min;
    (*win_scale).x_max = max;
    *abs_zoomed = TRUE;
}   /*  End Function ax_set_zoom_h_info  */

/*PUBLIC_FUNCTION*/
void ax_set_zoom_v_info (y1, y2, ord_zoomed, win_scale)
/*  This routine will set the vertical zoom information based upon the two
    vertical pixel locations given by  y1  and  y2  .
    The routine modifies the  variable pointed to by  ord_zoomed  and the
    structure pointed to by  win_scale  .
    The routine returns nothing.
*/
int y1;
int y2;
flag *ord_zoomed;
struct win_scale_type *win_scale;
{
    static char function_name[] = "ax_set_zoom_v_info";
    double min;
    double max;

    if (y1 < y2)
    {
	min = (double) y2;
	max = (double) y1;
    }
    else
    {
	min = (double) y1;
	max = (double) y2;
    }
    min -= (double) (*win_scale).y_offset;
    max -= (double) (*win_scale).y_offset;
    min /= (double) ( (*win_scale).y_pixels - 1 );
    max /= (double) ( (*win_scale).y_pixels - 1 );
    min = 1.0 - min;
    max = 1.0 - max;
    if ( (*win_scale).y_log == TRUE )
    {
	/*  Logarithmic scale  */
	min *= log10 ( (*win_scale).y_max / (*win_scale).y_min );
	min = (*win_scale).y_min * pow (10.0, min);
	max *= log10 ( (*win_scale).y_max / (*win_scale).y_min );
	max = (*win_scale).y_min * pow (10.0, max);
    }
    else
    {
	/*  Linear scale  */
	min *= ( (*win_scale).y_max - (*win_scale).y_min );
	min += (*win_scale).y_min;
	max *= ( (*win_scale).y_max - (*win_scale).y_min );
	max += (*win_scale).y_min;
    }
    (*win_scale).y_min = min;
    (*win_scale).y_max = max;
    *ord_zoomed = TRUE;
}   /*  End Function ax_set_zoom_h_info  */

/*PUBLIC_FUNCTION*/
flag ax_unset_zoom_info (abs_zoomed, ord_zoomed)
/*  This routine will unset the zoom information pointed to by  abs_zoomed  and
    ord_zoomed  .
    The routine returns TRUE if there was previous zoom information to undo.
*/
flag *abs_zoomed;
flag *ord_zoomed;
{
    static char function_name[] = "ax_unset_zoom_info";
    flag return_value = FALSE;

    if (*abs_zoomed == TRUE)
    {
	return_value = TRUE;
    }
    if (*ord_zoomed == TRUE)
    {
	return_value = TRUE;
    }
    *abs_zoomed = FALSE;
    *ord_zoomed = FALSE;
    return (return_value);
}   /*  End Function ax_unset_zoom_info  */

/*PUBLIC_FUNCTION*/
flag ax_verify_crosshair_location (x, y, win_scale)
/*  This routine will determine if the crosshair location given by  x  and  y
    is within the plotting area.
    The window scaling information must be pointed to by  win_scale  .
    If the crosshair is within the plotting area, the routine will return TRUE,
    else it will return FALSE.
*/
int x;
int y;
struct win_scale_type *win_scale;
{
    static char function_name[] = "ax_verify_crosshair_location";
    if (x < (*win_scale).x_offset)
    {
	return (FALSE);
    }
    if (x > (*win_scale).x_offset + (*win_scale).x_pixels - 1)
    {
	return (FALSE);
    }
    if (y < (*win_scale).y_offset)
    {
	return (FALSE);
    }
    if (y > (*win_scale).y_offset + (*win_scale).y_pixels - 1)
    {
	return (FALSE);
    }
    return (TRUE);
}   /*  End Function ax_verify_crosshair_location  */

/*PUBLIC_FUNCTION*/
double ax_choose_ord_intvl (input_interval)
/*  This routine will choose an ordinate interval which is equal to or slightly
    greater than the input interval specified by  input_interval  .
    The choosen interval is determined by some sensible rules.
    The routine returns the choosen interval.
*/
double input_interval;
{
    static char function_name[] = "ax_choose_ord_intvl";
    unsigned int choice_count;
    double base_scale;
    static double ord_interval_choices[MAX_ORD_INTERVAL_CHOICES] =
    {
	1.0, 1.5, 2.0, 2.5, 5.0
    };

    if (input_interval < 0.0)
    {
	(void) fprintf (stderr, "\ninput interval: %e less than zero",
			input_interval);
	a_prog_bug (function_name);
    }
    if (input_interval == 0.0)
    {
	return (input_interval);
    }

    input_interval *= ORDINATE_TRACE_SEPARATION_FACTOR;
    base_scale = pow ( 10.0, floor (log10 (input_interval) ) );
    for (choice_count = 0; choice_count < MAX_ORD_INTERVAL_CHOICES;
	 ++choice_count)
    {
	if (input_interval <= base_scale * ord_interval_choices[choice_count])
	{
	    return (base_scale * ord_interval_choices[choice_count]);
	}
    }
    return (10.0 *  base_scale);
}   /*  End Function ax_choose_ord_intvl  */

/*PUBLIC_FUNCTION*/
double ax_pixel_to_abscissa (x, win_scale)
/*  This routine will convert a horizontal pixel position to an abscissa value.
    The horizontal pixel position must be given by  x  .
    The window scaling information must be pointed to by  win_scale  .
    The routine returns the abscissa value.
*/
int x;
struct win_scale_type *win_scale;
{
    static char function_name[] = "ax_pixel_to_abscissa";
    double abs_value;

    abs_value = (double) (x - (*win_scale).x_offset);
    abs_value /= (double) ( (*win_scale).x_pixels - 1 );
    if ( (*win_scale).x_log == TRUE )
    {
	/*  Logarithmic scale  */
	abs_value *= log10 ( (*win_scale).x_max / (*win_scale).x_min );
	abs_value = (*win_scale).x_min * pow (10.0, abs_value);
    }
    else
    {
	/*  Linear scale  */
	abs_value *= ( (*win_scale).x_max - (*win_scale).x_min );
	abs_value += (*win_scale).x_min;
    }
    return (abs_value);
}   /*  End Function ax_pixel_to_abscissa  */

/*PUBLIC_FUNCTION*/
double ax_pixel_to_ordinate (y, win_scale)
/*  This routine will convert a vertical pixel position to an ordinate value.
    The vertical pixel position must be given by  x  .
    The window scaling information must be pointed to by  win_scale  .
    The routine returns the ordinate value.
*/
int y;
struct win_scale_type *win_scale;
{
    static char function_name[] = "ax_pixel_to_ordinate";
    double ord_value;

    ord_value = (double) (y - (*win_scale).y_offset);
    ord_value /= (double) ( (*win_scale).y_pixels - 1 );
    ord_value = 1.0 - ord_value;
    if ( (*win_scale).y_log == TRUE )
    {
	/*  Logarithmic scale  */
	ord_value *= log10 ( (*win_scale).y_max / (*win_scale).y_min );
	ord_value = (*win_scale).y_min * pow (10.0, ord_value);
    }
    else
    {
	/*  Linear scale  */
	ord_value *= ( (*win_scale).y_max - (*win_scale).y_min );
	ord_value += (*win_scale).y_min;
    }
    return (ord_value);
}   /*  End Function ax_pixel_to_ordinate  */
