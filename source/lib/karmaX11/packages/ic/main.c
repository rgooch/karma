/*LINTLIBRARY*/
/*  main.c

    This code provides routines to create X icons for some modules.

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

/*  This file contains routines used in creating standard icons.


    Written by      Richard Gooch   29-AUG-1992

    Updated by      Richard Gooch   4-FEB-1993

    Last updated by Richard Gooch   26-NOV-1994: Moved to  packages/ic/main.c


*/
#include <stdio.h>
#include <math.h>
#include "karma.h"
#include <karma_a.h>
#include <karma_n.h>
#include <karma_ic.h>
#include <X11/Xutil.h>

#define KSCAT_Y_PIXELS (unsigned int) 5
#define KSCAT_OFFSET (double) 0.1
#define KSCAT_RISE (double) 0.7
#define KSCAT_VARIANCE (double) 0.1
#define FUNCGEN_CYCLES (unsigned int) 3
#define KARMA_CM_LINES (unsigned int) 18
#define DATASOURCE_BORDER (double) 0.1
#define DATASOURCE_LINE_LENGTH (double) 0.2
#define DATASINK_BORDER (double) 0.1
#define DATASINK_LINE_LENGTH (double) 0.2
#define DATAFILTER_BORDER (double) 0.1
#define DATAFILTER_LINE_LENGTH (double) 0.2

static GC gc = 0;

/*PUBLIC_FUNCTION*/
void ic_write_kplot_icon (display, pixmap, width, height)
/*  This routine will write the icon pixmap required for the  kplot  module.
    The display must be pointed to by  display  and the pixmap to write to must
    be pointed to by  pixmap  .
    The width of the pixmap must be given by  width  .
    The height of the pixmap must be given by  height  .
    The routine returns nothing.
*/
Display *display;
Pixmap pixmap;
int width;
int height;
{
    static char function_name[] = "ic_write_kplot_icon";
    int icon_x;
    int icon_y;
    int old_x;
    int old_y;
    int pixel_count;
    double x;
    double y;
    XGCValues gcvalues;
    extern GC gc;

    if (gc == 0)
    {
	/*  Create graphics context  */
	gcvalues.foreground = 0;
	gcvalues.background = 1;
	gc = XCreateGC (display, pixmap, GCForeground | GCBackground,
			&gcvalues);
    }
    gcvalues.foreground = 0;
    gcvalues.background = 1;
    XChangeGC (display, gc, GCForeground | GCBackground, &gcvalues);
    /*  Clear the icon pixmap  */
    XFillRectangle (display, pixmap, gc, 0, 0, width, height);
    gcvalues.foreground = 1;
    gcvalues.background = 0;
    XChangeGC (display, gc, GCForeground | GCBackground, &gcvalues);
    /*  Put a border around our icon  */
    XDrawRectangle (display, pixmap, gc, 0, 0, width - 1, height - 1);
    /*  Draw our icon image  */
    old_x = 0;
    old_y = height;
    /*  The curve drawn in the icon is a cubic with the following fixed points:
	(0,0)  (1/3, 2/3)  (2/3, 1/3)
	In addition, it has a maxima at x = 1/3 and a minima at x = 2/3
	The function is: y = 9*x^3 - 13.5*x^2 + 5.5*x
	*/
    for (pixel_count = 0; pixel_count < width; ++pixel_count)
    {
	/*  Determine x co-ordinate  */
	x = (double) pixel_count / (double) width;
	/*  Compute function  */
	y = 9.0 * x * x * x - 13.5 * x * x + 5.5 * x;
	icon_x = (int) (pixel_count + 0.5);
	icon_y = height - (int) (y * (double) height + 0.5);
	XDrawLine (display, pixmap, gc,
		   old_x, old_y, icon_x, icon_y);
	old_x = icon_x;
	old_y = icon_y;
    }
}   /*  End Function ic_write_kplot_icon  */

