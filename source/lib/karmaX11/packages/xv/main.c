/*LINTLIBRARY*/
/*  main.c

    This code provides miscellaneous routines for the X Window system

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

/*  This file contains miscellaneous routines needed for the using the X
  Window system.


    Written by      Richard Gooch   15-SEP-1996

    Last updated by Richard Gooch   3-DEC-1996


*/
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <errno.h>
#include <karma.h>
#include <karma_xv.h>
#include <karma_a.h>


/*  Private functions  */


/*  Private data  */


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
void xv_get_vinfos (Screen *screen, XVisualInfo **pseudocolour,
		    XVisualInfo **truecolour, XVisualInfo **directcolour)
/*  [SUMMARY] Get supported visuals available on a screen.
    <screen> The X Window screen.
    <pseudocolour> A PseudoColour XVisualInfo pointer will be written here. If
    this is NULL, nothing is written here. If no PseudoColour visual is
    supported, NULL is written here.
    <truecolour> A TrueColour XVisualInfo pointer will be written here. If this
    is NULL, nothing is written here. If no TrueColour visual is supported,
    NULL is written here.
    <directcolour> A DirectColour XVisualInfo pointer will be written here. If
    this is NULL, nothing is written here. If no DirectColour visual is
    supported, NULL is written here.
    [NOTE] The XVisualInfo structures returned must be freed by XFree()
    [RETURNS] Nothing.
*/
{
    int num_vinfos;
    XVisualInfo vinfo_template;

    vinfo_template.screen = XScreenNumberOfScreen (screen);
    vinfo_template.colormap_size = 256;
    if (pseudocolour != NULL)
    {
	/*  Get PseudoColour visual  */
	vinfo_template.depth = 8;
	vinfo_template.class = PseudoColor;
	*pseudocolour = XGetVisualInfo (XDisplayOfScreen (screen),
					VisualScreenMask | VisualDepthMask |
					VisualClassMask |
					VisualColormapSizeMask,
					&vinfo_template, &num_vinfos);
    }
    if (truecolour != NULL)
    {
	/*  Get TrueColour visual  */
	vinfo_template.depth = 24;
	vinfo_template.class = TrueColor;
	*truecolour = XGetVisualInfo (XDisplayOfScreen (screen),
				      VisualScreenMask | VisualDepthMask |
				      VisualClassMask | VisualColormapSizeMask,
				      &vinfo_template, &num_vinfos);
    }
    if (directcolour != NULL)
    {
	/*  Get DirectColour visual  */
	vinfo_template.depth = 24;
	vinfo_template.class = DirectColor;
	*directcolour = XGetVisualInfo (XDisplayOfScreen (screen),
					VisualScreenMask | VisualDepthMask |
					VisualClassMask |
					VisualColormapSizeMask,
					&vinfo_template, &num_vinfos);
    }
}   /*  End Function xv_get_vinfos  */

/*PUBLIC_FUNCTION*/
void xv_get_visuals (Screen *screen, Visual **pseudocolour,
		     Visual **truecolour, Visual **directcolour)
/*  [SUMMARY] Get supported visuals available on a screen.
    <screen> The X Window screen.
    <pseudocolour> A PseudoColour XVisualInfo pointer will be written here. If
    this is NULL, nothing is written here. If no PseudoColour visual is
    supported, NULL is written here.
    <truecolour> A TrueColour XVisualInfo pointer will be written here. If this
    is NULL, nothing is written here. If no TrueColour visual is supported,
    NULL is written here.
    <directcolour> A DirectColour XVisualInfo pointer will be written here. If
    this is NULL, nothing is written here. If no DirectColour visual is
    supported, NULL is written here.
    [NOTE] The XVisualInfo structures returned must be freed by XFree()
    [RETURNS] Nothing.
*/
{
    XVisualInfo *pc = NULL;
    XVisualInfo *tc = NULL;
    XVisualInfo *dc = NULL;
    XVisualInfo **pc_ptr, **tc_ptr, **dc_ptr;

    if (pseudocolour == NULL) pc_ptr = NULL;
    else
    {
	pc_ptr = &pc;
	*pseudocolour = NULL;
    }
    if (truecolour == NULL) tc_ptr = NULL;
    else
    {
	tc_ptr = &tc;
	*truecolour = NULL;
    }
    if (directcolour == NULL) dc_ptr = NULL;
    else
    {
	dc_ptr = &dc;
	*directcolour = NULL;
    }
    xv_get_vinfos (screen, pc_ptr, tc_ptr, dc_ptr);
    if ( (pseudocolour != NULL) && (pc != NULL) ) *pseudocolour = pc->visual;
    if ( (truecolour != NULL) && (tc != NULL) ) *truecolour = tc->visual;
    if ( (directcolour != NULL) && (dc != NULL) ) *directcolour = dc->visual;
    if (pc != NULL) XFree (pc);
    if (tc != NULL) XFree (tc);
    if (dc != NULL) XFree (dc);
}   /*  End Function xv_get_visuals  */

/*PUBLIC_FUNCTION*/
XVisualInfo *xv_get_visinfo_for_visual (Display *dpy, Visual *visual)
/*  [SUMMARY] Get the visual information structure for a visual.
    <dpy> The X display.
    <visual> The visual.
    [RETURNS] A pointer to an XVisualInfo structure on succes, else NULL. The
    XVisualInfo structure must be freed by XFree()
*/
{
    int num_vinfos;
    XVisualInfo vinfo_template;
    XVisualInfo *vinfos;
    static char function_name[] = "xv_get_visinfo_for_visual";

    vinfo_template.visualid = XVisualIDFromVisual (visual);
    vinfos = XGetVisualInfo (dpy, VisualIDMask, &vinfo_template, &num_vinfos);
    if (num_vinfos < 1)
    {
	fprintf (stderr, "Error getting visual info for visual: %p\n",
		 visual);
	a_prog_bug (function_name);
    }
    if (num_vinfos > 1)
    {
	fprintf (stderr, "WARNING: number of visuals for visual: %p is: %d\n",
		 visual, num_vinfos);
    }
    return (vinfos);
}   /*  End Function xv_get_visinfo_for_visual  */
