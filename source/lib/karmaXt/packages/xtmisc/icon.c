/*LINTLIBRARY*/
/*  icon.c

    This code provides miscellaneous routines for the X Intrinsics toolkit

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

/*  This file contains miscellaneous routines needed for the using the Xt
  toolkit.


    Written by      Richard Gooch   17-MAR-1995

    Updated by      Richard Gooch   16-APR-1996: Changed to new documentation
  format.

    Last updated by Richard Gooch   26-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.


*/
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <errno.h>
#include <X11/Xlib.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
/*  Do not include  karma.h  first, since CONST may be screwed otherwise. This
    applies to broken versions of  X11/Intrinsic.h  */
#include <karma_xtmisc.h>
#include <karma_a.h>

#define DEFAULT_ICON_WIDTH 64
#define DEFAULT_ICON_HEIGHT 64


/*  Private functions  */
STATIC_FUNCTION (void get_closest_size,
		 (XIconSize size, int *width, int *height) );


/*  Private data  */


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
void xtmisc_set_icon ( Widget top_level,
		       void (*icon_func) (Display *display, Pixmap pixmap,
					  int width, int height) )
/*  [SUMMARY] Create an icon for an application's top level widget.
    <top_level> The top level widget.
    <icon_func> The function to use when writing the icon. The prototype
    function is [<XTMISC_PROTO_icon_func>].
    [RETURNS] Nothing.
*/
{
    int icon_width = -1;
    int icon_height = -1;
    int tmp_width, tmp_height;
    int num_icon_sizes, count;
    Pixmap icon_pixmap;
    Display *display;
    Screen *screen;
    XIconSize *icon_size_list;
    static char function_name[] = "xtmisc_set_icon";

    /*  Determine the display (needed later)  */
    if ( ( display = XtDisplay (top_level) ) == NULL )
    {
	(void) fprintf (stderr, "Error getting display pointer\n");
	a_prog_bug (function_name);
    }
    screen = XtScreen (top_level);
    /*  Determine window manager preferred icon sizes  */
    if (XGetIconSizes (display, RootWindowOfScreen (screen), &icon_size_list,
		       &num_icon_sizes) == 0)
    {
	/*  No window manager preferences: use appliaction default  */
	icon_width = DEFAULT_ICON_WIDTH;
	icon_height = DEFAULT_ICON_HEIGHT;
    }
    else
    {
	/*  Get the icon size equal to or greater than default if possible,
	    else get largest.  */
	get_closest_size (icon_size_list[0], &icon_width, &icon_height);
	for (count = 1; count < num_icon_sizes; ++count)
	{
	    if ( (icon_width == DEFAULT_ICON_WIDTH) &&
		 (icon_height == DEFAULT_ICON_HEIGHT) ) continue;
	    /*  Existing size not ideal  */
	    get_closest_size (icon_size_list[count], &tmp_width, &tmp_height);
	    if ( (icon_width < DEFAULT_ICON_WIDTH) ||
		 (icon_height < DEFAULT_ICON_HEIGHT) )
	    {
		/*  Existing size is smaller than desired  */
		if ( (tmp_width >= icon_width) && (tmp_height >= icon_height) )
		{
		    /*  This one is bigger: take it  */
		    icon_width = tmp_width;
		    icon_height = tmp_height;
		}
	    }
	    if ( (icon_width > DEFAULT_ICON_WIDTH) ||
		 (icon_height > DEFAULT_ICON_HEIGHT) )
	    {
		/*  Existing size is partially larger than desired  */
		if ( (tmp_width < DEFAULT_ICON_WIDTH) ||
		     (tmp_height < DEFAULT_ICON_HEIGHT) )
		{
		    /*  This one is not big enough  */
		    continue;
		}
		/*  This one is big enough, but maybe too big  */
		if ( (icon_width > DEFAULT_ICON_WIDTH) &&
		     (tmp_width > icon_width) ) continue;
		if ( (icon_height > DEFAULT_ICON_HEIGHT) &&
		     (tmp_height > icon_height) ) continue;
		/*  Looks like we're better off  */
		icon_width = tmp_width;
		icon_height = tmp_height;
	    }
	}
	XFree ( (char *) icon_size_list );
/*
	(void) fprintf (stderr, "preferred size: %d  %d\n",
			icon_width, icon_height);
*/
    }
    icon_pixmap = XCreatePixmap (display, RootWindowOfScreen (screen),
				 icon_width, icon_height, 1);
    (*icon_func) (display, icon_pixmap, icon_width, icon_height);
    XtVaSetValues (top_level,
		   XtNiconPixmap, icon_pixmap, NULL);
}   /*  End Function xtmisc_set_icon  */


/*  Private functions follow  */

static void get_closest_size (XIconSize size, int *width, int *height)
{
    if ( (size.max_width < DEFAULT_ICON_WIDTH) ||
	 (size.max_height < DEFAULT_ICON_HEIGHT) )
    {
	/*  Maximum is smaller than preferred  */
	*width = size.max_width;
	*height = size.max_height;
	return;
    }
    if ( (size.min_width > DEFAULT_ICON_WIDTH) ||
	 (size.min_height > DEFAULT_ICON_HEIGHT) )
    {
	/*  Minimum is larger than preferred  */
	*width = size.min_width;
	*height = size.min_height;
	return;
    }
    /*  Truth is somewhere in between  */
    *width = DEFAULT_ICON_WIDTH - (DEFAULT_ICON_WIDTH -
				   size.min_width) % size.width_inc;
    *height = DEFAULT_ICON_HEIGHT - (DEFAULT_ICON_HEIGHT -
				     size.min_height) % size.height_inc;
}   /*  End Function get_closest_size  */