/*PUBLIC_FUNCTION*/
void ic_write_kscat_icon (display, pixmap, width, height)
/*  This routine will write the icon pixmap required for the  kscat  module.
    The display must be pointed to by  display  and the pixmap to write to must
    be pointed to by  pixmap  .
    The width of the pixmap must be given by  width  .
    The height of the pixmap must be given by  height  .
    The routine returns nothing.
*/
Display *display;
Pixmap pixmap;
int width;
int height;
{
    static char function_name[] = "ic_write_kscat_icon";
    int icon_x;
    int icon_y;
    int x_count;
    unsigned int y_count;
    double x;
    double y;
    XGCValues gcvalues;
    extern GC gc;

    if (gc == 0)
    {
	/*  Create graphics context  */
	gcvalues.foreground = 0;
	gcvalues.background = 1;
	gc = XCreateGC (display, pixmap, GCForeground | GCBackground,
			&gcvalues);
    }
    gcvalues.foreground = 0;
    gcvalues.background = 1;
    XChangeGC (display, gc, GCForeground | GCBackground, &gcvalues);
    /*  Clear the icon pixmap  */
    XFillRectangle (display, pixmap, gc, 0, 0, width, height);
    gcvalues.foreground = 1;
    gcvalues.background = 0;
    XChangeGC (display, gc, GCForeground | GCBackground, &gcvalues);
    /*  Put a border around our icon  */
    XDrawRectangle (display, pixmap, gc, 0, 0, width - 1, height - 1);
    /*  Draw our icon image  */
    for (x_count = 0; x_count < width; ++x_count)
    {
	/*  Determine x co-ordinate  */
	x = (double) x_count / (double) width;
	for (y_count = 0; y_count < KSCAT_Y_PIXELS; ++y_count)
	{
	    /*  Gaussian distributed random number  */
	    y = n_gaussian () * KSCAT_VARIANCE + x * KSCAT_RISE + KSCAT_OFFSET;
	    if ( (y >= 0.0) && (y <= 1.0) )
	    {
		icon_x = (int) (x_count + 0.5);
		icon_y = height - (int) (y * (double) height + 0.5);
		XDrawPoint (display, pixmap, gc, icon_x, icon_y);
	    }
	}
    }
}   /*  End Function ic_write_kscat_icon  */

/*PUBLIC_FUNCTION*/
void ic_write_funcgen_icon (display, pixmap, width, height)
/*  This routine will write the icon pixmap required for the  funcgen  module.
    The display must be pointed to by  display  and the pixmap to write to must
    be pointed to by  pixmap  .
    The width of the pixmap must be given by  width  .
    The height of the pixmap must be given by  height  .
    The routine returns nothing.
*/
Display *display;
Pixmap pixmap;
int width;
int height;
{
    static char function_name[] = "ic_write_funcgen_icon";
    int cycle_count;
    double seg_width;
    XGCValues gcvalues;
    extern GC gc;

    if (gc == 0)
    {
	/*  Create graphics context  */
	gcvalues.foreground = 0;
	gcvalues.background = 1;
	gc = XCreateGC (display, pixmap, GCForeground | GCBackground,
			&gcvalues);
    }
    gcvalues.foreground = 0;
    gcvalues.background = 1;
    XChangeGC (display, gc, GCForeground | GCBackground, &gcvalues);
    /*  Clear the icon pixmap  */
    XFillRectangle (display, pixmap, gc, 0, 0, width, height);
    gcvalues.foreground = 1;
    gcvalues.background = 0;
    XChangeGC (display, gc, GCForeground | GCBackground, &gcvalues);
    /*  Put a border around our icon  */
    XDrawRectangle (display, pixmap, gc, 0, 0, width - 1, height - 1);
    /*  Draw our icon image  */
    /*  The function drawn in the icon is a triangle wave with n cycles  */
    seg_width = (double) width / (double) (FUNCGEN_CYCLES * 2);
    for (cycle_count = 0; cycle_count < FUNCGEN_CYCLES; ++cycle_count)
    {
	/*  Draw rising line  */
	XDrawLine (display, pixmap, gc,
		   (int) ( (double) (cycle_count * 2) * seg_width + 0.5 ),
		   height - 1,
		   (int) ( (double) ( (cycle_count * 2) + 1 )
			  * seg_width + 0.5 ),
		   0);
	/*  Draw falling line  */
	XDrawLine (display, pixmap, gc,
		   (int) ( (double) ( (cycle_count * 2) + 1 )
			  * seg_width + 0.5 ),
		   0,
		   (int) ( (double) ( (cycle_count * 2) + 2 )
			  * seg_width + 0.5 ),
		   height - 1);
    }
}   /*  End Function ic_write_funcgen_icon  */

/*PUBLIC_FUNCTION*/
void ic_write_kimage_icon (display, pixmap, width, height)
/*  This routine will write the icon pixmap required for the  kimage  module.
    The display must be pointed to by  display  and the pixmap to write to must
    be pointed to by  pixmap  .
    The width of the pixmap must be given by  width  .
    The height of the pixmap must be given by  height  .
    The routine returns nothing.
*/
Display *display;
Pixmap pixmap;
int width;
int height;
{
    static char function_name[] = "ic_write_kimage_icon";
    int x;
    int y;
    int diameter;
    XGCValues gcvalues;
    extern GC gc;

    if (gc == 0)
    {
	/*  Create graphics context  */
	gcvalues.foreground = 0;
	gcvalues.background = 1;
	gc = XCreateGC (display, pixmap, GCForeground | GCBackground,
			&gcvalues);
    }
    gcvalues.foreground = 0;
    gcvalues.background = 1;
    XChangeGC (display, gc, GCForeground | GCBackground, &gcvalues);
    /*  Clear the icon pixmap  */
    XFillRectangle (display, pixmap, gc, 0, 0, width, height);
    gcvalues.foreground = 1;
    gcvalues.background = 0;
    XChangeGC (display, gc, GCForeground | GCBackground, &gcvalues);
    /*  Put a border around our icon  */
    XDrawRectangle (display, pixmap, gc, 0, 0, width - 1, height - 1);
    /*  Draw our icon image  */
    if (width < height)
    {
	diameter = width / 3;
    }
    else
    {
	diameter = height / 3;
    }
    x = (width - diameter) / 2;
    y = (height - diameter) / 2;
    XDrawArc (display, pixmap, gc,
	      x, y, diameter, diameter,
	      0, 23040);
    XFillArc (display, pixmap, gc,
	      x, y, diameter, diameter,
	      0, 23040);
    XDrawLine (display, pixmap, gc,
	       width / 2, 0, width / 2, height - 1);
    XDrawLine (display, pixmap, gc,
	       0, height / 2, width - 1, height / 2);
}   /*  End Function ic_write_kimage_icon  */

struct line_type
{
    int x1;
    int y1;
    int x2;
    int y2;
};

/*PUBLIC_FUNCTION*/
void ic_write_karma_cm_icon (display, pixmap, width, height)
/*  This routine will write the icon pixmap required for the  karma_cm  module.
    The display must be pointed to by  display  and the pixmap to write to must
    be pointed to by  pixmap  .
    The width of the pixmap must be given by  width  .
    The height of the pixmap must be given by  height  .
    The routine returns nothing.
*/
Display *display;
Pixmap pixmap;
int width;
int height;
{
    static char function_name[] = "ic_write_karma_cm_icon";
    unsigned int line_count;
    static struct line_type lines[KARMA_CM_LINES] =
    {
	{0, 4, 2, 4},
	{2, 1, 2, 7},
	{2, 1, 5, 1},
	{2, 7, 5, 7},
	{5, 1, 5, 3},
	{5, 3, 7, 3},
	{7, 3, 7, 5},
	{7, 5, 5, 5},
	{5, 5, 5, 7},
	{6, 1, 10, 1},
	{10, 1, 10, 7},
	{10, 7, 6, 7},
	{6, 7, 6, 6},
	{6, 6, 8, 6},
	{8, 6, 8, 2},
	{8, 2, 6, 2},
	{6, 2, 6, 1},
	{10, 4, 12, 4}
    };
    XGCValues gcvalues;
    extern GC gc;

    if (gc == 0)
    {
	/*  Create graphics context  */
	gcvalues.foreground = 0;
	gcvalues.background = 1;
	gc = XCreateGC (display, pixmap, GCForeground | GCBackground,
			&gcvalues);
    }
    gcvalues.foreground = 0;
    gcvalues.background = 1;
    XChangeGC (display, gc, GCForeground | GCBackground, &gcvalues);
    /*  Clear the icon pixmap  */
    XFillRectangle (display, pixmap, gc, 0, 0, width, height);
    gcvalues.foreground = 1;
    gcvalues.background = 0;
    XChangeGC (display, gc, GCForeground | GCBackground, &gcvalues);
    /*  Put a border around our icon  */
    XDrawRectangle (display, pixmap, gc, 0, 0, width - 1, height - 1);
    /*  Draw our icon image  */
    for (line_count = 0; line_count < KARMA_CM_LINES; ++line_count)
    {
	XDrawLine (display, pixmap, gc,
		   lines[line_count].x1 * width / 12,
		   height - lines[line_count].y1 * height / 8,
		   lines[line_count].x2 * width / 12,
		   height - lines[line_count].y2 * height / 8);
    }
}   /*  End Function ic_write_karma_cm_icon  */

/*PUBLIC_FUNCTION*/
void ic_write_datasource_icon (display, pixmap, width, height)
/*  This routine will write the icon pixmap required for generic data source
    modules.
    The display must be pointed to by  display  and the pixmap to write to must
    be pointed to by  pixmap  .
    The width of the pixmap must be given by  width  .
    The height of the pixmap must be given by  height  .
    The routine returns nothing.
*/
Display *display;
Pixmap pixmap;
int width;
int height;
{
    static char function_name[] = "ic_write_datasource_icon";
    int x1;
    int y1;
    int x2;
    int y2;
    XGCValues gcvalues;
    extern GC gc;

    if (gc == 0)
    {
	/*  Create graphics context  */
	gcvalues.foreground = 0;
	gcvalues.background = 1;
	gc = XCreateGC (display, pixmap, GCForeground | GCBackground,
			&gcvalues);
    }
    gcvalues.foreground = 0;
    gcvalues.background = 1;
    XChangeGC (display, gc, GCForeground | GCBackground, &gcvalues);
    /*  Clear the icon pixmap  */
    XFillRectangle (display, pixmap, gc, 0, 0, width, height);
    gcvalues.foreground = 1;
    gcvalues.background = 0;
    XChangeGC (display, gc, GCForeground | GCBackground, &gcvalues);
    /*  Put a border around our icon  */
    XDrawRectangle (display, pixmap, gc, 0, 0, width - 1, height - 1);
    /*  Draw our icon image  */
    x1 = (int) (DATASOURCE_BORDER * (double) width + 0.5);
    x2 = (int) ( (1.0 - DATASOURCE_LINE_LENGTH) * (double) width + 0.5 );
    y1 = (int) (DATASOURCE_BORDER * (double) height + 0.5);
    y2 = (int) ( (1.0 - DATASOURCE_BORDER) * (double) height + 0.5 );
    XDrawRectangle (display, pixmap, gc,
		    x1, y1, x2 - x1, y2 - y1);
    XDrawLine (display, pixmap, gc,
	       x2, height / 2, width - 1, height / 2);
}   /*  End Function ic_write_datasource_icon  */

/*PUBLIC_FUNCTION*/
void ic_write_datasink_icon (display, pixmap, width, height)
/*  This routine will write the icon pixmap required for generic data sink
    modules.
    The display must be pointed to by  display  and the pixmap to write to must
    be pointed to by  pixmap  .
    The width of the pixmap must be given by  width  .
    The height of the pixmap must be given by  height  .
    The routine returns nothing.
*/
Display *display;
Pixmap pixmap;
int width;
int height;
{
    static char function_name[] = "ic_write_datasink_icon";
    int x1;
    int y1;
    int x2;
    int y2;
    XGCValues gcvalues;
    extern GC gc;

    if (gc == 0)
    {
	/*  Create graphics context  */
	gcvalues.foreground = 0;
	gcvalues.background = 1;
	gc = XCreateGC (display, pixmap, GCForeground | GCBackground,
			&gcvalues);
    }
    gcvalues.foreground = 0;
    gcvalues.background = 1;
    XChangeGC (display, gc, GCForeground | GCBackground, &gcvalues);
    /*  Clear the icon pixmap  */
    XFillRectangle (display, pixmap, gc, 0, 0, width, height);
    gcvalues.foreground = 1;
    gcvalues.background = 0;
    XChangeGC (display, gc, GCForeground | GCBackground, &gcvalues);
    /*  Put a border around our icon  */
    XDrawRectangle (display, pixmap, gc, 0, 0, width - 1, height - 1);
    /*  Draw our icon image  */
    x1 = (int) (DATASINK_LINE_LENGTH * (double) width + 0.5);
    x2 = (int) ( (1.0 - DATASINK_BORDER) * (double) width + 0.5 );
    y1 = (int) (DATASINK_BORDER * (double) height + 0.5);
    y2 = (int) ( (1.0 - DATASINK_BORDER) * (double) height + 0.5 );
    XDrawRectangle (display, pixmap, gc,
		    x1, y1, x2 - x1, y2 - y1);
    XDrawLine (display, pixmap, gc,
	       0, height / 2, x1 - 1, height / 2);
}   /*  End Function ic_write_datasink_icon  */

/*PUBLIC_FUNCTION*/
void ic_write_datafilter_icon (display, pixmap, width, height)
/*  This routine will write the icon pixmap required for generic data filter
    modules.
    The display must be pointed to by  display  and the pixmap to write to must
    be pointed to by  pixmap  .
    The width of the pixmap must be given by  width  .
    The height of the pixmap must be given by  height  .
    The routine returns nothing.
*/
Display *display;
Pixmap pixmap;
int width;
int height;
{
    static char function_name[] = "ic_write_datafilter_icon";
    int x1;
    int y1;
    int x2;
    int y2;
    XGCValues gcvalues;
    extern GC gc;

    if (gc == 0)
    {
	/*  Create graphics context  */
	gcvalues.foreground = 0;
	gcvalues.background = 1;
	gc = XCreateGC (display, pixmap, GCForeground | GCBackground,
			&gcvalues);
    }
    gcvalues.foreground = 0;
    gcvalues.background = 1;
    XChangeGC (display, gc, GCForeground | GCBackground, &gcvalues);
    /*  Clear the icon pixmap  */
    XFillRectangle (display, pixmap, gc, 0, 0, width, height);
    gcvalues.foreground = 1;
    gcvalues.background = 0;
    XChangeGC (display, gc, GCForeground | GCBackground, &gcvalues);
    /*  Put a border around our icon  */
    XDrawRectangle (display, pixmap, gc, 0, 0, width - 1, height - 1);
    /*  Draw our icon image  */
    x1 = (int) (DATAFILTER_LINE_LENGTH * (double) width + 0.5);
    x2 = (int) ( (1.0 - DATAFILTER_LINE_LENGTH) * (double) width + 0.5 );
    y1 = (int) (DATAFILTER_BORDER * (double) height + 0.5);
    y2 = (int) ( (1.0 - DATAFILTER_BORDER) * (double) height + 0.5 );
    XDrawRectangle (display, pixmap, gc,
		    x1, y1, x2 - x1, y2 - y1);
    XFillRectangle (display, pixmap, gc,
		    x1, y1, x2 - x1, y2 - y1);
    XDrawLine (display, pixmap, gc,
	       0, height / 2, x1 - 1, height / 2);
    XDrawLine (display, pixmap, gc,
	       x2, height / 2, width - 1, height / 2);
}   /*  End Function ic_write_datafilter_icon  */
